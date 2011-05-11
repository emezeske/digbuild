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

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/gui/cursors.h>
#include <agar/gui/sdl.h>

#include <assert.h>
#include <math.h>

#include <vector>
#include <iostream>
#include <stdexcept>

#include <boost/foreach.hpp>

#include "gui.h"
#include "game_application.h"

//////////////////////////////////////////////////////////////////////////////////
// Member function definitions for Window:
//////////////////////////////////////////////////////////////////////////////////

Window::Window( const std::string& name, const bool overlay, const int flags )
{
    window_ = AG_WindowNewNamedS( flags, name.c_str() );
    AG_WindowSetCaptionS( window_, name.c_str() );
    AG_WindowSetPadding( window_, 8, 8, 8, 8 );

    AG_SetInt( window_, "overlay", overlay );
    AG_SetInt( window_, "previously_visible", false );
}

Window::~Window()
{
}

AG_Window* Window::get_window()
{
    return window_;
}

//////////////////////////////////////////////////////////////////////////////////
// Member function definitions for DebugInfoWindow:
//////////////////////////////////////////////////////////////////////////////////

DebugInfoWindow::DebugInfoWindow() :
    Window( "Debug Info", true )
{
    fps_label_ = AG_LabelNewS( window_, 0, "FPS: 0" );
    AG_ExpandHoriz( fps_label_ );
    AG_WidgetUpdate( fps_label_ );

    chunks_label_ = AG_LabelNewS( window_, 0, "Chunks: 0/0" );
    AG_ExpandHoriz( chunks_label_ );
    AG_WidgetUpdate( chunks_label_ );

    triangles_label_ = AG_LabelNewS( window_, 0, "Triangles: 0" );
    AG_ExpandHoriz( triangles_label_ );
    AG_WidgetUpdate( triangles_label_ );

    current_material_label_ = AG_LabelNewS( window_, 0, "Current Material: None" );
    AG_ExpandHoriz( current_material_label_ );
    AG_WidgetUpdate( current_material_label_ );

    AG_WindowSetGeometry( window_, 0, 0, 300, 128 );
    AG_WindowSetPosition( window_, AG_WINDOW_TL, 0 );
    AG_WindowShow( window_ );
}

void DebugInfoWindow::set_engine_fps( const unsigned fps )
{
    AG_LabelText( fps_label_, "FPS: %d", fps );
}

void DebugInfoWindow::set_engine_chunk_stats( const unsigned chunks_drawn, const unsigned chunks_total, const unsigned triangles_drawn )
{
    AG_LabelText( chunks_label_, "Chunks: %d/%d", chunks_drawn, chunks_total );
    AG_LabelText( triangles_label_, "Triangles: %d", triangles_drawn );
}

void DebugInfoWindow::set_current_material( const std::string& current_material )
{
    AG_LabelText( current_material_label_, "Current Material: %s", current_material.c_str() );
}

//////////////////////////////////////////////////////////////////////////////////
// Static function definitions for InputSettingsWindow:
//////////////////////////////////////////////////////////////////////////////////

void InputSettingsWindow::set_button( AG_Event* event ) 
{
}

//////////////////////////////////////////////////////////////////////////////////
// Member function definitions for InputSettingsWindow:
//////////////////////////////////////////////////////////////////////////////////

InputSettingsWindow::InputSettingsWindow() :
    Window( "Input Settings", false )
{
    AG_WindowSetGeometry( window_, 0, 0, 300, 128 );

    // TODO: Lots of stuff.

    AG_Box* box1 = AG_BoxNewHoriz( window_, AG_BOX_HFILL | AG_BOX_HOMOGENOUS );
    AG_LabelNewS( box1, 0, "Move Forward" );
    AG_ButtonNewFn( box1, 0, "w", &InputSettingsWindow::set_button, "%p(todo)", 0 );

    AG_Box* box2 = AG_BoxNewHoriz( window_, AG_BOX_HFILL | AG_BOX_HOMOGENOUS );
    AG_LabelNewS( box2, 0, "Move Backward" );
    AG_ButtonNewFn( box2, 0, "s", &InputSettingsWindow::set_button, "%p(todo)", 0 );
}

//////////////////////////////////////////////////////////////////////////////////
// Member function definitions for GraphicsSettingsWindow:
//////////////////////////////////////////////////////////////////////////////////

GraphicsSettingsWindow::GraphicsSettingsWindow() :
    Window( "Graphics Settings", false )
{
    AG_WindowSetGeometry( window_, 0, 0, 300, 128 );

    // TODO: Lots of stuff.
}

//////////////////////////////////////////////////////////////////////////////////
// Static function definitions for MainMenuWindow:
//////////////////////////////////////////////////////////////////////////////////

void MainMenuWindow::resume( AG_Event* event )
{
    GameApplication* application = static_cast<GameApplication*>( AG_PTR_NAMED( "application" ) );
    application->toggle_gui_focus();
}

void MainMenuWindow::show_window( AG_Event* event )
{
    AG_Window* window = static_cast<AG_Window*>( AG_PTR_NAMED( "window" ) );
    AG_WindowShow( window );
}

