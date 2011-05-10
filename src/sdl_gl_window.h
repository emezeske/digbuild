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

#ifndef SDL_GL_WINDOW
#define SDL_GL_WINDOW

#include <GL/glew.h>
#include <SDL/SDL.h>
#include <string>

#include "math.h"

struct SDL_GL_Window
{
    SDL_GL_Window( const int w, const int h, const int bpp, const Uint32 flags, const std::string &title );

    void reshape_window( const int w, const int h );
    void reshape_window();

    Vector2i get_screen_size() const { return screen_size_; }

    SDL_Surface* get_screen() const { return screen_; }

    float get_draw_distance() const { return draw_distance_; }
    void set_draw_distance( const float draw_distance ) { draw_distance_ = draw_distance; }

protected:

    void init_GL();

    SDL_Surface *screen_;

    Vector2i screen_size_;

    int
        screen_bpp_,
        sdl_video_flags_;

    float draw_distance_;

    std::string title_;
};

#endif // SDL_GL_WINDOW
