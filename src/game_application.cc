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
    window_( window ),
    gui_focused_( false ),
    camera_( Vector3f( -32.0f, 70.0f, -32.0f ), 0.15f, -25.0f, 225.0f ),
    // world_( time( NULL ) * 91387 + SDL_GetTicks() * 75181 )
    world_( 0xeaafa35aaa8eafdf ), // FIXME: Using a constant for performance measurements.
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

        default:
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
    camera_.do_one_step( step_time );
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
    window_.reshape_window(); // FIXME: This makes Agar work.  Patch Agar instead?

    renderer_.render( camera_, world_ );
    gui_.set_engine_chunk_stats( renderer_.get_num_chunks_drawn(), world_.get_chunks().size(), renderer_.get_num_triangles_drawn() );
    gui_.render();

    SDL_GL_SwapBuffers();
}
