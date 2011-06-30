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
#include <set>

#include "math.h"

typedef std::set<Vector2i, VectorLess<Vector2i> > ResolutionSet;

struct SDL_GL_Window
{
    SDL_GL_Window( const std::string& title );

    void reshape_window( const int w, const int h );
    void reshape_window();

    const Vector2i& get_resolution() const { return resolution_; }
    const ResolutionSet& get_available_resolutions() const { return available_resolutions_; };
    SDL_Surface* get_screen() const { return screen_; }
    float get_draw_distance() const { return draw_distance_; }
    void set_draw_distance( const float draw_distance ) { draw_distance_ = draw_distance; }

protected:

    static const int
        BITS_PER_PIXEL = 32,
        BYTES_PER_PIXEL = 4,
        VIDEO_MODE_FLAGS = SDL_OPENGL | SDL_FULLSCREEN;

    static const float
        DEFAULT_DRAW_DISTANCE = 250.0f;

    void prepare_resolution();
    void init_GL();

    SDL_Surface* screen_;

    Vector2i resolution_;

    ResolutionSet available_resolutions_;

    int
        screen_bpp_,
        sdl_video_flags_;

    float draw_distance_;

    std::string title_;
};

#endif // SDL_GL_WINDOW
