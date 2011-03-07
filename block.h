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
    static const uint8_t FULLY_LIT = 0x0f;

    Block() :
        material_( BLOCK_MATERIAL_NONE ),
        lighting_( 0 )
    {
    }

    void set_material( const BlockMaterial material )
    {
        assert( material >= std::numeric_limits<uint8_t>::min() );
        assert( material <= std::numeric_limits<uint8_t>::max() );

        material_ = material;
    }

    BlockMaterial get_material() const { return BlockMaterial( material_ ); }

    void set_light_source( const bool source )
    {
        set_lighting_bit( source, LIGHT_SOURCE_BIT );
    }

    bool is_light_source() const { return lighting_ & LIGHT_SOURCE_BIT; }

    void set_sunlight_source( const bool source )
    {
        set_lighting_bit( source, SUNLIGHT_SOURCE_BIT );
    }

    bool is_sunlight_source() const { return lighting_ & SUNLIGHT_SOURCE_BIT; }

    void set_visited( const bool visited )
    {
        set_lighting_bit( visited, VISITED_BIT );
    }

    bool is_visited() const { return lighting_ & VISITED_BIT; }

    void set_light_level( const uint8_t level )
    {
        assert( !( level & ~LIGHT_LEVEL_MASK ) );
        lighting_ = ( lighting_ & ~LIGHT_LEVEL_MASK ) | level;
    }

    uint8_t get_light_level() const { return lighting_ & LIGHT_LEVEL_MASK; }

private:

    static const uint8_t
        LIGHT_SOURCE_BIT    = 1 << 7,
        SUNLIGHT_SOURCE_BIT = 1 << 6,
        VISITED_BIT         = 1 << 5,
        LIGHT_LEVEL_MASK    = 0x1f;

    void set_lighting_bit( const bool value, const uint8_t bit )
    {
        if ( value )
        {
            lighting_ |= bit;
        }
        else lighting_ &= ~bit;
    }

    uint8_t material_;

    uint8_t lighting_;
};

struct BlockFace
{
    enum { NUM_VERTICES = 4 };

    struct Vertex
    {
        Vertex()
        {
        }

        Vertex( const Vector3f& position, const Vector3f& lighting ) :
            position_( position ),
            lighting_( lighting )
        {
        }

        Vector3f position_;
        Vector3f lighting_;
    };

    BlockFace()
    {
    }

    BlockFace( const Vector3f& normal, const BlockMaterial material ) :
        normal_( normal ),
        material_( material )
    {
    }

    Vertex vertices_[NUM_VERTICES];

    Vector3f normal_;

    BlockMaterial material_;
};

typedef std::vector<BlockFace> BlockFaceV;

#endif // BLOCK_H
