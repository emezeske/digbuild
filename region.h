#ifndef REGION_H
#define REGION_H

#include <GL/gl.h>
#include <vector>

#include "chunk.h"
#include "math.h"

struct Region
{
    enum
    {
        BLOCKS_PER_EDGE = 64,
        CHUNKS_PER_EDGE = BLOCKS_PER_EDGE / Chunk::BLOCKS_PER_EDGE
    };

    Region( const uint64_t world_seed, const Vector2i position );

    const Chunk& get_chunk( const Vector2i index ) const;

    Vector2i position_;

protected:

    Chunk chunks_[CHUNKS_PER_EDGE][CHUNKS_PER_EDGE];
};

typedef std::vector<Region> RegionV;

#endif // REGION_H
