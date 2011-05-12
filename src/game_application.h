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

#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include <vector>

#include <boost/threadpool.hpp>

#include "sdl_gl_window.h"
#include "renderer.h"
#include "player.h"
#include "world.h"
#include "gui.h"
#include "player_input.h"

struct GameApplication
{
    GameApplication( SDL_GL_Window &window );
    ~GameApplication();

    void main_loop();
    void stop();
    void set_gui_focus( const bool focus );
    void reroute_input( const PlayerInputAction reroute_action );

protected:

    static const double FRAME_INTERVAL = 1.0 / 60.0;

    enum InputMode
    {
        INPUT_MODE_GUI,
        INPUT_MODE_PLAYER,
        INPUT_MODE_REROUTE
    };

    void process_events();
    void handle_event( const SDL_Event& event );
    bool handle_universal_event( const SDL_Event& event );
    void handle_gui_event( const SDL_Event& event );
    void handle_player_event( const SDL_Event& event );
    void handle_reroute_event( const SDL_Event& event );
    void handle_resize_event( const int w, const int h );
    void handle_mouse_motion_event( const int xrel, const int yrel );
    void handle_input_down_event( const PlayerInputBinding& binding );
    void handle_input_up_event( const PlayerInputBinding& binding );

    void toggle_fullscreen();

    void schedule_chunk_update();
    void handle_chunk_changes();

    void do_one_step( const float step_time );
    void render();

    bool run_;

    unsigned
        fps_last_time_,
        fps_frame_count_;

    Scalar mouse_sensitivity_;

    SDL_GL_Window& window_;

    Renderer renderer_;

    Player player_;

    World world_;

    Gui gui_;

    bool gui_focused_;

    PlayerInputRouter input_router_;

    InputMode input_mode_;

    PlayerInputAction reroute_action_;

    boost::threadpool::pool chunk_updater_;

    ChunkSet updated_chunks_;
};

#endif // GAME_APPLICATION_H
