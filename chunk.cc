#include <assert.h>

#include "chunk.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Chunk:
//////////////////////////////////////////////////////////////////////////////////

Chunk::Chunk()
{
}

void Chunk::add_block_to_column( const Vector2i index, const Block& block )
{
    assert( index[0] >= 0 );
    assert( index[1] >= 0 );
    assert( index[0] < BLOCKS_PER_EDGE );
    assert( index[1] < BLOCKS_PER_EDGE );

    // TODO: Check for block intersections.
    // TODO: Ensure height-sorted order.
    columns_[index[0]][index[1]].push_back( block );
}

const BlockV& Chunk::get_column( const Vector2i index ) const
{
    assert( index[0] >= 0 );
    assert( index[1] >= 0 );
    assert( index[0] < BLOCKS_PER_EDGE );
    assert( index[1] < BLOCKS_PER_EDGE );

    return columns_[index[0]][index[1]];
}
