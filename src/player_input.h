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

#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H

#include <boost/bimap/bimap.hpp>

enum PlayerInputAction
{
    PLAYER_INPUT_ACTION_MOVE_FORWARD,
    PLAYER_INPUT_ACTION_MOVE_BACKWARD,
    PLAYER_INPUT_ACTION_MOVE_LEFT,
    PLAYER_INPUT_ACTION_MOVE_RIGHT,
    PLAYER_INPUT_ACTION_JUMP,
    PLAYER_INPUT_ACTION_WALK,
    PLAYER_INPUT_ACTION_SPRINT,
    PLAYER_INPUT_ACTION_NOCLIP,
    PLAYER_INPUT_ACTION_PRIMARY_FIRE,
    PLAYER_INPUT_ACTION_SECONDARY_FIRE,
    PLAYER_INPUT_ACTION_SELECT_NEXT,
    PLAYER_INPUT_ACTION_SELECT_PREVIOUS
};

struct PlayerInputBinding
{
    enum Source
    {
        SOURCE_MOUSE,
        SOURCE_KEYBOARD
    };

    PlayerInputBinding( const Source source, const int descriptor );

    bool operator<( const PlayerInputBinding& other ) const;

protected:

    // TODO: SDL_GetKeyName()

    Source source_;

    int descriptor_;
};

struct PlayerInputRouter
{
    PlayerInputRouter();

    void set_binding( const PlayerInputAction action, const PlayerInputBinding& binding );

    bool get_binding_for_action( const PlayerInputAction action, PlayerInputBinding& binding ) const;
    bool get_action_for_binding( const PlayerInputBinding& binding, PlayerInputAction& action ) const;

protected:
        
    typedef boost::bimaps::bimap<PlayerInputAction, PlayerInputBinding> InputMap;

    InputMap input_map_;
};

#endif // PLAYER_INPUT_H
