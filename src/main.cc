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

int main( int argc, char **argv )
{
    int result = -1;

    try
    {
        SDL_GL_Window window( "Digbuild" );
        GameApplication game( window );
        game.main_loop();
        result =  0;
    }
    catch ( const std::exception& e ) { LOG( "Error: " << e.what() << "." ); }

    return result;
}
