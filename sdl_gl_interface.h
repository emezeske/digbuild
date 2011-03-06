#ifndef SDL_GL_INTERFACE_H
#define SDL_GL_INTERFACE_H

#include <GL/glew.h>
#include <SDL/SDL.h>
#include <string>

struct SDL_GL_Window
{
    SDL_GL_Window( const int w, const int h, const int bpp, const Uint32 flags, const std::string &title );
    virtual ~SDL_GL_Window() {}

    virtual void create_window();
    virtual void reshape_window( const int w, const int h );

    virtual int width() const { return screen_width_; }
    virtual int height() const { return screen_height_; }

protected:

    virtual void init_GL();
    virtual void reshape_window();

    SDL_Surface *screen_;

    int
        screen_width_,
        screen_height_,
        screen_bpp_,
        sdl_video_flags_;

    std::string title_;
};

struct SDL_GL_Interface
{
    SDL_GL_Interface( SDL_GL_Window &initializer, const int fps_limit );
    virtual ~SDL_GL_Interface();

    virtual void toggle_fullscreen();
    virtual void main_loop();

protected:

    virtual void process_events();
    virtual void handle_event( SDL_Event& event );

    virtual void handle_key_down_event( const int key, const int mod ) {}
    virtual void handle_key_up_event( const int key, const int mod ) {}
    virtual void handle_mouse_motion_event( const int button, const int x, const int y, const int xrel, const int yrel ) {}
    virtual void handle_mouse_down_event( const int button, const int x, const int y, const int xrel, const int yrel ) {}
    virtual void handle_mouse_up_event( const int button, const int x, const int y, const int xrel, const int yrel ) {}
    virtual void do_one_step( float step_time ) = 0;
    virtual void render() = 0;

    bool run_;

    int fps_limit_;

    SDL_GL_Window &window_;
};

#endif // SDL_GL_INTERFACE_H
