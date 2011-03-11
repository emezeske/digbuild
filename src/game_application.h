#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include <vector>

#include "sdl_gl_interface.h"
#include "renderer.h"
#include "camera.h"
#include "world.h"

struct GameApplication : public SDL_GL_Interface
{
    GameApplication( SDL_GL_Window &initializer, const int fps );
    virtual ~GameApplication();

protected:

    void initialize_gui();

    virtual void handle_resize_event( const int w, const int h );
    virtual void handle_key_down_event( const int key, const int mod );
    virtual void handle_key_up_event( const int key, const int mod );
    virtual void handle_mouse_motion_event( const int button, const int x, const int y, const int xrel, const int yrel );
    virtual void handle_mouse_down_event( const int button, const int x, const int y, const int xrel, const int yrel );
    virtual void do_one_step( const float step_time );
    virtual void render();

    bool show_gui_;

    Renderer renderer_;

    Camera camera_;

    World world_;
};

#endif // GAME_APPLICATION_H
