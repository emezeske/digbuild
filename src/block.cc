#include "block.h"

const Vector4i
    Block::MIN_LIGHT_LEVEL( 0x00, 0x00, 0x00, 0x00 ),
    Block::MAX_LIGHT_LEVEL( 0x0f, 0x0f, 0x0f, 0x0f );

const BlockMaterialAttributes& get_block_material_attributes( const BlockMaterial material )
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
