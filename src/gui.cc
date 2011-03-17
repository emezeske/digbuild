#include <agar/core.h>
#include <agar/gui.h>
#include <agar/gui/cursors.h>
#include <agar/gui/sdl.h>

#include <assert.h>
#include <math.h>

#include <iostream>
#include <stdexcept>

#include "gui.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for GameApplication:
//////////////////////////////////////////////////////////////////////////////////

Gui::Gui( SDL_Surface* screen )
{
    if ( AG_InitCore( "Digbuild", 0 ) == -1 ||
         AG_InitVideoSDL( screen, AG_VIDEO_OVERLAY ) == -1 )
    {
        throw std::runtime_error( std::string( "Error intializing GUI: " ) + AG_GetError() );
    }

    AG_Window *window = AG_WindowNew( 0 );
    AG_WindowSetCaption( window, "Engine Stats" );

    fps_label_ = AG_LabelNewS( window, 0, "FPS: 0" );
    AG_ExpandHoriz( fps_label_ );
    AG_WidgetUpdate( fps_label_ );

    chunks_label_ = AG_LabelNewS( window, 0, "Chunks: 0/0" );
    AG_ExpandHoriz( chunks_label_ );
    AG_WidgetUpdate( chunks_label_ );

    triangles_label_ = AG_LabelNewS( window, 0, "Triangles: 0" );
    AG_ExpandHoriz( triangles_label_ );
    AG_WidgetUpdate( triangles_label_ );

    AG_WindowSetGeometry( window, 8, 8, 300, 128 );
    AG_WindowShow( window );
}

Gui::~Gui()
{
    AG_Destroy();
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

void Gui::set_engine_fps( const unsigned fps )
{
    AG_LabelText( fps_label_, "FPS: %d", fps );
}

void Gui::set_engine_chunk_stats( const unsigned chunks_drawn, const unsigned chunks_total, const unsigned triangles_drawn )
{
    AG_LabelText( chunks_label_, "Chunks: %d/%d", chunks_drawn, chunks_total );
    AG_LabelText( triangles_label_, "Triangles: %d", triangles_drawn );
}
