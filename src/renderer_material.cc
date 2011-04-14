#include <SDL/SDL_image.h> 

#include <stdexcept> 

#include "renderer_material.h"

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
    std::string error_message;
    SDL_Surface *surface = IMG_Load( filename.c_str() );
    
    if ( surface )
    {
        // Ensure that the width and height of the image are powers of 2.
        if ( ( surface->w & ( surface->w - 1 ) ) == 0 ) 
        {
            if ( ( surface->h & ( surface->h - 1 ) ) == 0 )
            {
                GLint num_colors = surface->format->BytesPerPixel;
                GLenum texture_format = 0;
                size_ = Vector2i( surface->w, surface->h );
        
                if ( num_colors == 4 ) // Contains an alpha channel.
                {
                    texture_format = ( surface->format->Rmask == 0x000000ff ) ? GL_RGBA : GL_BGRA;
                } 
                else if ( num_colors == 3 ) // No alpha channel.
                {
                    texture_format = ( surface->format->Rmask == 0x000000ff ) ? GL_RGB : GL_BGR;
                } 
                else if ( num_colors == 1 ) // Luminance map.
                {
                    texture_format = GL_LUMINANCE;
                } 
        
                if ( texture_format )
                {
                    glGenTextures( 1, &texture_id_ );
                    glBindTexture( GL_TEXTURE_2D, texture_id_ );

                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        
                    if ( SDL_LockSurface( surface ) != -1 )
                    {
                        gluBuild2DMipmaps( GL_TEXTURE_2D, num_colors, surface->w, surface->h, texture_format, GL_UNSIGNED_BYTE, surface->pixels );
                        SDL_UnlockSurface( surface );
                    }
                    else error_message = "SDL_LockSurface() failed: " + std::string( SDL_GetError() );
                }
                else error_message = "Unsupported color channel count.";
            }
            else error_message = "Width is not a power of 2.";
        }
        else error_message = "Height is not a power of 2.";
    
        SDL_FreeSurface( surface );
    }
    else error_message = "IMG_Load() failed: " + std::string( IMG_GetError() );

    if ( !error_message.empty() ) throw std::runtime_error( "Failed to load texture file '" + filename + "': " + error_message );
}

Texture::~Texture()
{
    glDeleteTextures( 1, &texture_id_ );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for RendererMaterial:
//////////////////////////////////////////////////////////////////////////////////

RendererMaterial::RendererMaterial( const std::string& name, ShaderSP shader ) :
    texture_( RendererMaterialManager::TEXTURE_DIRECTORY + "/" + name + ".png" ),
    specular_map_( RendererMaterialManager::TEXTURE_DIRECTORY + "/" + name + ".specular.png" ),
    bump_map_( RendererMaterialManager::TEXTURE_DIRECTORY + "/" + name + ".bump.png" ),
    shader_( shader )
{
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
    current_material_( BLOCK_MATERIAL_AIR )
{
    ShaderSP default_block_shader(
        new Shader( SHADER_DIRECTORY + "/block.vertex.glsl", SHADER_DIRECTORY + "/block.fragment.glsl" )
    );

    materials_.resize( BLOCK_MATERIAL_SIZE );
    materials_[BLOCK_MATERIAL_GRASS].reset      ( new RendererMaterial( "grass",       default_block_shader ) );
    materials_[BLOCK_MATERIAL_DIRT].reset       ( new RendererMaterial( "dirt",        default_block_shader ) );
    materials_[BLOCK_MATERIAL_CLAY].reset       ( new RendererMaterial( "clay",        default_block_shader ) );
    materials_[BLOCK_MATERIAL_STONE].reset      ( new RendererMaterial( "stone",       default_block_shader ) );
    materials_[BLOCK_MATERIAL_BEDROCK].reset    ( new RendererMaterial( "bedrock",     default_block_shader ) );
    materials_[BLOCK_MATERIAL_MAGMA].reset      ( new RendererMaterial( "magma",       default_block_shader ) );
    materials_[BLOCK_MATERIAL_TREE_TRUNK].reset ( new RendererMaterial( "tree-trunk",  default_block_shader ) );
    materials_[BLOCK_MATERIAL_TREE_LEAF].reset  ( new RendererMaterial( "tree-leaf",   default_block_shader ) );
    materials_[BLOCK_MATERIAL_GLASS_CLEAR].reset( new RendererMaterial( "glass-clear", default_block_shader ) );
    materials_[BLOCK_MATERIAL_GLASS_RED].reset  ( new RendererMaterial( "glass-red",   default_block_shader ) );
    materials_[BLOCK_MATERIAL_WATER].reset      ( new RendererMaterial( "water",       default_block_shader ) );
}

void RendererMaterialManager::configure_block_material( const Camera& camera, const Sky& sky, const BlockMaterial material )
{
    assert( material >= 0 && material < static_cast<int>( materials_.size() ) && material != BLOCK_MATERIAL_AIR );

    if ( material == current_material_ )
    {
        return;
    }

    current_material_ = material;
    const RendererMaterial& renderer_material = *materials_[material];

    glEnable( GL_TEXTURE_2D );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, renderer_material.get_texture().get_texture_id() );

    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, renderer_material.get_specular_map().get_texture_id() );

    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, renderer_material.get_bump_map().get_texture_id() );

    if ( renderer_material.get_shader().get() == current_shader_.get() )
    {
        return;
    }

    current_shader_ = renderer_material.get_shader();
    current_shader_->enable();

    const Vector3f
        sun_direction = spherical_to_cartesian( Vector3f( 1.0f, sky.get_sun_angle()[0], sky.get_sun_angle()[1] ) ),
        moon_direction = spherical_to_cartesian( Vector3f( 1.0f, sky.get_moon_angle()[0], sky.get_moon_angle()[1] ) );

    current_shader_->set_uniform_vec3f( "camera_position", camera.get_position() );
    current_shader_->set_uniform_float( "fog_distance", camera.get_draw_distance() );

    current_shader_->set_uniform_vec3f( "sun_direction", sun_direction );
    current_shader_->set_uniform_vec3f( "moon_direction", moon_direction );

    current_shader_->set_uniform_vec3f( "sun_light_color", sky.get_sun_light_color() );
    current_shader_->set_uniform_vec3f( "moon_light_color", sky.get_moon_light_color() );

    current_shader_->set_uniform_int( "material_texture", 0 );
    current_shader_->set_uniform_int( "material_specular_map", 1 );
    current_shader_->set_uniform_int( "material_bump_map", 2 );
}

void RendererMaterialManager::deconfigure_block_material()
{
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    glDisable( GL_TEXTURE_2D );

    if ( current_shader_.get() )
    {
        current_shader_->disable();
        current_shader_.reset();
    }

    current_material_ = BLOCK_MATERIAL_AIR;
}
