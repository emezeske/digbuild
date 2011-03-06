#ifndef SDL_UTILITIES_H
#define SDL_UTILITIES_H

#include <SDL/SDL.h>
#include <string>

#include "log.h"

#define SCOPE_TIMER_BEGIN( label ) { SDL_ScopeTimer __scope_timer( label );
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
        LOG( position_ << " took " << ( SDL_GetTicks() - began_at_ ) << " ms." );
    }

private:

    std::string position_;
    long began_at_;
};

#endif // SDL_UTILITIES_H
