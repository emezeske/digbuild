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

#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/foreach.hpp>

#include "random.h"
#include "world_generator.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

const unsigned SEA_LEVEL = 128;

Block& get_block( ChunkSPV& chunks, const Vector2i& column_position, const unsigned x, const unsigned z, const unsigned height )
{
    const unsigned chunk_index = height / Chunk::SIZE_Y;

    while ( chunk_index >= chunks.size() )
    {
        unsigned y = 0;

        if ( !chunks.empty() )
        {
            y = chunks.back()->get_position()[1] + Chunk::SIZE_Y;
        }

        ChunkSP new_chunk( new Chunk( Vector3i( column_position[0], y, column_position[1] ) ) );
        chunks.push_back( new_chunk );
    }

    return chunks[chunk_index]->get_block( Vector3i( x, height % Chunk::SIZE_Y, z ) );
}

void generate_chunk_column(
    ChunkSPV& chunks,
    const RegionFeatures& features,
    const Vector2i& region_position,
    const Vector2i& column_position,
    ChunkHeightmap heights
)
{
    for ( int x = 0; x < Chunk::SIZE_X; ++x )
    {
        for ( int z = 0; z < Chunk::SIZE_Z; ++z )
        {
            const Vector2i relative_position = column_position - region_position + Vector2i( x, z );

            const Scalar fundamental_height =
                features.get_fundamental_patch().interpolate( vector_cast<Scalar>( relative_position ) / Scalar( WorldGenerator::REGION_SIZE ) );

            const BicubicPatch octave_patch = features.get_octave_patch( relative_position / int( RegionFeatures::BICUBIC_OCTAVE_EDGE ) );

            const Vector2f octave_position(
                Scalar( relative_position[0] % RegionFeatures::BICUBIC_OCTAVE_EDGE ) / RegionFeatures::BICUBIC_OCTAVE_EDGE,
                Scalar( relative_position[1] % RegionFeatures::BICUBIC_OCTAVE_EDGE ) / RegionFeatures::BICUBIC_OCTAVE_EDGE
            );

            const Scalar
                // NOTE: Remove the abs(), 32.0f, and negation here to undo the ridge experiment.
                octave_height = abs( octave_patch.interpolate( octave_position ) ),
                total_height = 32.0f + fundamental_height - octave_height;

            const std::pair<BlockMaterial, Scalar> layers[] = 
            {
                std::make_pair( BLOCK_MATERIAL_LAVA,   1.0f                             ),
                std::make_pair( BLOCK_MATERIAL_BEDROCK, 20.0f + ( total_height ) * 0.25f ),
                std::make_pair( BLOCK_MATERIAL_STONE,   52.0f + ( total_height ) * 1.00f ),
                std::make_pair( BLOCK_MATERIAL_CLAY,    58.0f + ( total_height ) * 1.00f ),
                std::make_pair( BLOCK_MATERIAL_DIRT,    62.0f + ( total_height ) * 1.00f ),
                std::make_pair( BLOCK_MATERIAL_GRASS,   63.0f + ( total_height ) * 1.00f )
            };

            const unsigned num_layers = sizeof( layers ) / sizeof( std::pair<Scalar, BlockMaterial> );
            unsigned bottom = 0;

            for ( unsigned i = 0; i < num_layers; ++i )
            {
                const BlockMaterial material = layers[i].first;
                const Scalar height = std::max( layers[i].second, Scalar( bottom + 1 ) );
                const unsigned top = unsigned( roundf( height ) );

                for ( unsigned y = bottom; y <= top; ++y )
                {
                    Block& block = get_block( chunks, column_position, x, z, y );

                    // TODO: Ensure that the components of this vector are clamped (or repeated) to [0.0,1.0].
                    const Vector3f box_position(
                        Scalar( relative_position[0] ) / Scalar( RegionFeatures::TRILINEAR_BOX_SIZE[0] ),
                        Scalar( y                    ) / Scalar( RegionFeatures::TRILINEAR_BOX_SIZE[1] ),
                        Scalar( relative_position[1] ) / Scalar( RegionFeatures::TRILINEAR_BOX_SIZE[2] )
                    );

                    if ( material != BLOCK_MATERIAL_LAVA )
                    {
                        const Scalar
                            densityA = features.get_box( 0 ).interpolate( box_position ),
                            densityB = features.get_box( 1 ).interpolate( box_position );

                        if ( densityA > 0.45 && densityA < 0.55 && densityB > 0.45 && densityB < 0.55 )
                        {
                            block.set_material( BLOCK_MATERIAL_AIR );
                        }
                        else block.set_material( material );
                    }
                    else
                    {
                        block.set_material( BLOCK_MATERIAL_LAVA );
                        BlockDataFlowable( block ).make_source();
                    }
                }

                bottom = top;
            }

            // TODO: Separate this out into another function.
            const unsigned height = std::max( bottom, SEA_LEVEL );
            heights[x][z] = height;

            for ( unsigned y = height; y != 0; --y )
            {
                Block& block = get_block( chunks, column_position, x, z, y );

                if ( block.get_material() == BLOCK_MATERIAL_AIR )
                {
                    if ( y <= SEA_LEVEL )
                    {
                        block.set_material( BLOCK_MATERIAL_WATER );
                        BlockDataFlowable( block ).make_source();
                    }
                }
                else if ( block.get_material() == BLOCK_MATERIAL_GRASS ||
                          block.get_material() == BLOCK_MATERIAL_DIRT )
                {
                    if ( y <= SEA_LEVEL )
                    {
                        block.set_material( BLOCK_MATERIAL_MUD );
                    }
                }
                else break;
            }
        }
    }
}

