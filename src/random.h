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

#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

#include "math.h"

inline uint64_t get_seed_for_coordinates( const uint64_t base_seed, const Vector3i& position )
{
    return base_seed ^ ( position[0] * 91387 + position[1] * 75181 + position[2] * 40591 ); // TODO: Are these prime numbers good?
}

inline uint64_t get_seed_for_coordinates( const uint64_t base_seed, const Vector2i& position )
{
    return get_seed_for_coordinates( base_seed, Vector3i( position[0], position[1], 0 ) );
}

#endif // RANDOM_H
