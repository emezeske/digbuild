#include <deque>

#include <string.h>

#include "chunk.h"

#define FOREACH_BLOCK( x_name, y_name, z_name )\
    for ( int x_name = 0; x_name < Chunk::CHUNK_SIZE; ++x_name )\
        for ( int y_name = 0; y_name < Chunk::CHUNK_SIZE; ++y_name )\
            for ( int z_name = 0; z_name < Chunk::CHUNK_SIZE; ++z_name )

#define FOR_EACH_CARDINAL_RELATION( iterator_name )\
    for ( CardinalRelation iterator_name = CARDINAL_RELATION_ABOVE;\
          iterator_name != NUM_CARDINAL_RELATIONS;\
          iterator_name = CardinalRelation( int( iterator_name ) + 1 ) )

#define FOREACH_RELATION( x_name, y_name, z_name )\
    for ( int x_name = -1; x_name < 2; ++x_name )\
        for ( int y_name = -1; y_name < 2; ++y_name )\
            for ( int z_name = -1; z_name < 2; ++z_name )

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

inline Vector3i cardinal_relation_vector( const CardinalRelation relation )
{
    switch ( relation )
    {
        case CARDINAL_RELATION_ABOVE: return Vector3i(  0,  1,  0 );
        case CARDINAL_RELATION_BELOW: return Vector3i(  0, -1,  0 );
        case CARDINAL_RELATION_NORTH: return Vector3i(  0,  0,  1 );
        case CARDINAL_RELATION_SOUTH: return Vector3i(  0,  0, -1 );
        case CARDINAL_RELATION_EAST:  return Vector3i(  1,  0,  0 );
        case CARDINAL_RELATION_WEST:  return Vector3i( -1,  0,  0 );
        default: throw std::runtime_error( "Invalid cardinal relation." );
    }
}

typedef std::vector<Block*> BlockV;
typedef std::pair<BlockIterator, Vector4i> FloodFillBlock;
typedef std::deque<FloodFillBlock> FloodFillQueue;