void populate_trees(
    ChunkSPV& chunks,
    const uint64_t world_seed,
    const Vector2i& column_position,
    const ChunkHeightmap heights
)
{
    const int
        MIN_TREE_RADIUS = 3,
        MAX_TREE_RADIUS = 5,
        MIN_TREE_HEIGHT = 8,
        MAX_TREE_HEIGHT = 24,
        TREES_PER_CHUNK = 1;

    boost::rand48 tree_generator( get_seed_for_coordinates( world_seed, column_position ) );

    boost::uniform_int<> tree_x_distribution( MAX_TREE_RADIUS, Chunk::SIZE_X - MAX_TREE_RADIUS - 1 );
    boost::variate_generator<boost::rand48&, boost::uniform_int<> >
        tree_x_random( tree_generator, tree_x_distribution );

    boost::uniform_int<> tree_z_distribution( MAX_TREE_RADIUS, Chunk::SIZE_Z - MAX_TREE_RADIUS - 1 );
    boost::variate_generator<boost::rand48&, boost::uniform_int<> >
        tree_z_random( tree_generator, tree_z_distribution );

    boost::uniform_int<> tree_height_distribution( MIN_TREE_HEIGHT, MAX_TREE_HEIGHT );
    boost::variate_generator<boost::rand48&, boost::uniform_int<> >
        tree_height_random( tree_generator, tree_height_distribution );

    boost::uniform_int<> tree_radius_distribution( MIN_TREE_RADIUS, MAX_TREE_RADIUS );
    boost::variate_generator<boost::rand48&, boost::uniform_int<> >
        tree_radius_random( tree_generator, tree_radius_distribution );

    for ( int i = 0; i < TREES_PER_CHUNK; ++i )
    {
        const int
            x = tree_x_random(),
            z = tree_z_random(),
            height = tree_height_random(),
            radius = tree_radius_random();

        const int bottom = heights[x][z];
        Block& bottom_block = get_block( chunks, column_position, x, z, bottom );

        if ( bottom_block.get_material() == BLOCK_MATERIAL_GRASS )
        {
            for ( int y = 1; y < height; ++y )
            {
                Block& trunk_block = get_block( chunks, column_position, x, z, bottom + y );
                trunk_block.set_material( BLOCK_MATERIAL_TREE_TRUNK );

                const int leaf_height = y - ( height - radius - 1 );

                if ( leaf_height >= 0 )
                {
                    for ( int u = -radius + leaf_height; u <= radius - leaf_height; ++u )
                    {
                        for ( int v = -radius + leaf_height; v <= radius - leaf_height; ++v )
                        {
                            if ( u != 0 || v != 0 )
                            {
                                Block& leaf_block = get_block( chunks, column_position, x + u, z + v, bottom + y );

                                if ( leaf_block.get_material() == BLOCK_MATERIAL_AIR )
                                {
                                    leaf_block.set_material( BLOCK_MATERIAL_TREE_LEAF );
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void generate_chunk_concurrently( 
    ChunkSPV& chunks,
    const uint64_t world_seed,
    const RegionFeatures& features,
    const Vector2i& region_position,
    const Vector2i& column_position
)
{
    ChunkSPV column_chunks;
    ChunkHeightmap heights;
    generate_chunk_column( column_chunks, features, region_position, column_position, heights );
    populate_trees( column_chunks, world_seed, column_position, heights );

    static boost::mutex lock;
    boost::lock_guard<boost::mutex> guard( lock );
    chunks.insert( chunks.end(), column_chunks.begin(), column_chunks.end() );
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for WorldGenerator:
//////////////////////////////////////////////////////////////////////////////////

const int WorldGenerator::REGION_SIZE;

const Vector2i
    WorldGenerator::CHUNKS_PER_REGION_EDGE( REGION_SIZE / Chunk::SIZE_X, REGION_SIZE / Chunk::SIZE_Z );

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for WorldGenerator:
//////////////////////////////////////////////////////////////////////////////////

WorldGenerator::WorldGenerator( const uint64_t world_seed ) :
    world_seed_( world_seed )
{
}

ChunkSPV WorldGenerator::generate_region( const Vector2i& region_position, boost::threadpool::pool worker_pool )
{
    // TODO: The region features here are static for now, but eventually they should be randomized
    //       depending on the position of the region itself.

    const BicubicPatchCornerFeatures fundamental_corner_features(
        Vector2f( 0.0f, 128.0f ),
        Vector2f( -64.0f, 64.0f ),
        Vector2f( -64.0f, 64.0f ),
        Vector2f( -64.0f, 64.0f )
    );

    const BicubicPatchFeatures fundamental_features(
        fundamental_corner_features,
        fundamental_corner_features,
        fundamental_corner_features,
        fundamental_corner_features
    );

    const BicubicPatchCornerFeatures octave_corner_features(
        Vector2f( -32.0f, 32.0f ),
        Vector2f( -64.0f, 64.0f ),
        Vector2f( -64.0f, 64.0f ),
        Vector2f( -64.0f, 64.0f )
    );

    const BicubicPatchFeatures octave_features(
        octave_corner_features,
        octave_corner_features,
        octave_corner_features,
        octave_corner_features
    );

    RegionFeatures region_features( world_seed_, region_position, fundamental_features, octave_features );
    ChunkSPV chunks;

    for ( int x = 0; x < CHUNKS_PER_REGION_EDGE[0]; ++x )
    {
        for ( int z = 0; z < CHUNKS_PER_REGION_EDGE[1]; ++z )
        {
            const Vector2i column_position =
                region_position + Vector2i( x * Chunk::SIZE_X, z * Chunk::SIZE_Z );

            worker_pool.schedule(
                boost::bind(
                    &generate_chunk_concurrently,
                    boost::ref( chunks ),
                    world_seed_,
                    boost::ref( region_features ),
                    boost::ref( region_position ),
                    column_position
                )
            );
        }
    }

    worker_pool.wait();

    return chunks;
}

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for RegionFeatures:
//////////////////////////////////////////////////////////////////////////////////

const Vector3i
    RegionFeatures::TRILINEAR_BOX_SIZE( WorldGenerator::REGION_SIZE, TRILINEAR_BOX_HEIGHT, WorldGenerator::REGION_SIZE );

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for RegionFeatures:
//////////////////////////////////////////////////////////////////////////////////

RegionFeatures::RegionFeatures(
    const uint64_t world_seed,
    const Vector2i& region_position,
    const BicubicPatchFeatures& fundamental_features,
    const BicubicPatchFeatures& octave_features
) :
    fundamental_patch_( world_seed, region_position, Vector2i( WorldGenerator::REGION_SIZE, WorldGenerator::REGION_SIZE ), fundamental_features )
{
    // TODO: Somehow move the ultimate seeds being used here into a different space
    //       than those used for the fundamental patch.  Otherwise, the corners shared
    //       by the fundamental and octave patches will have the same attributes (boring).
    //
    //       Right now this is done by XORing the world seed with a big random number
    //       that I smashed out of the keyboard.  I'm not sure if this is good...

    const Vector2i octave_size( BICUBIC_OCTAVE_EDGE, BICUBIC_OCTAVE_EDGE );
    const uint64_t octave_seed = world_seed ^ 0xfea873529eaf;
    octave_patches_[0][0] = BicubicPatch( octave_seed, region_position + Vector2i( 0, 0 ), octave_size, octave_features );
    octave_patches_[0][1] = BicubicPatch( octave_seed, region_position + Vector2i( 0, BICUBIC_OCTAVE_EDGE ), octave_size, octave_features );
    octave_patches_[1][0] = BicubicPatch( octave_seed, region_position + Vector2i( BICUBIC_OCTAVE_EDGE, 0 ), octave_size, octave_features );
    octave_patches_[1][1] = BicubicPatch( octave_seed, region_position + Vector2i( BICUBIC_OCTAVE_EDGE, BICUBIC_OCTAVE_EDGE ), octave_size, octave_features );

    // The geometry generated by slicing up a single TrilinearBox by value ranges tends to be sheet-like,
    // which is not ideal for cave networks.  However, by taking the intersection of a value range in two
    // TrilinearBoxes, the resulting geometry is very stringy and tunnel-like.

    boxes_[0] = TrilinearBox(
        world_seed,
        Vector3i( region_position[0], 0, region_position[1] ),
        TRILINEAR_BOX_SIZE,
        32
    );

    // TODO: The world seed is being modified here to make sure this box is not identical
    //       to the first one.  Is this a good way to do so?
    boxes_[1] = TrilinearBox(
        world_seed ^ 0x313535f3235,
        Vector3i( region_position[0], 0, region_position[1] ),
        TRILINEAR_BOX_SIZE,
        32
    );
}
