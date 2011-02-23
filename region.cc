#include <assert.h>

#include "region.h"
#include "bicubic_patch.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Region:
//////////////////////////////////////////////////////////////////////////////////

Region::Region( const uint64_t base_seed, const Vector2i position ) :
    position_( position )
{
    // TODO: Clean this up; lots of weird stuff here just for testing.

    const BicubicPatchCornerFeatures fundamental_corner_features
    (
        Vector2f( 0.0f, 128.0f ),
        Vector2f( -64.0f, 64.0f ),
        Vector2f( -64.0f, 64.0f ),
        Vector2f( -64.0f, 64.0f )
    );

    const BicubicPatchFeatures fundamental_features
    (
        fundamental_corner_features,
        fundamental_corner_features,
        fundamental_corner_features,
        fundamental_corner_features
    );

    const BicubicPatchCornerFeatures octave_corner_features
    (
        Vector2f( 0.0f, 32.0f ),
        Vector2f( -32.0f, 32.0f ),
        Vector2f( -32.0f, 32.0f ),
        Vector2f( -32.0f, 32.0f )
    );

    const BicubicPatchFeatures octave_features
    (
        octave_corner_features,
        octave_corner_features,
        octave_corner_features,
        octave_corner_features
    );

    const BicubicPatch fundamental_patch( base_seed, position_, Vector2i( BLOCKS_PER_EDGE, BLOCKS_PER_EDGE ), fundamental_features );

    const int octave_harmonic = 2;
    const int octave_edge = BLOCKS_PER_EDGE / octave_harmonic;
    const Vector2i octave_size( octave_edge, octave_edge );
    const BicubicPatch octave_patches[2][2] =
    {
        // TODO: Somehow move the ultimate seeds being used here into a different space
        //       than those used for the fundamental patch.  Otherwise, the corners shared
        //       by the fundamental and octave patches will have the same attributes (boring).
        //
        //       FIXME: using base_seed * 3547 for now, but I'm not sure if I like it
        {
            BicubicPatch( base_seed * 3547, position_ + Vector2i( 0, 0           ), octave_size, octave_features ),
            BicubicPatch( base_seed * 3547, position_ + Vector2i( 0, octave_edge ), octave_size, octave_features ),
        },
        {
            BicubicPatch( base_seed * 3547, position_ + Vector2i( octave_edge, 0           ), octave_size, octave_features ),
            BicubicPatch( base_seed * 3547, position_ + Vector2i( octave_edge, octave_edge ), octave_size, octave_features )
        }
    };

    for ( int x = 0; x < BLOCKS_PER_EDGE; ++x )
    {
        for ( int z = 0; z < BLOCKS_PER_EDGE; ++z )
        {
            const Scalar fundamental_height = fundamental_patch.interpolate( 
                static_cast<Scalar>( x ) / BLOCKS_PER_EDGE,
                static_cast<Scalar>( z ) / BLOCKS_PER_EDGE
            );

            const Scalar octave_height = octave_patches[x / octave_edge][z / octave_edge].interpolate(
                static_cast<Scalar>( x % octave_edge ) / octave_edge,
                static_cast<Scalar>( z % octave_edge ) / octave_edge
            );

            const Scalar height = fundamental_height + octave_height / octave_harmonic;

            Chunk& chunk = chunks_[x / Chunk::BLOCKS_PER_EDGE][z / Chunk::BLOCKS_PER_EDGE];
            const Vector2i column( x % Chunk::BLOCKS_PER_EDGE, z % Chunk::BLOCKS_PER_EDGE );

            std::vector<Scalar> layer_heights;
            layer_heights.push_back( 0 );
            layer_heights.push_back( 1.0f  + ( height + 0.0f  ) * 0.25f );
            layer_heights.push_back( 1.0f  + ( height + 16.0f ) * 0.45f );
            layer_heights.push_back( 32.0f + ( height + 16.0f ) * 0.65f );
            layer_heights.push_back( 32.0f + ( height + 16.0f ) * 0.75f );
            layer_heights.push_back( 32.0f + ( height + 16.0f ) * 1.00f );
            layer_heights.push_back( 32.0f + ( height + 16.0f ) * 1.00f );

            std::vector<uint8_t> clamped_layer_heights;

            for ( size_t i = 0; i < layer_heights.size(); ++i )
            {
                // Enforce a minimum layer thickness of 1.
                const Scalar minimum = ( i == 0 ? 0.0f : Scalar( clamped_layer_heights.back() ) + 1.0f );
                Scalar clamped = std::max( layer_heights[i], minimum );
                clamped = std::min( clamped, Scalar( std::numeric_limits<uint8_t>::max() ) );
                clamped_layer_heights.push_back( uint8_t( gmtl::Math::round( clamped ) ) );
            }

            const Block blocks[] =
            {
                Block( clamped_layer_heights[0], clamped_layer_heights[1], BLOCK_MATERIAL_BEDROCK ),
                Block( clamped_layer_heights[2], clamped_layer_heights[3], BLOCK_MATERIAL_STONE ),
                Block( clamped_layer_heights[3], clamped_layer_heights[4], BLOCK_MATERIAL_CLAY ),
                Block( clamped_layer_heights[4], clamped_layer_heights[5], BLOCK_MATERIAL_DIRT ),
                Block( clamped_layer_heights[5], clamped_layer_heights[6], BLOCK_MATERIAL_GRASS )
            };

            for ( size_t i = 0; i < sizeof( blocks ) / sizeof( Block ); ++i )
            {
                chunk.add_block_to_column( column, blocks[i] );
            }
        }
    }
}

const Chunk& Region::get_chunk( const Vector2i index ) const
{
    assert( index[0] >= 0 );
    assert( index[1] >= 0 );
    assert( index[0] < BLOCKS_PER_EDGE );
    assert( index[1] < BLOCKS_PER_EDGE );

    return chunks_[index[0]][index[1]];
}
