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

#ifndef TIMER_H
#define TIMER_H

#ifndef _WINAPI
    #include <time.h>
    #include <stdexcept>
    
    struct HighResolutionTimer
    {
        HighResolutionTimer()
        {
            read_clock( last_time_ );
        }
    
        double get_seconds_elapsed()
        {
            read_clock( current_time_ );
    
            return
                double( current_time_.tv_sec - last_time_.tv_sec ) +
                double( current_time_.tv_nsec - last_time_.tv_nsec ) * 1e-9;
        }
    
        void reset()
        {
            last_time_ = current_time_;
        }
    
    protected:
    
        void read_clock( timespec& t )
        {
            const int clock_result = clock_gettime( CLOCK_MONOTONIC, &t );
    
            if ( clock_result == -1 )
            {
                throw std::runtime_error( "Unable to read clock" );
            }
        }
    
        timespec
            last_time_,
            current_time_;
    };
#else
    #error "TODO: Implement HighResolutionTimer in terms of Window's QueryPerformanceTimer."
#endif

#ifdef DEBUG_TIMERS
    #include <string>
    #include "log.h"
    #define SCOPE_TIMER_BEGIN( label ) { ScopeTimer __scope_timer( label );
    #define SCOPE_TIMER_END }
    
    struct ScopeTimer
    {
        ScopeTimer( const std::string& position ) : 
            position_( position )
        {
        }
    
        ~ScopeTimer()
        {
            const double ms = timer_.get_seconds_elapsed() * 1000.0;
            LOG( position_ << " took " <<  ms << " ms." );
        }
    
    protected:
    
        HighResolutionTimer timer_;
        std::string position_;
    };
#else
    #define SCOPE_TIMER_BEGIN( label )
    #define SCOPE_TIMER_END
#endif

#endif // TIMER_H
