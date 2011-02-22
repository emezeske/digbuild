#include <assert.h>

#include "block.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Block:
//////////////////////////////////////////////////////////////////////////////////

Block::Block( const uint8_t position, const uint8_t height, const BlockMaterial _material ) :
    position_( position ),
    height_( height ),
    material_( _material )
{
    assert( _material >= boost::integer_traits<uint16_t>::const_min );
    assert( _material <= boost::integer_traits<uint16_t>::const_max );
}
