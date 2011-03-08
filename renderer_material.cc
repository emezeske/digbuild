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
                        // glTexImage2D( GL_TEXTURE_2D, 0, num_colors, surface->w, surface->h, 0, texture_format, GL_UNSIGNED_BYTE, surface->pixels );
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
// Static constant definitions for RendererMaterial:
//////////////////////////////////////////////////////////////////////////////////

const std::string
    RendererMaterial::TEXTURE_DIRECTORY = "./media/materials/textures",
    RendererMaterial::SHADER_DIRECTORY  = "./media/materials/shaders";

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for RendererMaterial:
//////////////////////////////////////////////////////////////////////////////////

// TODO: Only load textures/shaders once, and keep referenes to them.

RendererMaterial::RendererMaterial( const std::string& name ) :
    texture_( TEXTURE_DIRECTORY + "/" + name + ".png" ),
    shader_( SHADER_DIRECTORY + "/default.vertex.glsl", SHADER_DIRECTORY + "/default.fragment.glsl" )
{
}
