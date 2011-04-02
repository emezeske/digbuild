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
    GameApplication( SDL_GL_Window &window, const unsigned fps_limit );
    ~GameApplication();

    void main_loop();

protected:

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

    void handle_chunk_updates();

    void do_one_step( const float step_time );
    void do_delay( const float delay_time );
    void render();

    bool run_;

    unsigned
        fps_limit_,
        fps_last_time_,
        fps_frame_count_;

    Scalar mouse_sensitivity_;

    SDL_GL_Window &window_;

    bool gui_focused_;

    Renderer renderer_;

    Player player_;

    World world_;

    Gui gui_;

    boost::threadpool::pool chunk_updater_;

    ChunkSet updated_chunks_;
};

#endif // GAME_APPLICATION_H
