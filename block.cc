#include <assert.h>

#include "block.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Block:
//////////////////////////////////////////////////////////////////////////////////

Block::Block( const uint8_t bottom, const uint8_t top, const BlockMaterial _material ) :
    bottom_( bottom ),
    top_( top ),
    material_( _material )
{
    assert( bottom < top );
    assert( _material >= boost::integer_traits<uint16_t>::const_min );
    assert( _material <= boost::integer_traits<uint16_t>::const_max );
}
