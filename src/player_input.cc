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

#include <SDL/SDL.h>

#include "player_input.h"
#include "log.h"

//////////////////////////////////////////////////////////////////////////////////
// Member function definitions for PlayerInputBinding:
//////////////////////////////////////////////////////////////////////////////////

PlayerInputBinding::PlayerInputBinding( const Source source, const int descriptor ) :
    source_( source ),
    descriptor_( descriptor )
{
}

bool PlayerInputBinding::operator<( const PlayerInputBinding& other ) const
{
    if ( source_ < other.source_ )
    {
        return true;
    }

    if ( source_ == other.source_ && descriptor_ < other.descriptor_ )
    {
        return true;
    }

    return false;
}

std::string PlayerInputBinding::describe() const
{
    std::string description;

    switch ( source_ )
    {
        case SOURCE_MOUSE:
            description += "mouse ";
            switch ( descriptor_ )
            {
                case SDL_BUTTON_LEFT:      description += "left";       break;
                case SDL_BUTTON_MIDDLE:    description += "middle";     break;
                case SDL_BUTTON_RIGHT:     description += "right";      break;
                case SDL_BUTTON_WHEELUP:   description += "wheel down"; break;
                case SDL_BUTTON_WHEELDOWN: description += "wheel up";   break;
                default:
                    description += make_string() << descriptor_;
                    break;
            }
            break;

        case SOURCE_KEYBOARD:
            description += SDL_GetKeyName( static_cast<SDLKey>( descriptor_ ) );
            break;
    }

    return description;
}

//////////////////////////////////////////////////////////////////////////////////
// Member function definitions for PlayerInputRouter:
//////////////////////////////////////////////////////////////////////////////////

PlayerInputRouter::PlayerInputRouter()
{
    // TODO: Load the settings from an INI file. Save them too!

    reset_to_defaults();
}

void PlayerInputRouter::reset_to_defaults()
{
    set_binding( PLAYER_INPUT_ACTION_MOVE_FORWARD,    PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, SDLK_w ) );
    set_binding( PLAYER_INPUT_ACTION_MOVE_BACKWARD,   PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, SDLK_s ) );
    set_binding( PLAYER_INPUT_ACTION_MOVE_LEFT,       PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, SDLK_a ) );
    set_binding( PLAYER_INPUT_ACTION_MOVE_RIGHT,      PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, SDLK_d ) );
    set_binding( PLAYER_INPUT_ACTION_JUMP,            PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, SDLK_SPACE ) );
    set_binding( PLAYER_INPUT_ACTION_WALK,            PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, SDLK_LSHIFT ) );
    set_binding( PLAYER_INPUT_ACTION_SPRINT,          PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, SDLK_LCTRL ) );
    set_binding( PLAYER_INPUT_ACTION_NOCLIP,          PlayerInputBinding( PlayerInputBinding::SOURCE_KEYBOARD, SDLK_b ) );
    set_binding( PLAYER_INPUT_ACTION_PRIMARY_FIRE,    PlayerInputBinding( PlayerInputBinding::SOURCE_MOUSE,    SDL_BUTTON_LEFT ) );
    set_binding( PLAYER_INPUT_ACTION_SECONDARY_FIRE,  PlayerInputBinding( PlayerInputBinding::SOURCE_MOUSE,    SDL_BUTTON_RIGHT ) );
    set_binding( PLAYER_INPUT_ACTION_SELECT_NEXT,     PlayerInputBinding( PlayerInputBinding::SOURCE_MOUSE,    SDL_BUTTON_WHEELUP ) );
    set_binding( PLAYER_INPUT_ACTION_SELECT_PREVIOUS, PlayerInputBinding( PlayerInputBinding::SOURCE_MOUSE,    SDL_BUTTON_WHEELDOWN ) );
}

void PlayerInputRouter::set_binding( const PlayerInputAction action, const PlayerInputBinding& binding )
{
    // First, remove any existing actions for the specified binding.  This prevents the same
    // key from being bound to multiple actions.

    InputMap::right_iterator binding_it = input_map_.right.find( binding );

    if ( binding_it != input_map_.right.end() )
    {
        input_map_.right.erase( binding_it );
    }

    // Now we can map the binding to the action.

    InputMap::left_iterator action_it = input_map_.left.find( action );

    if ( action_it == input_map_.left.end() )
    {
        input_map_.insert( InputMap::value_type( action, binding ) );
    }
    else input_map_.left.replace_data( action_it, binding );
}

bool PlayerInputRouter::get_binding_for_action( const PlayerInputAction action, PlayerInputBinding& binding ) const
{
    InputMap::left_const_iterator input_it = input_map_.left.find( action );

    if ( input_it == input_map_.left.end() )
    {
        return false;
    }

    binding = input_it->second;
    return true;
}

bool PlayerInputRouter::get_action_for_binding( const PlayerInputBinding& binding, PlayerInputAction& action ) const
{
    InputMap::right_const_iterator input_it = input_map_.right.find( binding );

    if ( input_it == input_map_.right.end() )
    {
        return false;
    }

    action = input_it->second;
    return true;
}
