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

GameApplication::GameApplication( SDL_GL_Window &window ) :
    run_( false ),
    fps_last_time_( 0 ),
    fps_frame_count_( 0 ),
    mouse_sensitivity_( 0.005f ),
    window_( window ),
    player_( Vector3f( 0.0f, 200.0f, 0.0f ), gmtl::Math::PI_OVER_2, gmtl::Math::PI_OVER_4 ),
    // world_( time( NULL ) * 91387 + SDL_GetTicks() * 75181 ),
    world_( 0xeaafa35aaa8eafdf ), // NOTE: Always use a constant for consistent performance measurements.
    input_mode_( INPUT_MODE_PLAYER ),
    gui_( *this, window_.get_screen() ),
    chunk_updater_( 1 )
{
    World::ChunkGuard chunk_guard( world_.get_chunk_lock() );

    SCOPE_TIMER_BEGIN( "Updating chunk VBOs" )

    BOOST_FOREACH( const ChunkMap::value_type& chunk_it, world_.get_chunks() )
    {
        renderer_.note_chunk_changes( *chunk_it.second );
    }

    SCOPE_TIMER_END

    gui_.stash();
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
        const double elapsed = frame_timer.get_seconds_elapsed();
        process_events();
        handle_chunk_changes();

        if ( elapsed >= FRAME_INTERVAL )
        {
            do_one_step( elapsed );
            schedule_chunk_update();
            render();
            frame_timer.reset();
        }
    }
}

void GameApplication::stop()
{
    run_ = false;
}

void GameApplication::set_gui_focus( const bool focus )
{
    if ( focus )
    {
        input_mode_ = INPUT_MODE_GUI;
        SDL_ShowCursor( SDL_ENABLE );
        SDL_WM_GrabInput( SDL_GRAB_OFF );
        gui_.unstash();
    }
    else
    {
        input_mode_ = INPUT_MODE_PLAYER;
        SDL_ShowCursor( SDL_DISABLE );
        SDL_WM_GrabInput( SDL_GRAB_ON );
        gui_.stash();
    }
}

void GameApplication::reroute_input( const PlayerInputAction reroute_action )
{
    input_mode_ = INPUT_MODE_REROUTE;
    reroute_action_ = reroute_action;
}

PlayerInputRouter& GameApplication::get_input_router()
{
    return input_router_;
}

void GameApplication::process_events()
{
    SDL_Event event;

    while( SDL_PollEvent( &event ) ) 
    {
        handle_event( event );
    }
}

void GameApplication::handle_event( const SDL_Event& event )
{
    if ( handle_universal_event( event ) )
    {
        return;
    }

    switch ( input_mode_ )
    {
        case INPUT_MODE_GUI:
            handle_gui_event( event );
            break;

        case INPUT_MODE_PLAYER:
            handle_player_event( event );
            break;

        case INPUT_MODE_REROUTE:
            handle_reroute_event( event );
            break;
    }
}

bool GameApplication::handle_universal_event( const SDL_Event& event )
{
    bool handled = true;

    switch ( event.type )
    {
        case SDL_KEYDOWN:
            if ( event.key.keysym.sym == SDLK_F11 )
            {
                toggle_fullscreen();
                return true;
            }
            break;

        case SDL_VIDEORESIZE:
            window_.reshape_window( event.resize.w, event.resize.h );
            gui_.handle_event( event );
            return true;

        case SDL_QUIT:
            stop();
            return true;

        default:
            break;
    }

    return false;
}

void GameApplication::handle_gui_event( const SDL_Event& event )
{
    if ( event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE )
    {
        set_gui_focus( false );
    }
    else gui_.handle_event( event );
}

void GameApplication::handle_player_event( const SDL_Event& event )
{
    switch ( event.type )
    {
        case SDL_KEYDOWN:
            if ( event.key.keysym.sym == SDLK_ESCAPE )
            {
                set_gui_focus( true );
            }
            else handle_input_down_event( PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, event.key.keysym.sym ) );
            break;

        case SDL_KEYUP:
            handle_input_up_event( PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, event.key.keysym.sym ) );
            break;

        case SDL_MOUSEMOTION:
            handle_mouse_motion_event( event.motion.xrel, event.motion.yrel );
            break;

        case SDL_MOUSEBUTTONDOWN:
            handle_input_down_event( PlayerInputBinding( PlayerInputBinding::SOURCE_MOUSE, event.button.button ) );
            break;

        case SDL_MOUSEBUTTONUP:
            handle_input_up_event( PlayerInputBinding( PlayerInputBinding::SOURCE_MOUSE, event.button.button ) );
            break;
    }
}

void GameApplication::handle_reroute_event( const SDL_Event& event )
{
    PlayerInputBinding binding;
    bool binding_set = false;

    switch ( event.type )
    {
        case SDL_KEYDOWN:
            if ( event.key.keysym.sym != SDLK_ESCAPE )
            {
                binding = PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, event.key.keysym.sym );
                binding_set = true;
            }
            else input_mode_ = INPUT_MODE_GUI;
            break;

        case SDL_MOUSEBUTTONDOWN:
            binding = PlayerInputBinding( PlayerInputBinding::SOURCE_MOUSE, event.button.button );
            binding_set = true;
            break;
    }

    if ( binding_set )
    {
        input_router_.set_binding( reroute_action_, binding );
        input_mode_ = INPUT_MODE_GUI;
        gui_.get_main_menu_window().get_input_settings_window().input_changed( input_router_ );
    }
}

