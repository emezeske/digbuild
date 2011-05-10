///////////////////////////////////////////////////////////////////////////
// Copyright 2011 Evan Mezeske.
//
// This file is part of Digbuild.
// 
// Digbuild is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.
// 
// Digbuild is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Digbuild.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////

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
    BLOCK_MATERIAL_MUD,
    BLOCK_MATERIAL_STONE,
    BLOCK_MATERIAL_BEDROCK,
    BLOCK_MATERIAL_LAVA,
    BLOCK_MATERIAL_TREE_TRUNK,
    BLOCK_MATERIAL_TREE_LEAF,
    BLOCK_MATERIAL_GLASS_CLEAR,
    BLOCK_MATERIAL_GLASS_RED,
    BLOCK_MATERIAL_GLASS_ORANGE,
    BLOCK_MATERIAL_GLASS_YELLOW,
    BLOCK_MATERIAL_GLASS_GREEN,
    BLOCK_MATERIAL_GLASS_BLUE,
    BLOCK_MATERIAL_GLASS_VIOLET,
    BLOCK_MATERIAL_WATER,
    NUM_BLOCK_MATERIALS
};

#define FOREACH_BLOCK_MATERIAL( iterator_name )\
    for ( BlockMaterial iterator_name = BLOCK_MATERIAL_AIR;\
          iterator_name != NUM_BLOCK_MATERIALS;\
          iterator_name = BlockMaterial( int( iterator_name ) + 1 ) )

enum BlockCollisionMode
{
    BLOCK_COLLISION_MODE_NONE,
    BLOCK_COLLISION_MODE_SOLID,
    BLOCK_COLLISION_MODE_FLUID
};

// TODO: It might be worth some testing to see if splitting these fields out into separate
//       lookup tables is more cache-friendly.
struct BlockMaterialAttributes
{
    BlockMaterialAttributes(
        const std::string& name,
        const std::string& texture_filename,
        const bool translucent,
        const bool is_light_source,
        const Vector3f& color,
        const BlockCollisionMode collision_mode
    ) :
        name_( name ),
        texture_filename_( texture_filename ),
        translucent_( translucent ),
        is_light_source_( is_light_source ),
        is_color_saturated_( color == Vector3f( 1.0f, 1.0f, 1.0f ) ),
        color_( color ),
        collision_mode_( collision_mode )
    {
    }

    const std::string
        name_,
        texture_filename_;

    const bool
        translucent_,
        is_light_source_,
        is_color_saturated_;

    // For translucent blocks, the color represents the filtering color.
    // For light source blocks, the color represents the light's color.
    const Vector3f color_;

    const BlockCollisionMode collision_mode_;
};

