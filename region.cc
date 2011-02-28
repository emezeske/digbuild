#include <assert.h>
#include <string.h>

#include <stdexcept>
#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>

#include "region.h"
#include "bicubic_patch.h"
#include "trilinear_box.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

const Vector3f
    NORTH_NORMAL(  0,  0,  1 ),
    SOUTH_NORMAL(  0,  0, -1 ),
    EAST_NORMAL (  1,  0,  0 ),
    WEST_NORMAL ( -1,  0,  0 ),
    UP_NORMAL   (  0,  1,  0 ),
    DOWN_NORMAL (  0, -1,  0 );

const Region* find_region( const RegionMap& regions, const Vector2i& position )
{
    RegionMap::const_iterator it = regions.find( position );
    return ( it == regions.end() ? 0 : it->second.get() );
}

const Chunk* find_chunk( const Region& region, const Vector3i& index )
{
    ChunkMap::const_iterator it = region.chunks().find( index );
    return ( it == region.chunks().end() ? 0 : it->second.get() );
}

const Chunk* find_adjacent_chunk( const Region& region, const Region* neighbor_region, const Vector3i& neighbor_index )
{
    if ( neighbor_index[0] >= 0 &&
         neighbor_index[2] >= 0 &&
         neighbor_index[0] < Region::CHUNKS_PER_EDGE &&
         neighbor_index[2] < Region::CHUNKS_PER_EDGE )
    {
        return find_chunk( region, neighbor_index );
    }
    else if ( neighbor_region )
    {
        return find_chunk( *neighbor_region, Vector3i(
            ( Region::CHUNKS_PER_EDGE + neighbor_index[0] ) % Region::CHUNKS_PER_EDGE,
            neighbor_index[1],
            ( Region::CHUNKS_PER_EDGE + neighbor_index[2] ) % Region::CHUNKS_PER_EDGE
        ) );
    }
    
    return 0;
}

const Block* find_colinear_column( const Chunk* neighbor_chunk, const Vector2i& column_index )
{
    if ( neighbor_chunk )
    {
        return neighbor_chunk->get_column( column_index );
    }

    return 0;
}

const Block* find_adjacent_column( const Chunk& chunk, const Chunk* neighbor_chunk, const Vector2i& neighbor_index )
{
    if ( neighbor_index[0] >= 0 &&
         neighbor_index[1] >= 0 &&
         neighbor_index[0] < Chunk::CHUNK_SIZE &&
         neighbor_index[1] < Chunk::CHUNK_SIZE )
    {
        return chunk.get_column( neighbor_index );
    }
    else if ( neighbor_chunk )
    {
        return neighbor_chunk->get_column( Vector2i(
            ( Chunk::CHUNK_SIZE + neighbor_index[0] ) % Chunk::CHUNK_SIZE,
            ( Chunk::CHUNK_SIZE + neighbor_index[1] ) % Chunk::CHUNK_SIZE 
        ) );
    }
    
    return 0;
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Chunk:
//////////////////////////////////////////////////////////////////////////////////

Chunk::Chunk()
{
    memset( columns_, 0, sizeof( columns_ ) ); 
}

void Chunk::add_block_to_column( const Vector2i& index, Block* block )
{
    assert( index[0] >= 0 );
    assert( index[1] >= 0 );
    assert( index[0] < CHUNK_SIZE );
    assert( index[1] < CHUNK_SIZE );
    assert( block );
    assert( block->top_ <= CHUNK_SIZE );

    // TODO: Check for block intersections.
    // TODO: Enforce height-sorted order.

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

void Chunk::update_external_faces(
    const Vector3i& chunk_index,
    const Region& region,
    const RegionMap& regions
)
{
    external_faces_.clear();

    const Region* neighbor_regions[NEIGHBOR_RELATION_SIZE];
    neighbor_regions[NEIGHBOR_ABOVE] = 0;
    neighbor_regions[NEIGHBOR_BELOW] = 0;
    neighbor_regions[NEIGHBOR_NORTH] = find_region( regions, region.position() + Vector2i( 0, Region::REGION_SIZE ) );
    neighbor_regions[NEIGHBOR_SOUTH] = find_region( regions, region.position() + Vector2i( 0, -Region::REGION_SIZE ) ); 
    neighbor_regions[NEIGHBOR_EAST]  = find_region( regions, region.position() + Vector2i( Region::REGION_SIZE, 0 ) );
    neighbor_regions[NEIGHBOR_WEST]  = find_region( regions, region.position() + Vector2i( -Region::REGION_SIZE, 0 ) );

    const Chunk* neighbor_chunks[NEIGHBOR_RELATION_SIZE];
    neighbor_chunks[NEIGHBOR_ABOVE] = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_ABOVE], chunk_index + Vector3i(  0,  1,  0 ) );
    neighbor_chunks[NEIGHBOR_BELOW] = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_BELOW], chunk_index + Vector3i(  0, -1,  0 ) );
    neighbor_chunks[NEIGHBOR_NORTH] = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_NORTH], chunk_index + Vector3i(  0,  0,  1 ) );
    neighbor_chunks[NEIGHBOR_SOUTH] = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_SOUTH], chunk_index + Vector3i(  0,  0, -1 ) );
    neighbor_chunks[NEIGHBOR_EAST]  = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_EAST],  chunk_index + Vector3i(  1,  0,  0 ) );
    neighbor_chunks[NEIGHBOR_WEST]  = find_adjacent_chunk( region, neighbor_regions[NEIGHBOR_WEST],  chunk_index + Vector3i( -1,  0,  0 ) );

    // TODO: Should chunk_position be a member variable (as part of an AABoxf)?
    const Vector3i chunk_position = Vector3i( region.position()[0], 0, region.position()[1] ) + chunk_index * int( CHUNK_SIZE );
    update_column_faces( chunk_position, neighbor_chunks );
}

