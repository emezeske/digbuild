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

#include "player_input.h"

struct GameApplication;

struct Window
{
    Window( const std::string& name, const bool overlay, const int flags = DEFAULT_FLAGS );
    virtual ~Window();

    virtual AG_Window* get_window();

protected:

    static const int DEFAULT_FLAGS =
        AG_WINDOW_NORESIZE |
        AG_WINDOW_NOMAXIMIZE |
        AG_WINDOW_NOMINIMIZE;

    AG_Window* window_;
};

struct DebugInfoWindow : public Window
{
    DebugInfoWindow();

    void set_engine_fps( const unsigned fps );
    void set_engine_chunk_stats( const unsigned chunks_drawn, const unsigned chunks_total, const unsigned triangles_drawn );
    void set_current_material( const std::string& material );

protected:

    AG_Label* fps_label_;
    AG_Label* chunks_label_;
    AG_Label* triangles_label_;
    AG_Label* current_material_label_;
};

typedef boost::shared_ptr<DebugInfoWindow> DebugInfoWindowSP;

struct InputSettingsWindow : public Window
{
    InputSettingsWindow( GameApplication& application );

    void input_changed( const PlayerInputRouter& router );

protected:

    static void bind_input( AG_Event* event );
    static void reset_to_defaults( AG_Event* event );

    void add_input_button( AG_Box* parent, GameApplication& application, const std::string& label, const PlayerInputAction input_action );
    std::string get_binding_name( const PlayerInputRouter& router, const PlayerInputAction action ) const;

    AG_Button* input_buttons[NUM_PLAYER_INPUT_ACTIONS];
};

typedef boost::shared_ptr<InputSettingsWindow> InputSettingsWindowSP;

struct GraphicsSettingsWindow : public Window
{
    GraphicsSettingsWindow( GameApplication& application );

protected:

    static void todo( AG_Event* event );

    void add_label( AG_Box* parent, const std::string& label );

    int draw_distance_;
};

typedef boost::shared_ptr<GraphicsSettingsWindow> GraphicsSettingsWindowSP;

struct MainMenuWindow : public Window
{
    MainMenuWindow( GameApplication& application );

    DebugInfoWindow& get_debug_info_window();
    InputSettingsWindow& get_input_settings_window();

protected:
        
    static void resume( AG_Event* event );
    static void show_window( AG_Event* event );
    static void quit( AG_Event* event );

    AG_Label* fps_label_;

    DebugInfoWindowSP debug_info_window_;
    InputSettingsWindowSP input_settings_window_;
    GraphicsSettingsWindowSP graphics_settings_window_;
};

typedef boost::shared_ptr<MainMenuWindow> MainMenuWindowSP;

struct Gui
{
    Gui( GameApplication& application, SDL_Surface* screen );
    ~Gui();

    MainMenuWindow& get_main_menu_window();

    // "Stashing" the Gui hides all of the windows that are not marked as overlay
    // windows.  "Unstashing" it returns all stashed windows to their previous state.
    void stash();
    void unstash();

    void handle_event( const SDL_Event& sdl_event );
    void do_one_step( const float step_time );
    void render();

protected:

    MainMenuWindowSP main_menu_window_;

    bool stashed_;
};

#endif // GUI_H
