#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include <vector>

#include "sdl_gl_interface.h"
#include "renderer.h"
#include "camera.h"
#include "region.h"

struct GameWindow : public SDL_GL_Window
{
    GameWindow( const int w, const int h, const int bpp, const Uint32 flags, const std::string &title );
    virtual ~GameWindow() {}

    virtual void create_window();

protected:

    virtual void init_GL();
};

struct GameApplication : public SDL_GL_Interface
{
    GameApplication( SDL_GL_Window &initializer, const int fps );
    virtual ~GameApplication();

protected:
    virtual void handle_key_down_event( const int key, const int mod );
    virtual void handle_key_up_event( const int key, const int mod );
    virtual void handle_mouse_motion_event( const int button, const int x, const int y, const int xrel, const int yrel );
    virtual void handle_mouse_down_event( const int button, const int x, const int y, const int xrel, const int yrel );
    virtual void do_one_step( float stepTime );
    virtual void render();

    Renderer renderer_;

    Camera camera_;

    RegionV regions_;
};

#endif // GAME_APPLICATION_H
