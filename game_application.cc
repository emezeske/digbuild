#include <GL/gl.h>

#include "sdl_utilities.h"
#include "game_application.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for GameWindow:
//////////////////////////////////////////////////////////////////////////////////

GameWindow::GameWindow( const int w, const int h, const int bpp, const Uint32 flags, const std::string &title ) : 
    SDL_GL_Window( w, h, bpp, flags, title )
{
}

void GameWindow::create_window()
{
    SDL_GL_Window::create_window();

    SDL_ShowCursor( SDL_DISABLE );
    SDL_WM_GrabInput( SDL_GRAB_ON );
}

void GameWindow::init_GL()
{
    SDL_GL_Window::init_GL();

    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat light_specular[] = { 0.01f, 0.01f, 0.01f, 1.0f };
    GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f };
  
    glLightfv( GL_LIGHT0, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, light_specular );
    glLightfv( GL_LIGHT0, GL_POSITION, light_position );
  
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );
 
    glShadeModel( GL_SMOOTH );
    glEnable( GL_COLOR_MATERIAL );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for GameApplication:
//////////////////////////////////////////////////////////////////////////////////

GameApplication::GameApplication( SDL_GL_Window &initializer, const int fps ) :
    SDL_GL_Interface( initializer, fps ),
    camera_( Vector3f( -32.0f, 70.0f, -32.0f ), 0.15f, -25.0f, 225.0f )
{
    const long ticks = SDL_GetTicks();
    // const uint64_t world_seed = time( NULL ) * 91387 + ticks * 75181;
    const uint64_t world_seed = 0x58afe359358eafd3; // FIXME: Using a constant for performance measurements.

    SCOPE_TIMER_BEGIN
    for ( int x = 0; x < 3; ++x )
    {
        for ( int y = 0; y < 3; ++y )
        {
            const Vector2i position( x * Region::REGION_SIZE, y * Region::REGION_SIZE );
            RegionSP region( new Region( world_seed, position ) );
            regions_[position] = region;
        }
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
            printf( "Camera: pitch: %f, yaw: %f\n", camera_.pitch_, camera_.yaw_ );
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

void GameApplication::do_one_step( float step_time )
{
    camera_.do_one_step( step_time );
}

void GameApplication::render()
{
    camera_.render(); // Perform camera based translation and rotation.

    static bool first_time = true;

    if ( first_time )
    {
        SCOPE_TIMER_BEGIN
        renderer_.render( regions_ );
        SCOPE_TIMER_END
        first_time = false;
    }
    else renderer_.render( regions_ );
}