void MainMenuWindow::quit( AG_Event* event )
{
    GameApplication* application = static_cast<GameApplication*>( AG_PTR_NAMED( "application" ) );
    application->stop();
}

//////////////////////////////////////////////////////////////////////////////////
// Member function definitions for MainMenuWindow:
//////////////////////////////////////////////////////////////////////////////////

MainMenuWindow::MainMenuWindow( GameApplication& application ) :
    Window( "Main Menu", false, Window::DEFAULT_FLAGS | AG_WINDOW_NOCLOSE | AG_WINDOW_NOMOVE ),
    debug_info_window_( new DebugInfoWindow ),
    input_settings_window_( new InputSettingsWindow ),
    graphics_settings_window_( new GraphicsSettingsWindow )
{
    std::vector<AG_Button*> buttons;
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Resume", &MainMenuWindow::resume, "%p(application)", &application ) );
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Debug Info", &MainMenuWindow::show_window, "%p(window)", debug_info_window_->get_window() ) );
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Input Settings", &MainMenuWindow::show_window, "%p(window)", input_settings_window_->get_window() ) );
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Graphics Settings", &MainMenuWindow::show_window, "%p(window)", graphics_settings_window_->get_window() ) );
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Quit", &MainMenuWindow::quit, "%p(application)", &application ) );

    BOOST_FOREACH( AG_Button* button, buttons )
    {
        AG_ExpandHoriz( button );
    }

    AG_WindowSetGeometry( window_, 0, 0, 300, 150 );
    AG_WindowSetPosition( window_, AG_WINDOW_MC, 0 );
    AG_WindowShow( window_ );
}

DebugInfoWindow& MainMenuWindow::get_debug_info_window()
{
    return *debug_info_window_;
}

//////////////////////////////////////////////////////////////////////////////////
// Member function definitions for Gui:
//////////////////////////////////////////////////////////////////////////////////

Gui::Gui( GameApplication& application, SDL_Surface* screen )
{
    if ( AG_InitCore( "DigBuild", 0 ) == -1 ||
         AG_InitVideoSDL( screen, AG_VIDEO_OVERLAY ) == -1 )
    {
        throw std::runtime_error( std::string( "Error intializing GUI: " ) + AG_GetError() );
    }

    main_menu_window_.reset( new MainMenuWindow( application ) );
}

Gui::~Gui()
{
    AG_Destroy();
}

MainMenuWindow& Gui::get_main_menu_window()
{
    return *main_menu_window_;
}

void Gui::handle_event( SDL_Event& sdl_event )
{
    assert( agDriverSw );

    AG_DriverEvent agar_event;
    AG_SDL_TranslateEvent( agDriverSw, &sdl_event, &agar_event );

    if ( AG_ProcessEvent( AGDRIVER( agDriverSw ), &agar_event ) == -1 )
    {
        throw std::runtime_error( std::string( "Error handling event: " ) + AG_GetError() );
    }
}

void Gui::do_one_step( const float step_time )
{
    if ( AG_TIMEOUTS_QUEUED() )
    {
        AG_ProcessTimeouts( static_cast<Uint32>( roundf( step_time * 1000.f ) ) );
    }
}

void Gui::render()
{
    // TODO: Research whether the AG_LockVFS() and AG_ObjectLock() calls here and
    //       in the following functions are actually necessary, given that the Gui
    //       is only used from a single thread.  If it really is necessary, abstract
    //       it out so that it's not duplicated.

    AG_LockVFS( &agDrivers );
    assert( agDriverSw );

    AG_BeginRendering( agDriverSw );

    AG_Window *window;
    AG_FOREACH_WINDOW( window, agDriverSw )
    {
        if ( window->visible )
        {
            AG_ObjectLock( window );
            AG_WindowDraw( window );
            AG_ObjectUnlock( window );
        }
    }

    AG_EndRendering( agDriverSw );

    AG_UnlockVFS( &agDrivers );
}

void Gui::stash()
{
    AG_LockVFS( &agDrivers );
    assert( agDriverSw );

    AG_Window *window;
    AG_FOREACH_WINDOW( window, agDriverSw )
    {
        AG_ObjectLock( window );

        if ( !AG_GetInt( window, "overlay" ) )
        {
            AG_SetInt( window, "previously_visible", AG_WindowIsVisible( window ) );
            AG_WindowHide( window );
        }

        AG_ObjectUnlock( window );
    }

    AG_UnlockVFS( &agDrivers );
}

void Gui::unstash()
{
    AG_LockVFS( &agDrivers );
    assert( agDriverSw );

    AG_Window *window;
    AG_FOREACH_WINDOW( window, agDriverSw )
    {
        AG_ObjectLock( window );

        if ( !AG_GetInt( window, "overlay" ) && AG_GetInt( window, "previously_visible" ) )
        {
            AG_WindowShow( window );
        }

        AG_ObjectUnlock( window );
    }

    AG_UnlockVFS( &agDrivers );
}
