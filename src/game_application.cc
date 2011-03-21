#include "sdl_utilities.h"
#include "game_application.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for GameApplication:
//////////////////////////////////////////////////////////////////////////////////

GameApplication::GameApplication( SDL_GL_Window &window, const unsigned fps_limit ) :
    run_( false ),
    fps_limit_( fps_limit ),
    fps_last_time_( 0 ),
    fps_frame_count_( 0 ),
    mouse_sensitivity_( 0.005f ),
    window_( window ),
    gui_focused_( false ),
    player_( Vector3f( 0.0f, 200.0f, 0.0f ), gmtl::Math::PI_OVER_2, gmtl::Math::PI_OVER_4 ),
    // world_( time( NULL ) * 91387 + SDL_GetTicks() * 75181 )
    world_( 0xeaafa35aaa8eafdf ), // FIXME: Using a constant for consistent performance measurements.
    gui_( window_.get_screen() )
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
    SDL_Quit();
}

void GameApplication::main_loop()
{
    run_ = true;

    // TODO: Pull the high-resolution clock stuff below out into a class, and add
    //       similar calculations for Windows using QueryPerformanceTimer.

    timespec
        resolution,
        last_time,
        current_time;

    int clock_result = clock_getres( CLOCK_MONOTONIC_RAW, &resolution );

    if ( clock_result == -1 )
    {
        throw std::runtime_error( "Unable to determine clock resolution" );
    }

    clock_result = clock_gettime( CLOCK_MONOTONIC_RAW, &last_time );

    if ( clock_result == -1 )
    {
        throw std::runtime_error( "Unable to read clock" );
    }

    while ( run_ )
    {
        clock_result = clock_gettime( CLOCK_MONOTONIC_RAW, &current_time );

        if ( clock_result == -1 )
        {
            throw std::runtime_error( "Unable to read clock" );
        }

        const double step_time_seconds = 
            double( current_time.tv_sec - last_time.tv_sec ) +
            double( current_time.tv_nsec - last_time.tv_nsec ) / 1000000000.0;

        const double seconds_until_next_frame = 1.0f / fps_limit_ - step_time_seconds;

        process_events();

        if ( seconds_until_next_frame <= 0.0 )
        {
            last_time = current_time;
            do_one_step( float( step_time_seconds ) );
            render();
        }
        else SDL_Delay( int( seconds_until_next_frame * 1000.0 ) );
    }
}

void GameApplication::process_events()
{
    SDL_Event event;

    while( SDL_PollEvent( &event ) ) 
    {
        handle_event( event );
    }
}

void GameApplication::handle_event( SDL_Event &event )
{
    if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE )
    {
        toggle_gui_focus();
        return;
    }

    if ( gui_focused_ )
    {
        gui_.handle_event( event );
        return;
    }

    switch ( event.type )
    {
        case SDL_KEYDOWN:
            handle_key_down_event( event.key.keysym.sym, event.key.keysym.mod );
            break;

        case SDL_KEYUP:
            handle_key_up_event( event.key.keysym.sym, event.key.keysym.mod );
            break;

        case SDL_MOUSEMOTION:
            handle_mouse_motion_event( event.button.button, event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel );
            break;

        case SDL_MOUSEBUTTONDOWN:
            handle_mouse_down_event( event.button.button, event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel );
            break;

        case SDL_MOUSEBUTTONUP:
            handle_mouse_up_event( event.button.button, event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel );
            break;
    
        case SDL_VIDEORESIZE:
            window_.reshape_window( event.resize.w, event.resize.h );
            gui_.handle_event( event );
            break;
    
        case SDL_QUIT:
            run_ = false;
            break;
    }
}

void GameApplication::handle_key_down_event( const int key, const int mod )
{
    switch ( key )
    {
        case SDLK_LSHIFT:
            player_.request_fast_move( true );
            break;

        case SDLK_w:
            player_.request_move_forward( true );
            break;

        case SDLK_s:
            player_.request_move_backward( true );
            break;

        case SDLK_a:
            player_.request_strafe_left( true );
            break;

        case SDLK_d:
            player_.request_strafe_right( true );
            break;

        case SDLK_e:
            player_.request_jump( true );
            break;

        case SDLK_q:
            player_.request_crouch( true );
            break;

        case SDLK_b:
            player_.toggle_noclip();
            break;

        default:
            break;
    }
}

void GameApplication::handle_key_up_event( const int key, const int mod )
{
    switch ( key )
    {
        case SDLK_LSHIFT:
            player_.request_fast_move( false );
            break;

        case SDLK_w:
            player_.request_move_forward( false );
            break;

        case SDLK_s:
            player_.request_move_backward( false );
            break;

        case SDLK_a:
            player_.request_strafe_left( false );
            break;

        case SDLK_d:
            player_.request_strafe_right( false );
            break;

        case SDLK_e:
            player_.request_jump( false );
            break;

        case SDLK_q:
            player_.request_crouch( false );
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
    // When the SDL library is first started, it will generate a mouse motion event with the current
    // position of the cursor.  We ignore it so that the initial camera settings remain intact.
    static bool first_event = true;

    if ( first_event )
    {
        first_event = false;
        return;
    }

    player_.adjust_direction( mouse_sensitivity_ * Scalar( yrel ), mouse_sensitivity_ * Scalar( -xrel ) );
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

void GameApplication::handle_mouse_up_event( const int button, const int x, const int y, const int xrel, const int yrel )
{
    switch ( button )
    {
        case SDL_BUTTON_LEFT:
            break;
        case SDL_BUTTON_RIGHT:
            break;
    }
}

void GameApplication::toggle_fullscreen()
{
    SDL_Surface* s = SDL_GetVideoSurface();

    if( !s || ( SDL_WM_ToggleFullScreen( s ) != 1 ) )
    {
        LOG( "Unable to toggle fullscreen: " << SDL_GetError() );
    }
}

void GameApplication::toggle_gui_focus()
{
    SDL_ShowCursor( SDL_ShowCursor( SDL_QUERY ) == SDL_ENABLE ? SDL_DISABLE : SDL_ENABLE );
    SDL_WM_GrabInput( SDL_WM_GrabInput( SDL_GRAB_QUERY ) == SDL_GRAB_ON ? SDL_GRAB_OFF : SDL_GRAB_ON );
    gui_focused_ = !gui_focused_;
}

void GameApplication::do_one_step( const float step_time )
{
    player_.do_one_step( step_time, world_ );
    world_.do_one_step( step_time );
    gui_.do_one_step( step_time );
}

void GameApplication::render()
{
    ++fps_frame_count_;
    const unsigned now = SDL_GetTicks();

    if ( fps_last_time_ + 1000 < now )
    {
        fps_last_time_ = now;
        gui_.set_engine_fps( fps_frame_count_ );
        fps_frame_count_ = 0;
    }

    glClear( GL_DEPTH_BUFFER_BIT );
    window_.reshape_window();

    Camera camera( player_.get_eye_position(), player_.get_pitch(), player_.get_yaw(), window_.get_draw_distance() );
    renderer_.render( camera, world_ );
    gui_.set_engine_chunk_stats( renderer_.get_num_chunks_drawn(), world_.get_chunks().size(), renderer_.get_num_triangles_drawn() );
    gui_.render();

    SDL_GL_SwapBuffers();
}