void Chunk::update_column_faces(
    const Vector3i& chunk_position,
    const Chunk* neighbor_chunks[NEIGHBOR_RELATION_SIZE]
)
{
    for ( int x = 0; x < CHUNK_SIZE; ++x )
    {
        for ( int z = 0; z < CHUNK_SIZE; ++z )
        {
            const Vector2i column_index( x, z );
            const Block* column = get_column( column_index );
            const Block* neighbor_columns[NEIGHBOR_RELATION_SIZE];
            neighbor_columns[NEIGHBOR_ABOVE] = find_colinear_column( neighbor_chunks[NEIGHBOR_ABOVE], column_index );
            neighbor_columns[NEIGHBOR_BELOW] = find_colinear_column( neighbor_chunks[NEIGHBOR_BELOW], column_index );
            neighbor_columns[NEIGHBOR_NORTH] = find_adjacent_column( *this, neighbor_chunks[NEIGHBOR_NORTH], column_index + Vector2i(  0,  1 ) );
            neighbor_columns[NEIGHBOR_SOUTH] = find_adjacent_column( *this, neighbor_chunks[NEIGHBOR_SOUTH], column_index + Vector2i(  0, -1 ) );
            neighbor_columns[NEIGHBOR_EAST]  = find_adjacent_column( *this, neighbor_chunks[NEIGHBOR_EAST],  column_index + Vector2i(  1,  0 ) );
            neighbor_columns[NEIGHBOR_WEST]  = find_adjacent_column( *this, neighbor_chunks[NEIGHBOR_WEST],  column_index + Vector2i( -1,  0 ) );

            const Vector3i column_world_position = chunk_position + Vector3i( column_index[0], 0, column_index[1] );
            const Block* lower_block = 0;

            while ( column )
            {
                add_block_faces( column_world_position, *column, neighbor_columns[NEIGHBOR_NORTH], FACE_DIRECTION_NORTH );
                add_block_faces( column_world_position, *column, neighbor_columns[NEIGHBOR_SOUTH], FACE_DIRECTION_SOUTH );
                add_block_faces( column_world_position, *column, neighbor_columns[NEIGHBOR_EAST],  FACE_DIRECTION_EAST );
                add_block_faces( column_world_position, *column, neighbor_columns[NEIGHBOR_WEST],  FACE_DIRECTION_WEST );

                add_block_endcaps( column_world_position, *column, lower_block, neighbor_columns );

                lower_block = column;
                column = column->next_;
            }
        }
    }
}

