#include "sdl_utilities.h"
#include "game_application.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for GameApplication:
//////////////////////////////////////////////////////////////////////////////////

GameApplication::GameApplication( SDL_GL_Window &initializer, const int fps ) :
    SDL_GL_Interface( initializer, fps ),
    camera_( Vector3f( -32.0f, 70.0f, -32.0f ), 0.15f, -25.0f, 225.0f ),
    // world_( time( NULL ) * 91387 + SDL_GetTicks() * 75181 )
    world_( 0x58afe359358eafd3 ) // FIXME: Using a constant for performance measurements.
{
    SCOPE_TIMER_BEGIN( "Updating chunk VBOs" )

    for ( ChunkMap::const_iterator chunk_it = world_.get_chunks().begin();
          chunk_it != world_.get_chunks().end();
          ++chunk_it )
    {
        renderer_.note_chunk_changes( *chunk_it->second );
    }

    SCOPE_TIMER_END
}

GameApplication::~GameApplication()
{
}

void GameApplication::handle_key_down_event( const int key, const int mod )
{
    bool handled = true;

    switch ( key )
    {
        case SDLK_LSHIFT:
            camera_.fast_move_mode( true );
            break;

        case SDLK_w:
            camera_.move_forward( true );
            break;

        case SDLK_s:
            camera_.move_backward( true );
            break;

        case SDLK_a:
            camera_.move_left( true );
            break;

        case SDLK_d:
            camera_.move_right( true );
            break;

        case SDLK_e:
            camera_.move_up( true );
            break;

        case SDLK_q:
            camera_.move_down( true );
            break;

        case SDLK_SPACE:
            SDL_ShowCursor( SDL_ShowCursor( SDL_QUERY ) == SDL_ENABLE ? SDL_DISABLE : SDL_ENABLE );
            SDL_WM_GrabInput( SDL_WM_GrabInput( SDL_GRAB_QUERY ) == SDL_GRAB_ON ? SDL_GRAB_OFF : SDL_GRAB_ON );
            break;

        default:
            handled = false;
            break;
    }
}

void GameApplication::handle_key_up_event( const int key, const int mod )
{
    switch ( key )
    {
        case SDLK_LSHIFT:
            camera_.fast_move_mode( false );
            break;

        case SDLK_w:
            camera_.move_forward( false );
            break;

        case SDLK_s:
            camera_.move_backward( false );
            break;

        case SDLK_a:
            camera_.move_left( false );
            break;

        case SDLK_d:
            camera_.move_right( false );
            break;

        case SDLK_e:
            camera_.move_up( false );
            break;

        case SDLK_q:
            camera_.move_down( false );
            break;

        case SDLK_ESCAPE:
            run_ = false;
            break;

        case SDLK_F11:
            toggle_fullscreen();
            break;
    }
}

void GameApplication::handle_mouse_motion_event( const int button, const int x, const int y, const int xrel, const int yrel )
{
    // FIXME: SDL_WarpMouse generates a mouse motion event.  This is a somewhat ugly way to ignore it.
    static bool first_event = true;

    if ( first_event )
    {
        first_event = false;
        return;
    }

    camera_.handle_mouse_motion( xrel, yrel );
}

void GameApplication::handle_mouse_down_event( const int button, const int x, const int y, const int xrel, const int yrel )
{
    switch ( button )
    {
        case SDL_BUTTON_LEFT:
            break;
        case SDL_BUTTON_RIGHT:
            break;
    }
}

void GameApplication::do_one_step( const float step_time )
{
    camera_.do_one_step( step_time );
    world_.do_one_step( step_time );
}

void GameApplication::render()
{
    renderer_.render( camera_, world_ );
}