void breadth_first_flood_fill_light(
    FloodFillQueue& queue,
    const BlockIterator& iterator,
    const bool is_sunlight,
    const Vector4i light_level,
    BlockV& blocks_visited
)
{
    blocks_visited.clear();
    queue.clear();
    queue.push_back( std::make_pair( iterator, light_level ) );

    while ( !queue.empty() )
    {
        FloodFillBlock flood_block = queue.front();
        queue.pop_front();

        if ( !flood_block.first.block_->is_visited() )
        {
            blocks_visited.push_back( flood_block.first.block_ );
            flood_block.first.block_->set_visited( true );

            Vector4i block_light_level = flood_block.first.block_->get_light_level() + flood_block.second;

            for ( int i = 0; i < Vector4i::Size; ++i )
            {
                block_light_level[i] = std::min( block_light_level[i], Block::MAX_LIGHT_LEVEL[i] );
            }

            flood_block.first.block_->set_light_level( block_light_level );

            Vector4i attenuated_light_level = flood_block.second - Vector4i( 1, 1, 1, 1 );

            for ( int i = 0; i < Vector4i::Size; ++i )
            {
                attenuated_light_level[i] = std::max( attenuated_light_level[i], 0 );
            }

            if ( attenuated_light_level != Block::MIN_LIGHT_LEVEL )
            {
                FOR_EACH_CARDINAL_RELATION( relation )
                {
                    const Vector3i relation_vector = cardinal_relation_vector( relation );
                    BlockIterator neighbor = flood_block.first.chunk_->get_block_neighbor( flood_block.first.index_, relation_vector );

                    if ( neighbor.block_ &&
                         neighbor.block_->get_material() == BLOCK_MATERIAL_NONE &&
                         !neighbor.block_->is_visited() &&
                         ( !is_sunlight || !neighbor.block_->is_sunlight_source() ) &&
                         ( neighbor.block_->get_light_level()[0] < attenuated_light_level[0] ||
                           neighbor.block_->get_light_level()[1] < attenuated_light_level[1] ||
                           neighbor.block_->get_light_level()[2] < attenuated_light_level[2] ||
                           neighbor.block_->get_light_level()[3] < attenuated_light_level[3] ) )
                    {
                        queue.push_back( std::make_pair( neighbor, attenuated_light_level ) );
                    }
                }
            }
        }
    }
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Chunk:
//////////////////////////////////////////////////////////////////////////////////

Chunk::Chunk( const Vector3i& position ) :
    position_( position )
{
    memset( neighbors_, 0, sizeof( neighbors_ ) );
    get_neighbor_impl( Vector3i( 0, 0, 0 ) ) = this;
}

void Chunk::reset_lighting()
{
    for ( int x = 0; x < CHUNK_SIZE; ++x )
    {
        for ( int z = 0; z < CHUNK_SIZE; ++z )
        {
            const int y_max = CHUNK_SIZE - 1;
            const Vector3i top_block_index( x, y_max, z );
            const Block* block_above = get_block_neighbor( top_block_index, Vector3i( 0, 1, 0 ) ).block_;
            bool above_ground = !block_above || block_above->is_sunlight_source();

            for ( int y = y_max; y >= 0; --y )
            {
                Block& block = get_block( Vector3i( x, y, z ) );
                block.set_light_level( Block::MIN_LIGHT_LEVEL );

                if ( above_ground )
                {
                    if ( block.get_material() == BLOCK_MATERIAL_NONE )
                    {
                        block.set_sunlight_source( true );
                    }
                    else
                    {
                        above_ground = false;
                        block.set_sunlight_source( false );
                    }
                }
                else block.set_sunlight_source( false );
            }
        }
    }
}

void Chunk::update_geometry()
{
    external_faces_.clear();
    collision_boxes_.clear();

    FOREACH_BLOCK( x, y, z )
    {
        const Vector3i block_index( x, y, z );
        const Block& block = get_block( block_index );

        if ( block.get_material() != BLOCK_MATERIAL_NONE )
        {
            const Vector3f block_position = vector_cast<Scalar>( Vector3i( position_ + block_index ) );
            bool block_visible = false;

            FOR_EACH_CARDINAL_RELATION( relation )
            {
                const Vector3i relation_vector = cardinal_relation_vector( relation );
                const Block* block_neighbor = get_block_neighbor( block_index, relation_vector ).block_;

                if ( !block_neighbor || block_neighbor->get_material() == BLOCK_MATERIAL_NONE )
                {
                    add_external_face(
                        block_index,
                        block_position,
                        block,
                        relation,
                        relation_vector
                    );
                    block_visible = true;
                }
            }

            if ( block_visible )
            {
                collision_boxes_.push_back( gmtl::AABoxf( block_position, block_position + Vector3f( 1.0f, 1.0f, 1.0f ) ) );
            }
        }
    }
}

void Chunk::add_external_face( const Vector3i& block_index, const Vector3f& block_position, const Block& block, const CardinalRelation relation, const Vector3i& relation_vector )
{
    external_faces_.push_back( BlockFace( vector_cast<Scalar>( relation_vector ), block.get_material() ) );

    #define V( x, y, z, nax, nay, naz, nbx, nby, nbz )\
        BlockFace::Vertex( block_position + Vector3f( x, y, z ),\
        calculate_vertex_lighting( block_index, relation_vector, Vector3i( nax, nay, naz ), Vector3i( nbx, nby, nbz ) ) )

    switch ( relation )
    {
        case CARDINAL_RELATION_ABOVE:
            external_faces_.back().vertices_[0] = V( 0, 1, 0, -1, 0, 0, 0, 0, -1 );
            external_faces_.back().vertices_[1] = V( 1, 1, 0,  1, 0, 0, 0, 0, -1 );
            external_faces_.back().vertices_[2] = V( 1, 1, 1,  1, 0, 0, 0, 0,  1 );
            external_faces_.back().vertices_[3] = V( 0, 1, 1, -1, 0, 0, 0, 0,  1 );
            break;

        case CARDINAL_RELATION_BELOW:
            external_faces_.back().vertices_[0] = V( 0, 0, 0, -1, 0, 0, 0, 0, -1 );
            external_faces_.back().vertices_[1] = V( 0, 0, 1, -1, 0, 0, 0, 0,  1 );
            external_faces_.back().vertices_[2] = V( 1, 0, 1,  1, 0, 0, 0, 0,  1 );
            external_faces_.back().vertices_[3] = V( 1, 0, 0,  1, 0, 0, 0, 0, -1 );
            break;

        case CARDINAL_RELATION_NORTH:
            external_faces_.back().vertices_[0] = V( 1, 0, 1,  1, 0, 0, 0, -1, 0 );
            external_faces_.back().vertices_[1] = V( 0, 0, 1, -1, 0, 0, 0, -1, 0 );
            external_faces_.back().vertices_[2] = V( 0, 1, 1, -1, 0, 0, 0,  1, 0 );
            external_faces_.back().vertices_[3] = V( 1, 1, 1,  1, 0, 0, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_SOUTH:
            external_faces_.back().vertices_[0] = V( 0, 0, 0, -1, 0, 0, 0, -1, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 0,  1, 0, 0, 0, -1, 0 );
            external_faces_.back().vertices_[2] = V( 1, 1, 0,  1, 0, 0, 0,  1, 0 );
            external_faces_.back().vertices_[3] = V( 0, 1, 0, -1, 0, 0, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_EAST:
            external_faces_.back().vertices_[0] = V( 1, 0, 0, 0, 0, -1, 0, -1, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 1, 0, 0,  1, 0, -1, 0 );
            external_faces_.back().vertices_[2] = V( 1, 1, 1, 0, 0,  1, 0,  1, 0 );
            external_faces_.back().vertices_[3] = V( 1, 1, 0, 0, 0, -1, 0,  1, 0 );
            break;

        case CARDINAL_RELATION_WEST:
            external_faces_.back().vertices_[0] = V( 0, 0, 0, 0, 0, -1, 0, -1, 0 );
            external_faces_.back().vertices_[1] = V( 0, 1, 0, 0, 0, -1, 0,  1, 0 );
            external_faces_.back().vertices_[2] = V( 0, 1, 1, 0, 0,  1, 0,  1, 0 );
            external_faces_.back().vertices_[3] = V( 0, 0, 1, 0, 0,  1, 0, -1, 0 );
            break;

        default:
            throw std::runtime_error( "Invalid cardinal relation." );
    }
    #undef V
}

Vector4f Chunk::calculate_vertex_lighting(
    const Vector3i& primary_index,
    const Vector3i& primary_relation,
    const Vector3i& neighbor_relation_a,
    const Vector3i& neighbor_relation_b
)
{
    const size_t NUM_NEIGHBORS = 4;
    BlockIterator neighbors[NUM_NEIGHBORS];
    neighbors[0] = get_block_neighbor( primary_index, primary_relation );
    neighbors[1] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_a );
    neighbors[2] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_b );

    bool neighbor_ab_may_contribute = false;

    if ( !neighbors[1].block_ || neighbors[1].block_->get_material() == BLOCK_MATERIAL_NONE ||
         !neighbors[2].block_ || neighbors[2].block_->get_material() == BLOCK_MATERIAL_NONE )
    {
        neighbor_ab_may_contribute = true;
        neighbors[3] = get_block_neighbor( primary_index, primary_relation + neighbor_relation_a + neighbor_relation_b );
    }

    Vector4i total_lighting = Block::MIN_LIGHT_LEVEL;
    int num_contributors = 0;

    for ( size_t i = 0; i < NUM_NEIGHBORS; ++i )
    {
        const Block* block = neighbors[i].block_;

        if ( block )
        {
            if ( block->get_material() == BLOCK_MATERIAL_NONE )
            {
                total_lighting += block->get_light_level();
                ++num_contributors;
            }
        }
        else if ( i != 3 || neighbor_ab_may_contribute )
        {
            total_lighting += Vector4i( 0, 0, 0, Block::MAX_LIGHT_LEVEL[3] );
            ++num_contributors;
        }
    }

    const Vector4f average_lighting = vector_cast<Scalar>( total_lighting ) / Scalar( num_contributors );
    const Vector4f attenuation_power = vector_cast<Scalar>( Block::MAX_LIGHT_LEVEL ) - average_lighting;
    const int ambient_occlusion_power = NUM_NEIGHBORS - neighbor_ab_may_contribute - num_contributors;
    Vector4f attenuated_lighting;

    for ( int i = 0; i < Vector4i::Size; ++i )
    {
        // TODO: This is extremely slow!  Around half of the total vertex lighting
        //       computation time is tied up in these pow() calls.
        attenuated_lighting[i] = 
            gmtl::Math::pow( 0.75f, attenuation_power[i] ) *
            gmtl::Math::pow( 0.75f, ambient_occlusion_power );
    }

    return attenuated_lighting;
}

//////////////////////////////////////////////////////////////////////////////////
// Free function definitions:
//////////////////////////////////////////////////////////////////////////////////

void chunk_stitch_into_map( ChunkSP chunk, ChunkMap& chunks )
{
    FOREACH_RELATION( x, y, z )
    {
        const Vector3i relation( x, y, z );

        ChunkMap::iterator neighbor_it = chunks.find( chunk->get_position() + relation * int( Chunk::CHUNK_SIZE ) );

        if ( neighbor_it != chunks.end() )
        {
            chunk->set_neighbor( relation, neighbor_it->second.get() );
        }
    }

    chunks[chunk->get_position()] = chunk;
}

void chunk_unstich_from_map( ChunkSP chunk, ChunkMap& chunks )
{
    FOREACH_RELATION( x, y, z )
    {
        const Vector3i relation( x, y, z );
        chunk->set_neighbor( relation, 0 );
    }

    chunks.erase( chunk->get_position() );
}

void chunk_apply_lighting( Chunk& chunk )
{
    BlockV blocks_visited;
    FloodFillQueue queue;

    FOREACH_BLOCK( x, y, z )
    {
        const Vector3i index( x, y, z );
        Block& block = chunk.get_block( index );

        Vector4i light_level;

        if ( block.is_sunlight_source() )
        {
            light_level = Vector4i( 0, 0, 0, Block::MAX_LIGHT_LEVEL[3] );
        }
        else if ( block.get_material() == BLOCK_MATERIAL_MAGMA )
        {
            light_level = Vector4i( 14, 4, 0, 0 );
        }

        if ( light_level != Block::MIN_LIGHT_LEVEL )
        {
            breadth_first_flood_fill_light(
                queue,
                BlockIterator( &chunk, &block, index ),
                block.is_sunlight_source(),
                light_level,
                blocks_visited
            );

            for ( BlockV::iterator it = blocks_visited.begin(); it != blocks_visited.end(); ++it )
            {
                ( *it )->set_visited( false );
            }
        }
    }
}
