#include <assert.h>

#include <boost/integer_traits.hpp>

#include "block.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Block:
//////////////////////////////////////////////////////////////////////////////////

Block::Block( const HeightT bottom, const HeightT top, const BlockMaterial material ) :
    next_( 0 ),
    bottom_( bottom ),
    top_( top ),
    material_( material )
{
    assert( bottom < top );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for BlockFace:
//////////////////////////////////////////////////////////////////////////////////

BlockFace::BlockFace()
{
}

BlockFace::BlockFace( const Vector3f& normal, const BlockMaterial material ) :
    normal_( normal ),
    material_( material )
{
}
