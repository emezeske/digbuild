#include <SDL/SDL_image.h> 

#include <stdexcept> 

#include "log.h"
#include "renderer_material.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

void texture_load_failure( const std::string& filename, const std::string& message )
{
    throw std::runtime_error( "Failed to load texture file '" + filename + "': " + message );
}

struct SurfaceGuard
{
    SurfaceGuard( const std::string& filename ) :
        surface_( IMG_Load( filename.c_str() ) )
    {
        if ( !surface_ )
        {
            texture_load_failure( filename, "IMG_Load() failed: " + std::string( IMG_GetError() ) );
        }
    }

    ~SurfaceGuard()
    {
        SDL_FreeSurface( surface_ );
    }

    SDL_Surface* surface_;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Texture:
//////////////////////////////////////////////////////////////////////////////////

Texture::Texture() :
    texture_id_( 0 ),
    size_( 0, 0 )
{
}

Texture::Texture( const std::string& filename ) :
    texture_id_( 0 ),
    size_( 0, 0 )
{
    SurfaceGuard guard( filename );
    
    // Ensure that the width and height of the image are powers of 2.
    if ( ( guard.surface_->w & ( guard.surface_->w - 1 ) ) != 0 )
    {
        texture_load_failure( filename, make_string() << "Width (" << guard.surface_->w << ") is not a power of 2." );
    }

    if ( ( guard.surface_->h & ( guard.surface_->h - 1 ) ) != 0 )
    {
        texture_load_failure( filename, make_string() << "Height (" << guard.surface_->h << ") is not a power of 2." );
    }

    GLint num_colors = guard.surface_->format->BytesPerPixel;
    GLenum texture_format = 0;
    size_ = Vector2i( guard.surface_->w, guard.surface_->h );

    if ( num_colors == 4 ) // Contains an alpha channel.
    {
        texture_format = ( guard.surface_->format->Rmask == 0x000000ff ) ? GL_RGBA : GL_BGRA;
    }
    else if ( num_colors == 3 ) // No alpha channel.
    {
        texture_format = ( guard.surface_->format->Rmask == 0x000000ff ) ? GL_RGB : GL_BGR;
    }
    else if ( num_colors == 1 ) // Luminance map.
    {
        texture_format = GL_LUMINANCE;
    }
    
    if ( texture_format == 0 )
    {
        texture_load_failure( filename, make_string() << "Unsupported color channel count: " << num_colors );
    }

    glGenTextures( 1, &texture_id_ );
    glBindTexture( GL_TEXTURE_2D, texture_id_ );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );

    if ( SDL_LockSurface( guard.surface_ ) == -1 )
    {
        texture_load_failure( filename, "SDL_LockSurface() failed: " + std::string( SDL_GetError() ) );
    }

    glTexImage2D( GL_TEXTURE_2D, 0, texture_format, size_[0], size_[1], 0, texture_format, GL_UNSIGNED_BYTE, guard.surface_->pixels );
    glGenerateMipmap( GL_TEXTURE_2D );
    SDL_UnlockSurface( guard.surface_ );
}

Texture::~Texture()
{
    glDeleteTextures( 1, &texture_id_ );
}

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for RendererMaterial:
//////////////////////////////////////////////////////////////////////////////////

const std::string
    RendererMaterialManager::TEXTURE_DIRECTORY = "./media/materials/textures",
    RendererMaterialManager::SHADER_DIRECTORY  = "./media/materials/shaders";

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for RendererMaterialManager:
//////////////////////////////////////////////////////////////////////////////////

RendererMaterialManager::RendererMaterialManager() :
    material_shader_( new Shader( SHADER_DIRECTORY + "/block.vertex.glsl", SHADER_DIRECTORY + "/block.fragment.glsl" ) )
{
    GLint supported_layers;
    glGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS, &supported_layers );

    if ( supported_layers < NUM_BLOCK_MATERIALS )
    {
        throw std::runtime_error( make_string() << "The supported number of array texture layers ("
            << supported_layers << ") is less than required (" << NUM_BLOCK_MATERIALS << ")" );
    }

    // Using texture arrays provides a HUGE speedup when rendering complex combinations
    // of translucent materials.  Since translucent materials must be rendered strictly
    // in back-to-front order, there are situations in which, due to the viewing angle,
    // every other face has a different material.  Switching materials for each face is
    // a major overhead, due in no small part to the sheer function call overhead.
    //
    // Texture arrays allow the texture number to be included as a per-vertex field,
    // which means that a VBO with complex translucent materials can be rendered with 
    // just a couple of calls: a call to reorder the vertex indices, and a call to draw
    // them all.  This is MUCH FASTER.

    create_texture_array( ".png",          TEXTURE_SIZE,      TEXTURE_CHANNELS,      texture_array_id_ );
    create_texture_array( ".bump.png",     BUMP_MAP_SIZE,     BUMP_MAP_CHANNELS,     bump_map_array_id_ );
    create_texture_array( ".specular.png", SPECULAR_MAP_SIZE, SPECULAR_MAP_CHANNELS, specular_map_array_id_ );
}

