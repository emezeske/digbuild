#include "log.h"
#include "game_application.h"

const int
    SCREEN_WIDTH  = 1920,
    SCREEN_HEIGHT = 1100,
    SCREEN_BPP    = 32;

const Uint32 SCREEN_FLAGS = SDL_OPENGL;

int main( int argc, char **argv )
{
    int result = -1;

    try
    {
        SDL_GL_Window window( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SCREEN_FLAGS, "DigBuild" );
        GameApplication game( window, 60 );
        game.main_loop();
        result =  0;
    }
    catch ( const std::exception& e ) { LOG( "Error: " << e.what() ); }

    return result;
}