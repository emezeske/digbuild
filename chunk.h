#ifndef CHUNK_H
#define CHUNK_H

#include "block.h"
#include "math.h"

struct Chunk
{
    enum { BLOCKS_PER_EDGE = 16 };

    Chunk();

    void add_block_to_column( const Vector2i index, const Block& block );

    const BlockV& get_column( const Vector2i index ) const;

protected:

    // TODO: A future optimization here might be to use a pool-based allocator for
    //       BlockV to ensure locality of reference for each of these vectors.
    BlockV columns_[BLOCKS_PER_EDGE][BLOCKS_PER_EDGE];
};

#endif // CHUNK_H
