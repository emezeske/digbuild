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
