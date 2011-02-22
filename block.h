#ifndef BLOCK_H
#define BLOCK_H

#include <boost/integer_traits.hpp>

#include <vector>
#include <stdint.h>

enum BlockMaterial
{
    BLOCK_MATERIAL_GRASS,
    BLOCK_MATERIAL_DIRT,
    BLOCK_MATERIAL_CLAY,
    BLOCK_MATERIAL_STONE,
    BLOCK_MATERIAL_BEDROCK
};

struct Block
{
    Block( const uint8_t bottom = 0, const uint8_t top = 1, const BlockMaterial _material = BLOCK_MATERIAL_GRASS );

    BlockMaterial material() const { return BlockMaterial( material_ ); }

    // TODO: Provide int accessors?

    uint8_t
        bottom_,
        top_;

    uint16_t
        material_;
};

typedef std::vector<Block> BlockV;

#endif // BLOCK_H
