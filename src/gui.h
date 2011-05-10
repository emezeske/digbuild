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

#ifndef GUI_H
#define GUI_H

#include <SDL/SDL.h>
#include <agar/core.h>
#include <agar/gui.h>

struct Gui
{
    Gui( SDL_Surface* screen );
    ~Gui();

    void handle_event( SDL_Event& sdl_event );
    void do_one_step( const float step_time );
    void render();

    void set_engine_fps( const unsigned fps );
    void set_engine_chunk_stats( const unsigned chunks_drawn, const unsigned chunks_total, const unsigned triangles_drawn );
    void set_current_material( const std::string& material );

protected:

    AG_Label* fps_label_;
    AG_Label* chunks_label_;
    AG_Label* triangles_label_;
    AG_Label* current_material_label_;
};

#endif // GUI_H
