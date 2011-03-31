#ifndef WORLD_GENERATOR
#define WORLD_GENERATOR

#include <boost/threadpool.hpp>

#include "bicubic_patch.h"
#include "trilinear_box.h"
#include "chunk.h"

struct RegionFeatures;

typedef unsigned ChunkHeightmap[Chunk::SIZE_X][Chunk::SIZE_Z];

struct WorldGenerator
{
    static const int REGION_SIZE = 128;
    static const Vector2i CHUNKS_PER_REGION_EDGE;

    WorldGenerator( const uint64_t world_seed );

    ChunkSPV generate_region( const Vector2i& region_position, boost::threadpool::pool worker_pool );

protected:

    const uint64_t world_seed_;
};

struct RegionFeatures
{
    enum
    {
        BICUBIC_OCTAVE_HARMONIC = 2,
        BICUBIC_OCTAVE_EDGE     = WorldGenerator::REGION_SIZE / BICUBIC_OCTAVE_HARMONIC,
        NUM_TRILINEAR_BOXES     = 2,
        TRILINEAR_BOX_HEIGHT    = 256
    };

    static const Vector3i TRILINEAR_BOX_SIZE;

    RegionFeatures(
        const uint64_t world_seed,
        const Vector2i& region_position,
        const BicubicPatchFeatures& fundamental_features,
        const BicubicPatchFeatures& octave_features
    );

    const BicubicPatch& get_fundamental_patch() const { return fundamental_patch_; }

    const BicubicPatch& get_octave_patch( const Vector2i& index ) const
    {
        assert( index[0] >= 0 );
        assert( index[1] >= 0 );
        assert( index[0] < BICUBIC_OCTAVE_HARMONIC );
        assert( index[1] < BICUBIC_OCTAVE_HARMONIC );
        return octave_patches_[index[0]][index[1]];
    }

    const TrilinearBox& get_box( const unsigned index ) const
    {
        assert( index < NUM_TRILINEAR_BOXES );
        return boxes_[index];
    }

private:

    BicubicPatch fundamental_patch_;
    BicubicPatch octave_patches_[BICUBIC_OCTAVE_HARMONIC][BICUBIC_OCTAVE_HARMONIC];
    TrilinearBox boxes_[NUM_TRILINEAR_BOXES];
};

#endif // WORLD_GENERATOR
