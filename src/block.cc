#include "block.h"

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for Block:
//////////////////////////////////////////////////////////////////////////////////

const Vector3i
    Block::MIN_LIGHT_LEVEL( MIN_LIGHT_COMPONENT_LEVEL, MIN_LIGHT_COMPONENT_LEVEL, MIN_LIGHT_COMPONENT_LEVEL ),
    Block::MAX_LIGHT_LEVEL( MAX_LIGHT_COMPONENT_LEVEL, MAX_LIGHT_COMPONENT_LEVEL, MAX_LIGHT_COMPONENT_LEVEL );

const Vector3f
    Block::SIZE( 1.0f, 1.0f, 1.0f ),
    Block::HALFSIZE( SIZE / 2.0f );
