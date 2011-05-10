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

#include "block.h"

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for Block:
//////////////////////////////////////////////////////////////////////////////////

const int
    Block::MIN_LIGHT_COMPONENT_LEVEL,
    Block::MAX_LIGHT_COMPONENT_LEVEL;

const Vector3i
    Block::MIN_LIGHT_LEVEL( MIN_LIGHT_COMPONENT_LEVEL, MIN_LIGHT_COMPONENT_LEVEL, MIN_LIGHT_COMPONENT_LEVEL ),
    Block::MAX_LIGHT_LEVEL( MAX_LIGHT_COMPONENT_LEVEL, MAX_LIGHT_COMPONENT_LEVEL, MAX_LIGHT_COMPONENT_LEVEL );

const Vector3f
    Block::SIZE( 1.0f, 1.0f, 1.0f ),
    Block::HALFSIZE( SIZE / 2.0f );

//////////////////////////////////////////////////////////////////////////////////
// Free function definitions:
//////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<( std::ostream& s, const Block& block )
{
    return
        s << "          material | " << block.get_material()       << std::endl
          << "    is_translucent | " << block.is_translucent()     << std::endl
          << "   is_light_source | " << block.is_light_source()    << std::endl
          << "is_color_saturated | " << block.is_color_saturated() << std::endl
          << "             color | " << block.get_color()          << std::endl
          << "    collision_mode | " << block.get_collision_mode() << std::endl
          << "is_sunlight_source | " << block.is_sunlight_source() << std::endl
          << "        is_visited | " << block.is_visited()         << std::endl
          << "       light_level | " << block.get_light_level()    << std::endl
          << "    sunlight_level | " << block.get_sunlight_level() << std::endl
          << "              data | " << block.get_data()           << std::endl;
}