void Chunk::add_block_faces(
    const Vector3i column_world_position,
    const Block& block,
    const Block* adjacent_column,
    const FaceDirection direction
)
{
    Block::HeightT sliding_bottom = block.bottom_;

    while ( adjacent_column )
    {
        if ( adjacent_column->bottom_ < block.top_ && adjacent_column->top_ > sliding_bottom )
        {
            if ( sliding_bottom < adjacent_column->bottom_ )
            {
                add_block_face( column_world_position, sliding_bottom, adjacent_column->bottom_, block.material_, direction );
            }

            if ( adjacent_column->top_ < block.top_ )
            {
                sliding_bottom = adjacent_column->top_;
            }
            else
            {
                sliding_bottom = block.top_;
                break;
            }
        }
        else if ( adjacent_column->bottom_ > block.top_ )
        {
            break;
        }

        adjacent_column = adjacent_column->next_;
    }

    if ( sliding_bottom < block.top_ )
    {
        add_block_face( column_world_position, sliding_bottom, block.top_, block.material_, direction );
    }
}

void Chunk::add_block_endcaps(
    const Vector3i column_world_position,
    const Block& block,
    const Block* lower_block,
    const Block* neighbor_columns[NEIGHBOR_RELATION_SIZE]
)
{
    bool bottom_cap = false;

    if ( lower_block )
    {
        if ( block.bottom_ > lower_block->top_ )
        {
            bottom_cap = true;
        }
    }
    else if ( block.bottom_ == 0 && neighbor_columns[NEIGHBOR_BELOW] )
    {
        const Block* lower_column = neighbor_columns[NEIGHBOR_BELOW];

        while ( lower_column->next_ )
        {
            lower_column = lower_column->next_;
        }

        if ( lower_column->top_ != CHUNK_SIZE )
        {
            bottom_cap = true;
        }
    }
    else bottom_cap = true;

    if ( bottom_cap )
        add_block_face( column_world_position, block.bottom_, block.bottom_, block.material_, FACE_DIRECTION_DOWN ); 

    bool top_cap = false;

    if ( block.next_ )
    {
        if ( block.top_ < block.next_->bottom_ )
        {
            top_cap = true;
        }
    }
    else if ( block.top_ == CHUNK_SIZE && neighbor_columns[NEIGHBOR_ABOVE] )
    {
        if ( neighbor_columns[NEIGHBOR_ABOVE]->bottom_ != 0 )
        {
            top_cap = true;
        }
    }
    else top_cap = true;

    if ( top_cap )
        add_block_face( column_world_position, block.top_, block.top_, block.material_, FACE_DIRECTION_UP ); 
}

