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

#include <boost/shared_ptr.hpp>

// TODO: Consider abstracting out some of the get_window() stuff.

struct DebugInfoWindow
{
    DebugInfoWindow();

    AG_Window* get_window();

    void set_engine_fps( const unsigned fps );
    void set_engine_chunk_stats( const unsigned chunks_drawn, const unsigned chunks_total, const unsigned triangles_drawn );
    void set_current_material( const std::string& material );

protected:

    AG_Window* window_;
    AG_Label* fps_label_;
    AG_Label* chunks_label_;
    AG_Label* triangles_label_;
    AG_Label* current_material_label_;
};

typedef boost::shared_ptr<DebugInfoWindow> DebugInfoWindowSP;

struct MainMenuWindow
{
    MainMenuWindow( DebugInfoWindowSP debug_info_window );

    AG_Window* get_window();

protected:
        
    static void show_window( AG_Event* event );
    static void quit( AG_Event* event );

    AG_Window* window_;
    AG_Label* fps_label_;

    DebugInfoWindowSP debug_info_window_;
};

typedef boost::shared_ptr<MainMenuWindow> MainMenuWindowSP;

struct Gui
{
    Gui( SDL_Surface* screen );
    ~Gui();

    DebugInfoWindow& get_debug_info_window();

    void handle_event( SDL_Event& sdl_event );
    void do_one_step( const float step_time );
    void render();

protected:

    DebugInfoWindowSP debug_info_window_;
    MainMenuWindowSP main_menu_window_;
};

#endif // GUI_H
