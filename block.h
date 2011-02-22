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
    BLOCK_MATERIAL_STONE
};

struct Block
{
    enum
    {
        MIN_POSITION = boost::integer_traits<uint8_t>::const_min,
        MIN_HEIGHT = 1,
        MAX_HEIGHT = boost::integer_traits<uint8_t>::const_max
    };

    Block( const uint8_t position = 0, const uint8_t height = 1, const BlockMaterial _material = BLOCK_MATERIAL_GRASS );

    BlockMaterial material() const { return BlockMaterial( material_ ); }

    // TODO: Provide int accessors?

    uint8_t
        position_,
        height_; // TODO: Maybe store bottom_ and top_ instead of position_ and height_?

    uint16_t
        material_;
};

typedef std::vector<Block> BlockV;

#endif // BLOCK_H
