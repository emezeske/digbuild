///////////////////////////////////////////////////////////////////////////
// Copyright 2011 Evan Mezeske.
//
// This file is part of Digbuild.
// 
// Digbuild is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.
// 
// Digbuild is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Digbuild.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////

#include <GL/glew.h>

#define NO_SDL_GLEXT
#include <SDL/SDL_opengl.h>

#include <stdexcept>
#include <time.h>
#include <assert.h>

#include "sdl_gl_window.h"
#include "log.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for SDL_GL_Window:
//////////////////////////////////////////////////////////////////////////////////

SDL_GL_Window::SDL_GL_Window( const int w, const int h, const int bpp, const Uint32 flags, const std::string &title ) :
    screen_( 0 ),
    screen_size_( w, h ), 
    screen_bpp_( bpp ),
    sdl_video_flags_( flags ),
    draw_distance_( 250.0f ),
    title_( title )
{
    if ( SDL_Init( SDL_INIT_VIDEO ) == -1 )
    {
        throw std::runtime_error( std::string( "Error initializing SDL: " ) + SDL_GetError() );
    }

    // TODO: Check return values here, and verify that they have not changed after the SDL_SetVideoMode() call.
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    // TODO: Antialiasing level should be configurable.
    SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
    SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

    // TODO: Vsync should be configurable.
    SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 ); 

    screen_ = SDL_SetVideoMode( screen_size_[0], screen_size_[1], screen_bpp_, sdl_video_flags_ );

    if ( !screen_ )
    {
        throw std::runtime_error( std::string( "Error setting video mode: " ) + SDL_GetError() );
    }

    SDL_WM_SetCaption( title_.c_str(), NULL );
    SDL_FillRect( screen_, NULL, SDL_MapRGBA( screen_->format, 0, 0, 0, 0 ) );

    SDL_ShowCursor( SDL_DISABLE );
    SDL_WM_GrabInput( SDL_GRAB_ON );

    init_GL();
    reshape_window();
}

void SDL_GL_Window::init_GL()
{
    GLenum glew_result = glewInit();

    if ( glew_result != GLEW_OK )
    {
        throw std::runtime_error( "Error initializing GLEW: " +
            std::string( reinterpret_cast<const char*>( glewGetErrorString( glew_result ) ) ) );
    }

    if ( !glewIsSupported( "GL_VERSION_2_0" ) )
    {
        throw std::runtime_error( "OpenGL 2.0 not supported" );
    }

    glShadeModel( GL_SMOOTH );
    glClearDepth( 1.0f );
    glEnable( GL_MULTISAMPLE );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    glHint( GL_GENERATE_MIPMAP_HINT, GL_NICEST );
    glHint( GL_TEXTURE_COMPRESSION_HINT, GL_NICEST );
    glHint( GL_FOG_HINT, GL_NICEST );
}

void SDL_GL_Window::reshape_window( const int w, const int h )
{
    screen_size_ = Vector2i( w, h );
    reshape_window();
}

void SDL_GL_Window::reshape_window()
{
    glViewport( 0, 0, ( GLsizei )( screen_size_[0] ), ( GLsizei )( screen_size_[1] ) );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 65.0f, ( GLfloat )( screen_size_[0] ) / ( GLfloat )( screen_size_[1] ), 0.1f, draw_distance_ );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}
