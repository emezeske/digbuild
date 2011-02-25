#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

enum BlockMaterial
{
    BLOCK_MATERIAL_NONE = 0xffff,
    BLOCK_MATERIAL_GRASS = 0,
    BLOCK_MATERIAL_DIRT,
    BLOCK_MATERIAL_CLAY,
    BLOCK_MATERIAL_STONE,
    BLOCK_MATERIAL_BEDROCK
};

struct Block
{
    Block( const uint8_t bottom = 0, const uint8_t top = 1, const BlockMaterial _material = BLOCK_MATERIAL_GRASS );

    // NOTE: Don't give this type a deconstructor!  It's managed by boost::pool, which (as an optimisation)
    //       can drop allocated Blocks without calling their destructors.

    BlockMaterial material() const { return BlockMaterial( material_ ); }

    Block* next_;

    // TODO: Provide int accessors?

    uint8_t
        bottom_,
        top_;

    uint16_t
        material_;
};

#endif // BLOCK_H
