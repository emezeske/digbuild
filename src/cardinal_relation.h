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

#ifndef CARDINAL_RELATION_H
#define CARDINAL_RELATION_H

#include <stdexcept>

#include "math.h"

enum CardinalRelation
{
    CARDINAL_RELATION_ABOVE,
    CARDINAL_RELATION_BELOW,
    CARDINAL_RELATION_NORTH,
    CARDINAL_RELATION_SOUTH,
    CARDINAL_RELATION_EAST,
    CARDINAL_RELATION_WEST,
    NUM_CARDINAL_RELATIONS
};

#define FOREACH_CARDINAL_RELATION( iterator_name )\
    for ( CardinalRelation iterator_name = CARDINAL_RELATION_ABOVE;\
          iterator_name != NUM_CARDINAL_RELATIONS;\
          iterator_name = CardinalRelation( int( iterator_name ) + 1 ) )

inline Vector3i cardinal_relation_vector( const CardinalRelation relation )
{
    switch ( relation )
    {
        case CARDINAL_RELATION_ABOVE: return Vector3i(  0,  1,  0 );
        case CARDINAL_RELATION_BELOW: return Vector3i(  0, -1,  0 );
        case CARDINAL_RELATION_NORTH: return Vector3i(  0,  0,  1 );
        case CARDINAL_RELATION_SOUTH: return Vector3i(  0,  0, -1 );
        case CARDINAL_RELATION_EAST:  return Vector3i(  1,  0,  0 );
        case CARDINAL_RELATION_WEST:  return Vector3i( -1,  0,  0 );
        default: throw std::runtime_error( "Invalid cardinal relation." );
    }
}

inline CardinalRelation cardinal_relation_reverse( const CardinalRelation relation )
{
    switch ( relation )
    {
        case CARDINAL_RELATION_ABOVE: return CARDINAL_RELATION_BELOW;
        case CARDINAL_RELATION_BELOW: return CARDINAL_RELATION_ABOVE;
        case CARDINAL_RELATION_NORTH: return CARDINAL_RELATION_SOUTH;
        case CARDINAL_RELATION_SOUTH: return CARDINAL_RELATION_NORTH;
        case CARDINAL_RELATION_EAST:  return CARDINAL_RELATION_WEST;
        case CARDINAL_RELATION_WEST:  return CARDINAL_RELATION_EAST;
        default: throw std::runtime_error( "Invalid cardinal relation." );
    }
}

inline CardinalRelation cardinal_relation_tangent( const CardinalRelation relation )
{
    switch ( relation )
    {
        case CARDINAL_RELATION_ABOVE: return CARDINAL_RELATION_NORTH;
        case CARDINAL_RELATION_BELOW: return CARDINAL_RELATION_SOUTH;
        case CARDINAL_RELATION_NORTH: return CARDINAL_RELATION_EAST;
        case CARDINAL_RELATION_SOUTH: return CARDINAL_RELATION_WEST;
        case CARDINAL_RELATION_EAST:  return CARDINAL_RELATION_ABOVE;
        case CARDINAL_RELATION_WEST:  return CARDINAL_RELATION_BELOW;
        default: throw std::runtime_error( "Invalid cardinal relation." );
    }
}

#endif // CARDINAL_RELATION_H
