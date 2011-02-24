#include <assert.h>
#include <string.h>

#include "region.h"
#include "bicubic_patch.h"
#include "trilinear_box.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Region:
//////////////////////////////////////////////////////////////////////////////////

Region::Region( const uint64_t base_seed, const Vector2i position ) :
    position_( position ),
    block_pool_( sizeof( Block ) )
{
    memset( columns_, 0, sizeof( columns_ ) ); 

    // TODO: Clean this up; lots of weird stuff here just for testing.

    // const BicubicPatchCornerFeatures fundamental_corner_features
    // (
    //     Vector2f( 0.0f, 128.0f ),
    //     Vector2f( -64.0f, 64.0f ),
    //     Vector2f( -64.0f, 64.0f ),
    //     Vector2f( -64.0f, 64.0f )
    // );

    // const BicubicPatchFeatures fundamental_features
    // (
    //     fundamental_corner_features,
    //     fundamental_corner_features,
    //     fundamental_corner_features,
    //     fundamental_corner_features
    // );

    // const BicubicPatchCornerFeatures octave_corner_features
    // (
    //     Vector2f( 0.0f, 32.0f ),
    //     Vector2f( -32.0f, 32.0f ),
    //     Vector2f( -32.0f, 32.0f ),
    //     Vector2f( -32.0f, 32.0f )
    // );

    // const BicubicPatchFeatures octave_features
    // (
    //     octave_corner_features,
    //     octave_corner_features,
    //     octave_corner_features,
    //     octave_corner_features
    // );

    // const BicubicPatch fundamental_patch( base_seed, position_, Vector2i( REGION_SIZE, REGION_SIZE ), fundamental_features );

    // const int octave_harmonic = 2;
    // const int octave_edge = REGION_SIZE / octave_harmonic;
    // const Vector2i octave_size( octave_edge, octave_edge );
    // const BicubicPatch octave_patches[2][2] =
    // {
    //     // TODO: Somehow move the ultimate seeds being used here into a different space
    //     //       than those used for the fundamental patch.  Otherwise, the corners shared
    //     //       by the fundamental and octave patches will have the same attributes (boring).
    //     //
    //     //       FIXME: using base_seed * 3547 for now, but I'm not sure if I like it
    //     {
    //         BicubicPatch( base_seed * 3547, position_ + Vector2i( 0, 0           ), octave_size, octave_features ),
    //         BicubicPatch( base_seed * 3547, position_ + Vector2i( 0, octave_edge ), octave_size, octave_features ),
    //     },
    //     {
    //         BicubicPatch( base_seed * 3547, position_ + Vector2i( octave_edge, 0           ), octave_size, octave_features ),
    //         BicubicPatch( base_seed * 3547, position_ + Vector2i( octave_edge, octave_edge ), octave_size, octave_features )
    //     }
    // };

    // for ( int x = 0; x < REGION_SIZE; ++x )
    // {
    //     for ( int z = 0; z < REGION_SIZE; ++z )
    //     {
    //         const Scalar fundamental_height = fundamental_patch.interpolate( 
    //             static_cast<Scalar>( x ) / REGION_SIZE,
    //             static_cast<Scalar>( z ) / REGION_SIZE
    //         );

    //         const Scalar octave_height = octave_patches[x / octave_edge][z / octave_edge].interpolate(
    //             static_cast<Scalar>( x % octave_edge ) / octave_edge,
    //             static_cast<Scalar>( z % octave_edge ) / octave_edge
    //         );

    //         const Scalar height = fundamental_height + octave_height / octave_harmonic;

    //         std::vector<Scalar> layer_heights;
    //         layer_heights.reserve( 8 );
    //         layer_heights.push_back( 0 );
    //         layer_heights.push_back( 1.0f  + ( height + 0.0f  ) * 0.25f );
    //         layer_heights.push_back( 1.0f  + ( height + 16.0f ) * 0.45f );
    //         layer_heights.push_back( 32.0f + ( height + 16.0f ) * 0.65f );
    //         layer_heights.push_back( 32.0f + ( height + 16.0f ) * 0.75f );
    //         layer_heights.push_back( 32.0f + ( height + 16.0f ) * 1.00f );
    //         layer_heights.push_back( 32.0f + ( height + 16.0f ) * 1.00f );

    //         std::vector<uint8_t> clamped_layer_heights;
    //         clamped_layer_heights.reserve( 8 );

    //         for ( size_t i = 0; i < layer_heights.size(); ++i )
    //         {
    //             // Enforce a minimum layer thickness of 1.
    //             const Scalar minimum = ( i == 0 ? 0.0f : Scalar( clamped_layer_heights.back() ) + 1.0f );
    //             Scalar clamped = std::max( layer_heights[i], minimum );
    //             clamped = std::min( clamped, Scalar( std::numeric_limits<uint8_t>::max() ) );
    //             clamped_layer_heights.push_back( uint8_t( gmtl::Math::round( clamped ) ) );
    //         }

    //         Block* blocks[] =
    //         {
    //             new ( block_pool_.malloc() ) Block( clamped_layer_heights[0], clamped_layer_heights[1], BLOCK_MATERIAL_BEDROCK ),
    //             new ( block_pool_.malloc() ) Block( clamped_layer_heights[2], clamped_layer_heights[3], BLOCK_MATERIAL_STONE ),
    //             new ( block_pool_.malloc() ) Block( clamped_layer_heights[3], clamped_layer_heights[4], BLOCK_MATERIAL_CLAY ),
    //             new ( block_pool_.malloc() ) Block( clamped_layer_heights[4], clamped_layer_heights[5], BLOCK_MATERIAL_DIRT ),
    //             new ( block_pool_.malloc() ) Block( clamped_layer_heights[5], clamped_layer_heights[6], BLOCK_MATERIAL_GRASS )
    //         };

    //         for ( size_t i = 0; i < sizeof( blocks ) / sizeof( Block* ); ++i )
    //         {
    //             add_block_to_column( Vector2i( x, z ), blocks[i] );
    //         }
    //     }
    // }

    TrilinearBox box
    (
        base_seed,
        Vector3i( position_[0], 0, position_[1] ),
        Vector3i( REGION_SIZE, REGION_SIZE, REGION_SIZE ),
        16
    );

    TrilinearBox box2
    (
        base_seed ^ 0x313535f3235,
        Vector3i( position_[0], 0, position_[1] ),
        Vector3i( REGION_SIZE, REGION_SIZE, REGION_SIZE ),
        16
    );

    // TODO: Only check density function for blocks that are created by the heightmap function above.

    for ( int x = 0; x < REGION_SIZE; ++x )
    {
        for ( int y = 0; y < REGION_SIZE; ++y )
        {
            for ( int z = 0; z < REGION_SIZE; ++z )
            {
                const Scalar density = box.interpolate( 
                    static_cast<Scalar>( x ) / REGION_SIZE,
                    static_cast<Scalar>( y ) / REGION_SIZE,
                    static_cast<Scalar>( z ) / REGION_SIZE
                );

                const Scalar density2 = box2.interpolate( 
                    static_cast<Scalar>( x ) / REGION_SIZE,
                    static_cast<Scalar>( y ) / REGION_SIZE,
                    static_cast<Scalar>( z ) / REGION_SIZE
                );

                if ( density >= 0.45 && density <= 0.55 && density2 >= 0.45 && density2 <= 0.55 )
                    add_block_to_column( Vector2i( x, z ), new ( block_pool_.malloc() ) Block( y, y + 1, BLOCK_MATERIAL_GRASS ) );
                // else if ( density <= 0.30 )
                //     add_block_to_column( Vector2i( x, z ), new ( block_pool_.malloc() ) Block( y, y + 1, BLOCK_MATERIAL_STONE ) );
                // else if ( density <= 0.60 )
                //     add_block_to_column( Vector2i( x, z ), new ( block_pool_.malloc() ) Block( y, y + 1, BLOCK_MATERIAL_CLAY ) );
                // else if ( density <= 0.80 )
                //     add_block_to_column( Vector2i( x, z ), new ( block_pool_.malloc() ) Block( y, y + 1, BLOCK_MATERIAL_DIRT ) );
                // else
                //     add_block_to_column( Vector2i( x, z ), new ( block_pool_.malloc() ) Block( y, y + 1, BLOCK_MATERIAL_GRASS ) );
            }
        }
    }
}

void Region::add_block_to_column( const Vector2i index, Block* block )
{
    assert( index[0] >= 0 );
    assert( index[1] >= 0 );
    assert( index[0] < REGION_SIZE );
    assert( index[1] < REGION_SIZE );

    // TODO: Check for block intersections.
    // TODO: Ensure height-sorted order.

    Block* column = columns_[index[0]][index[1]];

    if ( column )
    {
        while ( column->next_ != 0 )
        {
            column = column->next_;
        }

        column->next_ = block;
    }
    else columns_[index[0]][index[1]] = block;
}
