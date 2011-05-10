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

//////////////////////////////////////////////////////////////////////////////////
// Local namespace function definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

AG_Window* make_basic_window( const std::string& name )
{
    const int flags =
        AG_WINDOW_NORESIZE |
        AG_WINDOW_NOMAXIMIZE |
        AG_WINDOW_NOMINIMIZE;

    // TODO: Consider NOMOVE/NOCLOSE for the main menu?

    AG_Window *window = AG_WindowNewNamedS( flags, name.c_str() );
    AG_WindowSetCaptionS( window, name.c_str() );
    AG_WindowSetPadding( window, 8, 8, 8, 8 );
    return window;
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Static function definitions for MainMenuWindow:
//////////////////////////////////////////////////////////////////////////////////

void MainMenuWindow::show_window( AG_Event* event )
{
    AG_Window* window = static_cast<AG_Window*>( AG_PTR_NAMED( "window" ) );
    AG_WindowShow( window );
}

void MainMenuWindow::quit( AG_Event* /* event */ )
{
    // TODO
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for MainMenuWindow:
//////////////////////////////////////////////////////////////////////////////////

MainMenuWindow::MainMenuWindow( DebugInfoWindowSP debug_info_window ) :
    window_( make_basic_window( "Main Menu" ) ),
    debug_info_window_( debug_info_window )
{
    std::vector<AG_Button*> buttons;
    // TODO: Pass the correct window_ pointers instead of the main menu window_.
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Debug Info", &MainMenuWindow::show_window, "%p(window)", debug_info_window->get_window() ) );
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Keyboard/Mouse Settings", &MainMenuWindow::show_window, "%p(window)", window_ ) );
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Graphics Settings", &MainMenuWindow::show_window, "%p(window)", window_ ) );
    buttons.push_back( AG_ButtonNewFn( window_, 0, "Quit", &MainMenuWindow::quit, "%p(window)", this ) );

    BOOST_FOREACH( AG_Button* button, buttons )
    {
        AG_ExpandHoriz( button );
    }

    AG_WindowSetGeometry( window_, 0, 0, 300, 128 );
    AG_WindowSetPosition( window_, AG_WINDOW_MC, 0 );
    AG_WindowShow( window_ );
}

AG_Window* MainMenuWindow::get_window()
{
    return window_;
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for DebugInfoWindow:
//////////////////////////////////////////////////////////////////////////////////

DebugInfoWindow::DebugInfoWindow() :
    window_( make_basic_window( "Debug Info" ) )
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

AG_Window* DebugInfoWindow::get_window()
{
    return window_;
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
// Function definitions for Gui:
//////////////////////////////////////////////////////////////////////////////////

Gui::Gui( SDL_Surface* screen )
{
    if ( AG_InitCore( "DigBuild", 0 ) == -1 ||
         AG_InitVideoSDL( screen, AG_VIDEO_OVERLAY ) == -1 )
    {
        throw std::runtime_error( std::string( "Error intializing GUI: " ) + AG_GetError() );
    }

    debug_info_window_.reset( new DebugInfoWindow );
    main_menu_window_.reset( new MainMenuWindow( debug_info_window_ ) );
}

Gui::~Gui()
{
    AG_Destroy();
}

DebugInfoWindow& Gui::get_debug_info_window()
{
    return *debug_info_window_;
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
