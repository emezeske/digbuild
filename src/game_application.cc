#ifdef DEBUG_CHUNK_UPDATES
# include <boost/random/uniform_int.hpp>
# include <boost/random/uniform_real.hpp>
# include <boost/random/uniform_on_sphere.hpp>
# include <boost/random/variate_generator.hpp>
# include <boost/random/linear_congruential.hpp>
#endif

#include <boost/foreach.hpp>

#include "log.h"
#include "timer.h"
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
    // world_( time( NULL ) * 91387 + SDL_GetTicks() * 75181 ),
    world_( 0xeaafa35aaa8eafdf ), // NOTE: Always use a constant for consistent performance measurements.
    gui_( window_.get_screen() ),
    chunk_updater_( 1 )
{
    World::ChunkGuard chunk_guard( world_.get_chunk_lock() );

    SCOPE_TIMER_BEGIN( "Updating chunk VBOs" )

    BOOST_FOREACH( const ChunkMap::value_type& chunk_it, world_.get_chunks() )
    {
        renderer_.note_chunk_changes( *chunk_it.second );
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

    HighResolutionTimer frame_timer;

    while ( run_ )
    {
        const double
            step_time_seconds = frame_timer.get_seconds_elapsed(),
            seconds_until_next_frame = 1.0f / fps_limit_ - step_time_seconds;

        process_events();

        // TODO: Revisit this.  Maybe don't lock the framerate, but do lock the simulation rate.

        if ( seconds_until_next_frame <= 0.0 )
        {
            frame_timer.reset();
            do_one_step( float( step_time_seconds ) );
            render();
        }
        else do_delay( seconds_until_next_frame );
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
    switch ( event.type )
    {
        case SDL_KEYDOWN:
            if ( event.key.keysym.sym == SDLK_e )
            {
                toggle_gui_focus();
                return;
            }
            break;

        case SDL_VIDEORESIZE:
            std::cout << "w: " << event.resize.w << ", h: " << event.resize.h << std::endl;
            window_.reshape_window( event.resize.w, event.resize.h );
            gui_.handle_event( event );
            return;

        case SDL_QUIT:
            run_ = false;
            break;
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
    }
}

void GameApplication::handle_key_down_event( const int key, const int mod )
{
    switch ( key )
    {
        case SDLK_LCTRL:
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

        case SDLK_SPACE:
            player_.request_jump( true );
            break;

        case SDLK_LSHIFT:
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
        case SDLK_LCTRL:
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

        case SDLK_SPACE:
            player_.request_jump( false );
            break;

        case SDLK_LSHIFT:
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
            player_.request_primary_fire( true );
            break;
        case SDL_BUTTON_RIGHT:
            player_.request_secondary_fire( true );
            break;
        case SDL_BUTTON_WHEELUP:
            player_.select_next_material();
            break;
        case SDL_BUTTON_WHEELDOWN:
            player_.select_previous_material();
            break;
    }
}

void GameApplication::handle_mouse_up_event( const int button, const int x, const int y, const int xrel, const int yrel )
{
    switch ( button )
    {
        case SDL_BUTTON_LEFT:
            player_.request_primary_fire( false );
            break;
        case SDL_BUTTON_RIGHT:
            player_.request_secondary_fire( false );
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

void GameApplication::handle_chunk_updates()
{
    if ( !updated_chunks_.empty() )
    {
        World::ChunkGuard chunk_guard( world_.get_chunk_lock() );

        SCOPE_TIMER_BEGIN( "Updating chunk VBOs" )

        BOOST_FOREACH( Chunk* chunk, updated_chunks_ )
        {
            renderer_.note_chunk_changes( *chunk );
        }

        updated_chunks_.clear();

        SCOPE_TIMER_END
    }
}

void GameApplication::do_one_step( const float step_time )
{
    // Hopefully, handle_chunk_updates() will end up being called during the frame
    // delay, but it's called here as well, in case the rendering loop is FPS capped
    // and there was no delay.
    handle_chunk_updates();

    World::ChunkGuard chunk_guard( world_.get_chunk_lock() );

    player_.do_one_step( step_time, world_ );
    world_.do_one_step( step_time );
    gui_.do_one_step( step_time );

#ifdef DEBUG_CHUNK_UPDATES
    static boost::rand48 generator( 0 );

    const ChunkMap& chunks = world_.get_chunks();

    boost::variate_generator<boost::rand48&, boost::uniform_int<> >
        chunk_random( generator, boost::uniform_int<>( 0, chunks.size() - 1 ) );

    ChunkMap::const_iterator chunk_it = chunks.begin();
    std::advance( chunk_it, chunk_random() );
    const Vector3i& chunk_position = chunk_it->second->get_position();

    boost::variate_generator<boost::rand48&, boost::uniform_int<> >
        x_random( generator, boost::uniform_int<>( 0, Chunk::SIZE_X - 1 ) ),
        y_random( generator, boost::uniform_int<>( 0, Chunk::SIZE_Y - 1 ) ),
        z_random( generator, boost::uniform_int<>( 0, Chunk::SIZE_Z - 1 ) );

    const Vector3i block_position = chunk_position + Vector3i( x_random(), y_random(), z_random() );
    BlockIterator block_it = world_.get_block( block_position );
    assert( block_it.block_ );

    if ( block_it.block_->get_material() == BLOCK_MATERIAL_AIR )
    {
        block_it.block_->set_material( BLOCK_MATERIAL_GRASS );
    }
    else block_it.block_->set_material( BLOCK_MATERIAL_AIR );

    world_.mark_chunk_for_update( block_it.chunk_ );
#endif

    if ( !world_.is_chunk_update_in_progress() )
    {
        chunk_updater_.wait();
        updated_chunks_ = world_.get_updated_chunks();
        chunk_updater_.schedule( boost::bind( &World::update_chunks, boost::ref( world_ ) ) );
    }
}

void GameApplication::do_delay( const float delay_time )
{
    long delay_ms = long( delay_time * 1000.0 );
    const long begin_ticks = SDL_GetTicks();

    // If any Chunks need to be sent down to the Renderer, do so here while we're
    // supposed to be delaying.  It would be a shame to spend this time sleeping,
    // when we could be doing useful work.
    handle_chunk_updates();

    // Count any time we spent updating Chunks toward the delay.
    delay_ms -= SDL_GetTicks() - begin_ticks;

    if ( delay_ms >= 0 )
    {
        // FIXME: Commenting this out makes things MUCH smoother (at least with VSYNC enabled).
        // SDL_Delay( delay_ms );
    }
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

    window_.reshape_window();

    Camera camera( player_.get_eye_position(), player_.get_pitch(), player_.get_yaw(), window_.get_draw_distance() );

#ifdef DEBUG_COLLISIONS
    renderer_.render( window_, camera, world_, player_ );
#else
    renderer_.render( window_, camera, world_ );
#endif

    gui_.set_engine_chunk_stats( renderer_.get_num_chunks_drawn(), world_.get_chunks().size(), renderer_.get_num_triangles_drawn() );
    gui_.set_current_material( get_block_material_attributes( player_.get_material_selection() ).name_ );
    gui_.render();

    SDL_GL_SwapBuffers();
}
