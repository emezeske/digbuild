#include <assert.h>

#include "chunk.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Chunk:
//////////////////////////////////////////////////////////////////////////////////

Chunk::Chunk()
{
}

void Chunk::add_block_to_column( const Vector2i column, const Block& block )
{
    assert( column[0] >= 0 );
    assert( column[1] >= 0 );
    assert( column[0] < BLOCKS_PER_EDGE );
    assert( column[1] < BLOCKS_PER_EDGE );

    // TODO: Check for block intersections.
    // TODO: Ensure height-sorted order.
    columns_[column[0]][column[1]].push_back( block );
}

const BlockV& Chunk::get_column( const Vector2i column ) const
{
    assert( column[0] >= 0 );
    assert( column[1] >= 0 );
    assert( column[0] < BLOCKS_PER_EDGE );
    assert( column[1] < BLOCKS_PER_EDGE );

    return columns_[column[0]][column[1]];
}