inline const BlockMaterialAttributes& get_block_material_attributes( const BlockMaterial material )
{
    static const BlockMaterialAttributes attributes[NUM_BLOCK_MATERIALS] =
    {
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //                       name,             texture,        translucent, light-source, color,                          collision-mode
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        BlockMaterialAttributes( "Air",            "wtf",          true,        false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_NONE ),
        BlockMaterialAttributes( "Grass",          "grass",        false,       false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Dirt",           "dirt",         false,       false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Clay",           "clay",         false,       false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Mud",            "mud",          false,       false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Stone",          "stone",        false,       false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Bedrock",        "bedrock",      false,       false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Lava",           "lava",         true ,       true,         Vector3f( 0.93f, 0.26f, 0.0f ), BLOCK_COLLISION_MODE_FLUID ),
        BlockMaterialAttributes( "Tree Trunk",     "tree-trunk",   false,       false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Tree Leaf",      "tree-leaf",    false,       false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Glass (Clear)",  "glass-clear",  true,        false,        Vector3f( 1.0f,  1.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Glass (Red)",    "glass-red",    true,        false,        Vector3f( 1.0f,  0.0f,  0.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Glass (Orange)", "glass-orange", true,        false,        Vector3f( 1.0f,  0.5f,  0.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Glass (Yellow)", "glass-yellow", true,        false,        Vector3f( 1.0f,  1.0f,  0.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Glass (Green)",  "glass-green",  true,        false,        Vector3f( 0.0f,  1.0f,  0.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Glass (Blue)",   "glass-blue",   true,        false,        Vector3f( 0.0f,  0.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Glass (Violet)", "glass-violet", true,        false,        Vector3f( 1.0f,  0.0f,  1.0f ), BLOCK_COLLISION_MODE_SOLID ),
        BlockMaterialAttributes( "Water",          "water",        true,        false,        Vector3f( 0.0f,  0.0f,  1.0f ), BLOCK_COLLISION_MODE_FLUID )
    };

    assert( material >= 0 && material < NUM_BLOCK_MATERIALS );
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
        sunlight_source_( false ),
        visited_( false ),
        light_level_r_( 0 ),
        light_level_g_( 0 ),
        light_level_b_( 0 ),
        sunlight_level_r_( 0 ),
        sunlight_level_g_( 0 ),
        sunlight_level_b_( 0 ),
        data_( 0 )
    {
    }

    void set_material( const BlockMaterial material )
    {
        assert( material >= std::numeric_limits<uint8_t>::min() );
        assert( material <= std::numeric_limits<uint8_t>::max() );
        material_ = material;
        data_ = 0;
    }

    BlockMaterial get_material() const { return BlockMaterial( material_ ); }
    const BlockMaterialAttributes& get_material_attributes() const { return get_block_material_attributes( get_material() ); }
    bool is_translucent() const { return get_material_attributes().translucent_; }
    bool is_light_source() const { return get_material_attributes().is_light_source_; }
    bool is_color_saturated() const { return get_material_attributes().is_color_saturated_; }
    const Vector3f& get_color() const { return get_material_attributes().color_; }
    BlockCollisionMode get_collision_mode() const { return get_material_attributes().collision_mode_; }

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

    void set_sunlight_level( const Vector3i& sunlight_level )
    {
        assert( light_level_valid( sunlight_level ) );
        sunlight_level_r_ = sunlight_level[0];
        sunlight_level_g_ = sunlight_level[1];
        sunlight_level_b_ = sunlight_level[2];
    }

    Vector3i get_sunlight_level() const
    {
        return Vector3i( sunlight_level_r_, sunlight_level_g_, sunlight_level_b_ );
    }

    void set_data( const uint8_t data )
    {
        data_ = data;
    }

    uint8_t get_data() const
    {
        return data_;
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

    uint8_t material_         : 8;
    uint8_t sunlight_source_  : 1;
    uint8_t visited_          : 1;
    uint8_t light_level_r_    : 4;
    uint8_t light_level_g_    : 4;
    uint8_t light_level_b_    : 4;
    uint8_t sunlight_level_r_ : 4;
    uint8_t sunlight_level_g_ : 4;
    uint8_t sunlight_level_b_ : 4;
    uint8_t data_             : 8; // Material-specific data.
};

// TODO: Move debug output operators to a common header file?
std::ostream& operator<<( std::ostream& s, const Block& block );

typedef std::vector<Block*> BlockV;

struct BlockFace
{
    enum { NUM_VERTICES = 4 };

    struct Vertex
    {
        Vertex()
        {
        }

        Vertex( const Vector3f& position, const Vector3f& lighting, Vector3f& sunlighting ) :
            position_( position ),
            lighting_( lighting ),
            sunlighting_( sunlighting )
        {
        }

        Vector3f position_;
        Vector3f lighting_;
        Vector3f sunlighting_;
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

struct BlockDataFlowable
{
    static const int MAX_FLOW_LEVEL = 0x08;

    BlockDataFlowable( Block& block ) : 
        block_( block )
    {
        assert( block_.get_material() == BLOCK_MATERIAL_WATER ||
                block_.get_material() == BLOCK_MATERIAL_LAVA );
    }

    // TODO: Right now there's nothing really special about a "source".
    void make_source()
    {
        block_.set_data( MAX_FLOW_LEVEL );
    }

    int get_flow_level() const
    {
        return block_.get_data();
    }

    void set_flow_level( const int flow_level )
    {
        assert( flow_level <= MAX_FLOW_LEVEL );
        assert( flow_level >= 0 );
        block_.set_data( flow_level );
    }

protected:

    Block& block_;
};

#endif // BLOCK_H