void GameApplication::handle_mouse_motion_event( const int xrel, const int yrel )
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

void GameApplication::handle_input_down_event( const PlayerInputBinding& binding )
{
    PlayerInputAction action;

    if ( !input_router_.get_action_for_binding( binding, action ) )
    {
        return;
    }

    switch ( action )
    {
        case PLAYER_INPUT_ACTION_MOVE_FORWARD:
            player_.request_move_forward( true );
            break;

        case PLAYER_INPUT_ACTION_MOVE_BACKWARD:
            player_.request_move_backward( true );
            break;

        case PLAYER_INPUT_ACTION_MOVE_LEFT:
            player_.request_strafe_left( true );
            break;

        case PLAYER_INPUT_ACTION_MOVE_RIGHT:
            player_.request_strafe_right( true );
            break;

        case PLAYER_INPUT_ACTION_JUMP:
            player_.request_jump( true );
            break;

        case PLAYER_INPUT_ACTION_WALK:
            player_.request_walk( true );
            break;

        case PLAYER_INPUT_ACTION_SPRINT:
            player_.request_sprint( true );
            break;

        case PLAYER_INPUT_ACTION_NOCLIP:
            player_.toggle_noclip();
            break;

        case PLAYER_INPUT_ACTION_PRIMARY_FIRE:
            player_.request_primary_fire( true );
            break;

        case PLAYER_INPUT_ACTION_SECONDARY_FIRE:
            player_.request_secondary_fire( true );
            break;

        case PLAYER_INPUT_ACTION_SELECT_NEXT:
            player_.select_next_material();
            break;

        case PLAYER_INPUT_ACTION_SELECT_PREVIOUS:
            player_.select_previous_material();
            break;

        default:
            throw std::runtime_error( "Invalid player input action." );
    }
}

void GameApplication::handle_input_up_event( const PlayerInputBinding& binding )
{
    PlayerInputAction action;

    if ( !input_router_.get_action_for_binding( binding, action ) )
    {
        return;
    }

    switch ( action )
    {
        case PLAYER_INPUT_ACTION_MOVE_FORWARD:
            player_.request_move_forward( false );
            break;

        case PLAYER_INPUT_ACTION_MOVE_BACKWARD:
            player_.request_move_backward( false );
            break;

        case PLAYER_INPUT_ACTION_MOVE_LEFT:
            player_.request_strafe_left( false );
            break;

        case PLAYER_INPUT_ACTION_MOVE_RIGHT:
            player_.request_strafe_right( false );
            break;

        case PLAYER_INPUT_ACTION_JUMP:
            player_.request_jump( false );
            break;

        case PLAYER_INPUT_ACTION_WALK:
            player_.request_walk( false );
            break;

        case PLAYER_INPUT_ACTION_SPRINT:
            player_.request_sprint( false );
            break;

        case PLAYER_INPUT_ACTION_NOCLIP:
            break;

        case PLAYER_INPUT_ACTION_PRIMARY_FIRE:
            player_.request_primary_fire( false );
            break;

        case PLAYER_INPUT_ACTION_SECONDARY_FIRE:
            player_.request_secondary_fire( false );
            break;

        case PLAYER_INPUT_ACTION_SELECT_NEXT:
            break;

        case PLAYER_INPUT_ACTION_SELECT_PREVIOUS:
            break;

        default:
            throw std::runtime_error( "Invalid player input action." );
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

void GameApplication::schedule_chunk_update()
{
    World::ChunkGuard chunk_guard( world_.get_chunk_lock(), boost::defer_lock );

    boost::xtime not_long;
    not_long.sec = 0;
    not_long.nsec = 0;

    // If we can acquire the Chunk lock, AND the Chunk updater thread is not currently
    // executing an update, then it's okay to queue up a new update.

    if ( chunk_guard.try_lock() && chunk_updater_.wait( not_long ) )
    {
        updated_chunks_ = world_.get_updated_chunks();

        if ( world_.chunk_update_needed() )
        {
            chunk_updater_.schedule( boost::bind( &World::update_chunks, boost::ref( world_ ) ) );
        }
    }
}

void GameApplication::handle_chunk_changes()
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
    World::ChunkGuard chunk_guard( world_.get_chunk_lock() );

    player_.do_one_step( step_time, world_ );
    world_.do_one_step( step_time, player_.get_position() );
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
}

void GameApplication::render()
{
    DebugInfoWindow& debug_info_window = gui_.get_main_menu_window().get_debug_info_window();
    ++fps_frame_count_;
    const unsigned now = SDL_GetTicks();

    if ( fps_last_time_ + 1000 < now )
    {
        fps_last_time_ = now;
        debug_info_window.set_engine_fps( fps_frame_count_ );
        fps_frame_count_ = 0;
    }

    window_.reshape_window();

    Camera camera( player_.get_eye_position(), player_.get_pitch(), player_.get_yaw(), window_.get_draw_distance() );

#ifdef DEBUG_COLLISIONS
    renderer_.render( window_, camera, world_, player_ );
#else
    renderer_.render( window_, camera, world_ );
#endif

    debug_info_window.set_engine_chunk_stats( renderer_.get_num_chunks_drawn(), world_.get_chunks().size(), renderer_.get_num_triangles_drawn() );
    debug_info_window.set_current_material( get_block_material_attributes( player_.get_material_selection() ).name_ );

    gui_.render();

    SDL_GL_SwapBuffers();
}
