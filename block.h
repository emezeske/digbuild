#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

#include <vector>

#include "math.h"

enum BlockMaterial
{
    BLOCK_MATERIAL_NONE = -1,
    BLOCK_MATERIAL_GRASS = 0,
    BLOCK_MATERIAL_DIRT,
    BLOCK_MATERIAL_CLAY,
    BLOCK_MATERIAL_STONE,
    BLOCK_MATERIAL_BEDROCK,
    BLOCK_MATERIAL_MAGMA,
    BLOCK_MATERIAL_SIZE
};

struct Block
{
    typedef uint8_t HeightT;

    Block( const HeightT bottom, const HeightT top, const BlockMaterial material );

    // NOTE: Don't give this type a deconstructor!  It's managed by boost::pool, which (as an optimisation)
    //       can drop allocated Blocks without calling their destructors.

    Block* next_;

    HeightT
        bottom_,
        top_;

    BlockMaterial
        material_;
};

struct BlockFace
{
    enum { NUM_VERTICES = 4 };

    BlockFace();
    BlockFace( const Vector3f& normal, const BlockMaterial material );

    // TODO: Represent with origin, normal, and height?  That would make it
    //       easier for the renderer to break it down into squares.

    Vector3f vertices_[NUM_VERTICES];

    Vector3f normal_;

    BlockMaterial material_;
};

typedef std::vector<BlockFace> BlockFaceV;

#endif // BLOCK_H
