#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include <assert.h>

#include <limits>
#include <vector>

#include "math.h"

enum BlockMaterial
{
    BLOCK_MATERIAL_NONE = 0xff,
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
    Block( const BlockMaterial material = BLOCK_MATERIAL_NONE ) :
        material_( material )
    {
        assert( material >= std::numeric_limits<uint8_t>::min() );
        assert( material <= std::numeric_limits<uint8_t>::max() );
    }

    void set_material( const BlockMaterial material )
    {
        assert( material >= std::numeric_limits<uint8_t>::min() );
        assert( material <= std::numeric_limits<uint8_t>::max() );

        material_ = material;
    }

    BlockMaterial get_material() const { return BlockMaterial( material_ ); }

private:

    uint8_t material_;
};

struct BlockFace
{
    enum { NUM_VERTICES = 4 };

    BlockFace()
    {
    }

    BlockFace( const Vector3f& normal, const BlockMaterial material ) :
        normal_( normal ),
        material_( material )
    {
    }

    Vector3f vertices_[NUM_VERTICES];

    Vector3f normal_;

    BlockMaterial material_;
};

typedef std::vector<BlockFace> BlockFaceV;

#endif // BLOCK_H
