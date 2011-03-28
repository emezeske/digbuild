#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include <assert.h>

#include <limits>
#include <vector>

#include "math.h"

enum BlockMaterial
{
    BLOCK_MATERIAL_AIR = 0,
    BLOCK_MATERIAL_GRASS,
    BLOCK_MATERIAL_DIRT,
    BLOCK_MATERIAL_CLAY,
    BLOCK_MATERIAL_STONE,
    BLOCK_MATERIAL_BEDROCK,
    BLOCK_MATERIAL_MAGMA,
    BLOCK_MATERIAL_TREE_TRUNK,
    BLOCK_MATERIAL_TREE_LEAF,
    BLOCK_MATERIAL_GLASS,
    BLOCK_MATERIAL_SIZE
};

struct BlockMaterialAttributes
{
    BlockMaterialAttributes( const bool translucent ) :
        translucent_( translucent )
    {
    }

    const bool translucent_;
};

inline const BlockMaterialAttributes& get_block_material_attributes( const BlockMaterial material )
{
    static const BlockMaterialAttributes attributes[BLOCK_MATERIAL_SIZE] =
    {
        // BLOCK_MATERIAL_AIR
        BlockMaterialAttributes( true ),
        // BLOCK_MATERIAL_GRASS
        BlockMaterialAttributes( false ),
        // BLOCK_MATERIAL_DIRT
        BlockMaterialAttributes( false ),
        // BLOCK_MATERIAL_CLAY
        BlockMaterialAttributes( false ),
        // BLOCK_MATERIAL_STONE
        BlockMaterialAttributes( false ),
        // BLOCK_MATERIAL_BEDROCK
        BlockMaterialAttributes( false ),
        // BLOCK_MATERIAL_MAGMA
        BlockMaterialAttributes( false ),
        // BLOCK_MATERIAL_TREE_TRUNK
        BlockMaterialAttributes( false ),
        // BLOCK_MATERIAL_TREE_LEAF
        BlockMaterialAttributes( false ),
        // BLOCK_MATERIAL_GLASS
        BlockMaterialAttributes( true )
    };

    assert( material >= 0 && material < BLOCK_MATERIAL_SIZE );
    return attributes[material];
}

struct Block
{
    static const int
        MIN_LIGHT_COMPONENT_LEVEL = 0x0,
        MAX_LIGHT_COMPONENT_LEVEL = 0xf;

    static const Vector3i
        MIN_LIGHT_LEVEL,
        MAX_LIGHT_LEVEL;

    static const Vector3f
        SIZE,
        HALFSIZE;

    Block() :
        material_( BLOCK_MATERIAL_AIR ),
        sunlight_source_( 0 ),
        visited_( 0 ),
        light_level_r_( 0 ),
        light_level_g_( 0 ),
        light_level_b_( 0 )
    {
    }

    void set_material( const BlockMaterial material )
    {
        assert( material >= std::numeric_limits<uint8_t>::min() );
        assert( material <= std::numeric_limits<uint8_t>::max() );
        material_ = material;
    }

    BlockMaterial get_material() const { return BlockMaterial( material_ ); }
    const BlockMaterialAttributes& get_material_attributes() const { return get_block_material_attributes( get_material() ); }

    void set_sunlight_source( const bool sunlight_source ) { sunlight_source_ = sunlight_source; }
    bool is_sunlight_source() const { return sunlight_source_; }

    void set_visited( const bool visited ) { visited_ = visited; }
    bool is_visited() const { return visited_; }

    void set_light_level( const Vector3i& light_level )
    {
        assert( light_level_valid( light_level ) );
        light_level_r_ = light_level[0];
        light_level_g_ = light_level[1];
        light_level_b_ = light_level[2];
    }

    Vector3i get_light_level() const
    {
        return Vector3i( light_level_r_, light_level_g_, light_level_b_ );
    }

    void set_sunlight_level( const int light_level )
    {
        assert( light_level_valid( light_level ) );
        light_level_s_ = light_level;
    }

    int get_sunlight_level() const
    {
        return light_level_s_;
    }

private:

    bool light_level_valid( const int light_level ) const
    {
        return ( light_level >= MIN_LIGHT_COMPONENT_LEVEL && light_level <= MAX_LIGHT_COMPONENT_LEVEL );
    }

    bool light_level_valid( const Vector3i& light_level ) const
    {
        for ( int i = 0; i < Vector3i::Size; ++i )
        {
             if ( !light_level_valid( light_level[i] ) )
             {
                 return false;
             }
        }

        return true;
    }

    uint8_t material_        : 8;
    uint8_t sunlight_source_ : 1;
    uint8_t visited_         : 1;
    uint8_t light_level_r_   : 4;
    uint8_t light_level_g_   : 4;
    uint8_t light_level_b_   : 4;
    uint8_t light_level_s_   : 4;
};

struct BlockFace
{
    enum { NUM_VERTICES = 4 };

    struct Vertex
    {
        Vertex()
        {
        }

        Vertex( const Vector3f& position, const Vector4f& lighting ) :
            position_( position ),
            lighting_( lighting )
        {
        }

        Vector3f position_;
        Vector4f lighting_;
    };

    BlockFace()
    {
    }

    BlockFace( const Vector3f& normal, const Vector3f& tangent, const BlockMaterial material ) :
        normal_( normal ),
        tangent_( tangent ),
        material_( material )
    {
    }

    Vertex vertices_[NUM_VERTICES];

    Vector3f
        normal_,
        tangent_;

    BlockMaterial material_;
};

typedef std::vector<BlockFace> BlockFaceV;

#endif // BLOCK_H
