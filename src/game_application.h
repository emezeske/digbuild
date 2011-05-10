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

struct GameApplication
{
    GameApplication( SDL_GL_Window &window );
    ~GameApplication();

    void main_loop();

protected:

    static const double FRAME_INTERVAL = 1.0 / 60.0;

    void process_events();
    void handle_event( SDL_Event& event );
    void handle_resize_event( const int w, const int h );
    void handle_key_down_event( const int key, const int mod );
    void handle_key_up_event( const int key, const int mod );
    void handle_mouse_motion_event( const int button, const int x, const int y, const int xrel, const int yrel );
    void handle_mouse_down_event( const int button, const int x, const int y, const int xrel, const int yrel );
    void handle_mouse_up_event( const int button, const int x, const int y, const int xrel, const int yrel );

    void toggle_fullscreen();
    void toggle_gui_focus();

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

    bool gui_focused_;

    Renderer renderer_;

    Player player_;

    World world_;

    Gui gui_;

    boost::threadpool::pool chunk_updater_;

    ChunkSet updated_chunks_;
};

#endif // GAME_APPLICATION_H
