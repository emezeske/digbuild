#ifndef SDL_GL_WINDOW
#define SDL_GL_WINDOW

#include <GL/glew.h>
#include <SDL/SDL.h>
#include <string>

struct SDL_GL_Window
{
    SDL_GL_Window( const int w, const int h, const int bpp, const Uint32 flags, const std::string &title );

    void reshape_window( const int w, const int h );
    void reshape_window();

    int width() const { return screen_width_; }
    int height() const { return screen_height_; }

    SDL_Surface* get_screen() const { return screen_; }

protected:

    void init_GL();

    SDL_Surface *screen_;

    int
        screen_width_,
        screen_height_,
        screen_bpp_,
        sdl_video_flags_;

    std::string title_;
};

#endif // SDL_GL_WINDOW