RendererMaterialManager::~RendererMaterialManager()
{
    glDeleteTextures( 1, &texture_array_id_ );
    glDeleteTextures( 1, &bump_map_array_id_ );
    glDeleteTextures( 1, &specular_map_array_id_ );
}

void RendererMaterialManager::configure_materials( const Camera& camera, const Sky& sky )
{
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D_ARRAY, texture_array_id_ );

    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D_ARRAY, specular_map_array_id_ );

    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D_ARRAY, bump_map_array_id_ );

    material_shader_->enable();

    const Vector3f
        sun_direction = spherical_to_cartesian( Vector3f( 1.0f, sky.get_sun_angle()[0], sky.get_sun_angle()[1] ) ),
        moon_direction = spherical_to_cartesian( Vector3f( 1.0f, sky.get_moon_angle()[0], sky.get_moon_angle()[1] ) );

    material_shader_->set_uniform_vec3f( "camera_position", camera.get_position() );
    material_shader_->set_uniform_float( "fog_distance", camera.get_draw_distance() );

    material_shader_->set_uniform_vec3f( "sun_direction", sun_direction );
    material_shader_->set_uniform_vec3f( "moon_direction", moon_direction );

    material_shader_->set_uniform_vec3f( "sun_light_color", sky.get_sun_light_color() );
    material_shader_->set_uniform_vec3f( "moon_light_color", sky.get_moon_light_color() );

    material_shader_->set_uniform_int( "material_texture_array", 0 );
    material_shader_->set_uniform_int( "material_specular_map_array", 1 );
    material_shader_->set_uniform_int( "material_bump_map_array", 2 );
}

void RendererMaterialManager::deconfigure_materials()
{
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D_ARRAY, 0 );

    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D_ARRAY, 0 );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D_ARRAY, 0 );

    glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );

    material_shader_->disable();
}

void RendererMaterialManager::read_texture_data(
    const std::string& filename,
    const int size,
    const int channels,
    std::vector<unsigned char>& texture_data
)
{
    SurfaceGuard guard( filename );
    
    if ( guard.surface_->w != size || guard.surface_->h != size )
    {
        texture_load_failure( filename, make_string() << "Texture size ("
            << guard.surface_->w << ", " << guard.surface_->h << ") does not have the expected size ("
            << size << ", " << size << ")" );
    }

    if ( guard.surface_->format->BytesPerPixel != channels )
    {
        texture_load_failure( filename, make_string() << "Texture channel count ("
            << int( guard.surface_->format->BytesPerPixel ) << ") is not as expected (" << channels << ")" );
    }

    if ( guard.surface_->format->BytesPerPixel == 3 ||
         guard.surface_->format->BytesPerPixel == 4 )
    {
        if ( guard.surface_->format->Rmask != 0x000000ff )
        {
            texture_load_failure( filename, "Texture does not have the expected color channel ordering" );
        }
    }

    if ( SDL_LockSurface( guard.surface_ ) == -1 )
    {
        texture_load_failure( filename, "SDL_LockSurface() failed: " + std::string( SDL_GetError() ) );
    }

    const size_t pixel_bytes = size * size * channels;
    const size_t texture_data_start = texture_data.size();
    texture_data.resize( texture_data.size() + pixel_bytes );
    memcpy( &texture_data[texture_data_start], guard.surface_->pixels, pixel_bytes );
    SDL_UnlockSurface( guard.surface_ );
}

void RendererMaterialManager::create_texture_array(
    const std::string& filename_postfix,
    const int texture_size,
    const int texture_channels,
    GLuint& id
)
{
    std::vector<unsigned char> texture_data;

    FOREACH_BLOCK_MATERIAL( material )
    {
        const BlockMaterialAttributes& attributes = get_block_material_attributes( material );
        const std::string filename_base = TEXTURE_DIRECTORY + "/" + attributes.texture_filename_;
        read_texture_data( filename_base + filename_postfix, texture_size, texture_channels, texture_data );
    }

    GLenum texture_format = 0;

    switch ( texture_channels )
    {
        case 4: texture_format = GL_RGBA; break;
        case 3: texture_format = GL_RGB; break;
        case 1: texture_format = GL_LUMINANCE; break;
        default: throw std::runtime_error( make_string() << "Unsupported number of texture channels: " << texture_channels );
    }

    glGenTextures( 1, &id );
    glBindTexture( GL_TEXTURE_2D_ARRAY, id );

    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );

    glTexImage3D( GL_TEXTURE_2D_ARRAY, 0, texture_format, texture_size, texture_size, NUM_BLOCK_MATERIALS, 0, texture_format, GL_UNSIGNED_BYTE, &texture_data[0] );
    glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}
