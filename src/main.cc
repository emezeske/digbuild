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

#include "log.h"
#include "game_application.h"

const int
    SCREEN_WIDTH  = 1920,
    SCREEN_HEIGHT = 1100,
    SCREEN_BPP    = 32;

// FIXME: SDL_RESIZABLE is apparently not compatible with SDL_NOFRAME.  When they are both enabled,
//        resize events clobbers the OpenGL state and require OpenGL to be fully reinitialized.
//        This might be fixed in SDL 1.3.
const Uint32 SCREEN_FLAGS = SDL_OPENGL | SDL_RESIZABLE;

int main( int argc, char **argv )
{
    int result = -1;

    try
    {
        SDL_GL_Window window( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SCREEN_FLAGS, "DigBuild" );
        GameApplication game( window );
        game.main_loop();
        result =  0;
    }
    catch ( const std::exception& e ) { LOG( "Error: " << e.what() << "." ); }

    return result;
}
