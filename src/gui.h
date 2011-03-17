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

protected:

    AG_Label* fps_label_;
    AG_Label* chunks_label_;
    AG_Label* triangles_label_;
};

#endif // GUI_H
