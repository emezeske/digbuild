#ifndef SDL_UTILITIES_H
#define SDL_UTILITIES_H

#include <SDL/SDL.h>
#include <string>

#include "log.h"

// These macros are tricky, but useful.  There is some evil here;  study the quoting issues well before modifying.
#define __SCOPE_TIMER_STR( s ) #s
#define __SCOPE_TIMER_XSTR( s ) __SCOPE_TIMER_STR( s )
#define __SCOPE_TIMER_POSITION ( std::string( __FILE__ ":" __SCOPE_TIMER_XSTR( __LINE__ ) ":" ) + __func__ )

#define SCOPE_TIMER_BEGIN { SDL_ScopeTimer __scope_timer( __SCOPE_TIMER_POSITION );
#define SCOPE_TIMER_END }

struct SDL_ScopeTimer
{
    SDL_ScopeTimer( const std::string& position ) : 
        position_( position ),
        began_at_( SDL_GetTicks() )
    {
    }

    ~SDL_ScopeTimer()
    {
        LOG( position_ << ": Timer took " << ( SDL_GetTicks() - began_at_ )<< " ms." );
    }

private:

    std::string position_;
    long began_at_;
};

#endif // SDL_UTILITIES_H