void Chunk::add_block_face(
    const Vector3i column_world_position,
    const Block::HeightT block_bottom, 
    const Block::HeightT block_top, 
    const BlockMaterial material,
    const FaceDirection direction
)
{
    const Vector3f block_world_position = vector_cast<Vector3f>( column_world_position ) + Vector3f( 0.0f, Scalar( block_bottom ), 0.0f );
    const Scalar height = Scalar( block_top - block_bottom );

    #define V( x, y, z ) block_world_position + Vector3f( x, y, z )
    switch ( direction )
    {
        case FACE_DIRECTION_NORTH:
            external_faces_.push_back( BlockFace( NORTH_NORMAL, material ) );
            external_faces_.back().vertices_[0] = V( 1, 0,      1 );
            external_faces_.back().vertices_[1] = V( 1, height, 1 );
            external_faces_.back().vertices_[2] = V( 0, height, 1 );
            external_faces_.back().vertices_[3] = V( 0, 0,      1 );
            break;

        case FACE_DIRECTION_SOUTH:
            external_faces_.push_back( BlockFace( SOUTH_NORMAL, material ) );
            external_faces_.back().vertices_[0] = V( 0, height, 0 );
            external_faces_.back().vertices_[1] = V( 1, height, 0 );
            external_faces_.back().vertices_[2] = V( 1, 0,      0 );
            external_faces_.back().vertices_[3] = V( 0, 0,      0 );
            break;

        case FACE_DIRECTION_EAST:
            external_faces_.push_back( BlockFace( EAST_NORMAL, material ) );
            external_faces_.back().vertices_[0] = V( 1, height, 0 );
            external_faces_.back().vertices_[1] = V( 1, height, 1 );
            external_faces_.back().vertices_[2] = V( 1, 0,      1 );
            external_faces_.back().vertices_[3] = V( 1, 0,      0 );
            break;

        case FACE_DIRECTION_WEST:
            external_faces_.push_back( BlockFace( WEST_NORMAL, material ) );
            external_faces_.back().vertices_[0] = V( 0, 0,      1 );
            external_faces_.back().vertices_[1] = V( 0, height, 1 );
            external_faces_.back().vertices_[2] = V( 0, height, 0 );
            external_faces_.back().vertices_[3] = V( 0, 0,      0 );
            break;

        case FACE_DIRECTION_UP:
            external_faces_.push_back( BlockFace( UP_NORMAL, material ) );
            external_faces_.back().vertices_[0] = V( 0, 0, 1 );
            external_faces_.back().vertices_[1] = V( 1, 0, 1 );
            external_faces_.back().vertices_[2] = V( 1, 0, 0 );
            external_faces_.back().vertices_[3] = V( 0, 0, 0 );
            break;

        case FACE_DIRECTION_DOWN:
            external_faces_.push_back( BlockFace( DOWN_NORMAL, material ) );
            external_faces_.back().vertices_[0] = V( 1, 0, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 1 );
            external_faces_.back().vertices_[2] = V( 0, 0, 1 );
            external_faces_.back().vertices_[3] = V( 0, 0, 0 );
            break;

        default:
            throw std::runtime_error( "Invalid FaceDirection: " + boost::lexical_cast<std::string>( direction ) );
    }
    #undef V
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Region:
//////////////////////////////////////////////////////////////////////////////////

Region::Region( const uint64_t base_seed, const Vector2i _position ) :
    block_pool_( sizeof( Block ) ),
    position_( _position )
{
    // TODO: Clean this up; there's lots of weird stuff here just for testing, and the function
    //       is obviously way too huge (and crazy).

    const BicubicPatchCornerFeatures fundamental_corner_features
    (
        Vector2f( 0.0f, 64.0f ),
        Vector2f( -256.0f, 256.0f ),
        Vector2f( -256.0f, 256.0f ),
        Vector2f( -256.0f, 256.0f )
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
        Vector2f( -256.0f, 256.0f ),
        Vector2f( -256.0f, 256.0f ),
        Vector2f( -256.0f, 256.0f )
    );

    const BicubicPatchFeatures octave_features
    (
        octave_corner_features,
        octave_corner_features,
        octave_corner_features,
        octave_corner_features
    );

    const BicubicPatch fundamental_patch( base_seed, position_, Vector2i( REGION_SIZE, REGION_SIZE ), fundamental_features );

    const int octave_harmonic = 2;
    const int octave_edge = REGION_SIZE / octave_harmonic;
    const Vector2i octave_size( octave_edge, octave_edge );
    const BicubicPatch octave_patches[2][2] =
    {
        // TODO: Somehow move the ultimate seeds being used here into a different space
        //       than those used for the fundamental patch.  Otherwise, the corners shared
        //       by the fundamental and octave patches will have the same attributes (boring).
        //
        //       FIXME: using base_seed ^ 0xfea873529eaf for now, but I'm not sure if I like it
        {
            BicubicPatch( base_seed ^ 0xfea873529eaf, position_ + Vector2i( 0, 0           ), octave_size, octave_features ),
            BicubicPatch( base_seed ^ 0xfea873529eaf, position_ + Vector2i( 0, octave_edge ), octave_size, octave_features ),
        },
        {
            BicubicPatch( base_seed ^ 0xfea873529eaf, position_ + Vector2i( octave_edge, 0           ), octave_size, octave_features ),
            BicubicPatch( base_seed ^ 0xfea873529eaf, position_ + Vector2i( octave_edge, octave_edge ), octave_size, octave_features )
        }
    };

    // The geometry generated by slicing up a single TrilinearBox by value ranges tends to be sheet-like,
    // which is not ideal for cave networks.  However, by taking the intersection of a value range in two
    // TrilinearBoxes, the resulting geometry is very stringy and tunnel-like.

    // TODO: get this 256 from somewhere sane
    const Vector3i trilinear_box_size( REGION_SIZE, 256, REGION_SIZE );

    TrilinearBox boxA
    (
        base_seed,
        Vector3i( position_[0], 0, position_[1] ),
        trilinear_box_size,
        32
    );

    TrilinearBox boxB
    (
        // FIXME: using base_seed ^ 0x313535f3235 for now, but I'm not sure if I like it
        base_seed ^ 0x313535f3235,
        Vector3i( position_[0], 0, position_[1] ),
        trilinear_box_size,
        32
    );

    for ( int x = 0; x < REGION_SIZE; ++x )
    {
        for ( int z = 0; z < REGION_SIZE; ++z )
        {
            const Scalar fundamental_height = fundamental_patch.interpolate(
                Vector2f( Scalar( x ) / REGION_SIZE, Scalar( z ) / REGION_SIZE )
            );

            const Scalar octave_height = octave_patches[x / octave_edge][z / octave_edge].interpolate(
                Vector2f( Scalar( x % octave_edge ) / octave_edge, Scalar( z % octave_edge ) / octave_edge )
            );

            const Scalar total_height = fundamental_height + octave_height / octave_harmonic;

            const std::pair<Scalar, BlockMaterial> layers[] = 
            {
                std::make_pair( 1.0f  + ( total_height + 0.0f  ) * 0.25f, BLOCK_MATERIAL_BEDROCK ),
                std::make_pair( 1.0f  + ( total_height + 16.0f ) * 0.45f, BLOCK_MATERIAL_NONE ),
                std::make_pair( 32.0f + ( total_height + 16.0f ) * 0.65f, BLOCK_MATERIAL_STONE ),
                std::make_pair( 32.0f + ( total_height + 16.0f ) * 0.75f, BLOCK_MATERIAL_CLAY ),
                std::make_pair( 32.0f + ( total_height + 16.0f ) * 1.00f, BLOCK_MATERIAL_DIRT )
            };

            const int layer_heights_size = sizeof( layers ) / sizeof( std::pair<Scalar, BlockMaterial> );
            const Vector2i column_index = Vector2i( x, z );
            unsigned
                bottom = 1,
                top_block = 0;

            add_block_to_column( column_index, 0, 1, BLOCK_MATERIAL_MAGMA );

            for ( int i = 0; i < layer_heights_size; ++i )
            {
                const Scalar height = std::max( layers[i].first, Scalar( bottom + 1 ) );
                const BlockMaterial material = layers[i].second;
                const unsigned top = unsigned( gmtl::Math::round( height ) );

                if ( material != BLOCK_MATERIAL_NONE )
                {
                    unsigned sliding_bottom = bottom;

                    for ( unsigned y = sliding_bottom; y <= top; ++y )
                    {
                        // TODO: Ensure that the components of this vector are clamped (or repeated) to [0.0,1.0].
                        Vector3f box_position(
                            Scalar( x ) / Scalar( trilinear_box_size[0] ),
                            Scalar( y ) / Scalar( trilinear_box_size[1] ),
                            Scalar( z ) / Scalar( trilinear_box_size[2] )
                        );

                        const Scalar densityA = boxA.interpolate( box_position );
                        const Scalar densityB = boxB.interpolate( box_position );

                        if ( densityA > 0.45 && densityA < 0.55 && densityB > 0.45 && densityB < 0.55 )
                        {
                            if ( y - sliding_bottom > 1 )
                            {
                                add_block_to_column( column_index, sliding_bottom, y, material );
                                top_block = y;
                            }

                            sliding_bottom = y;
                        }
                    }

                    if ( sliding_bottom < top )
                    {
                        add_block_to_column( column_index, sliding_bottom, top, material );
                        top_block = top;
                    }
                }

                bottom = top;
            }

            add_block_to_column( column_index, top_block, top_block + 1, BLOCK_MATERIAL_DIRT );
            add_block_to_column( column_index, top_block + 1, top_block + 2, BLOCK_MATERIAL_GRASS );
        }
    }

    // TODO: For tuning the box intersection ranges:
    // 
    // TrilinearBox boxA
    // (
    //     base_seed,
    //     Vector3i( position_[0], 0, position_[1] ),
    //     Vector3i( REGION_SIZE, REGION_SIZE, REGION_SIZE ),
    //     16
    // );

    // TrilinearBox boxB
    // (
    //     base_seed ^ 0x313535f3235,
    //     Vector3i( position_[0], 0, position_[1] ),
    //     Vector3i( REGION_SIZE, REGION_SIZE, REGION_SIZE ),
    //     16
    // );

    // // TODO: Only check density function for blocks that are created by the heightmap function above.

    // for ( int x = 0; x < REGION_SIZE; ++x )
    // {
    //     for ( int y = 0; y < REGION_SIZE; ++y )
    //     {
    //         for ( int z = 0; z < REGION_SIZE; ++z )
    //         {
    //             const Vector3f box_position( 
    //                 static_cast<Scalar>( x ) / REGION_SIZE,
    //                 static_cast<Scalar>( y ) / REGION_SIZE,
    //                 static_cast<Scalar>( z ) / REGION_SIZE
    //             );

    //             const Scalar density = boxA.interpolate( box_position );
    //             const Scalar density2 = boxB.interpolate( box_position );

    //             if ( density >= 0.40 && density <= 0.60 && density2 >= 0.40 && density2 <= 0.60 )
    //                 add_block_to_column( Vector2i( x, z ), new ( block_pool_.malloc() ) Block( y, y + 1, BLOCK_MATERIAL_GRASS ) );
    //         }
    //     }
    // }
}

void Region::add_block_to_column( const Vector2i& column_index, const unsigned bottom, const unsigned top, const BlockMaterial material )
{
    assert( column_index[0] >= 0 );
    assert( column_index[1] >= 0 );
    assert( column_index[0] < REGION_SIZE );
    assert( column_index[1] < REGION_SIZE );

    unsigned sliding_bottom = bottom;

    while ( top > sliding_bottom )
    {
        const Vector3i chunk_index = Vector3i( column_index[0], sliding_bottom, column_index[1] ) / int( Chunk::CHUNK_SIZE );
        const Vector2i chunk_column_index( column_index[0] % Chunk::CHUNK_SIZE, column_index[1] % Chunk::CHUNK_SIZE );

        const ChunkMap::const_iterator chunk_it = chunks_.find( chunk_index );
        ChunkSP chunk;

        if ( chunk_it == chunks_.end() )
        {
            chunk.reset( new Chunk );
            chunks_[chunk_index] = chunk;
        }
        else chunk = chunk_it->second;

        const unsigned chunk_bottom = chunk_index[1] * Chunk::CHUNK_SIZE;
        const Block::HeightT
            block_bottom = sliding_bottom % Chunk::CHUNK_SIZE,
            block_top = Block::HeightT( std::min( top - chunk_bottom, unsigned( Chunk::CHUNK_SIZE ) ) );

        Block* block = new ( block_pool_.malloc() ) Block( block_bottom, block_top, material );
        chunk->add_block_to_column( chunk_column_index, block );
        sliding_bottom = chunk_bottom + Chunk::CHUNK_SIZE;
    }
}
