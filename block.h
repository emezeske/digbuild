#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

enum BlockMaterial
{
    BLOCK_MATERIAL_NONE = -1,
    BLOCK_MATERIAL_GRASS = 0,
    BLOCK_MATERIAL_DIRT,
    BLOCK_MATERIAL_CLAY,
    BLOCK_MATERIAL_STONE,
    BLOCK_MATERIAL_BEDROCK
};

struct Block
{
    typedef uint8_t HeightT;

    Block( const HeightT bottom = 0, const HeightT top = 1, const BlockMaterial _material = BLOCK_MATERIAL_GRASS );

    // NOTE: Don't give this type a deconstructor!  It's managed by boost::pool, which (as an optimisation)
    //       can drop allocated Blocks without calling their destructors.

    Block* next_;

    HeightT
        bottom_,
        top_;

    BlockMaterial
        material_;
};

#endif // BLOCK_H
