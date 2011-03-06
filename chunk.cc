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

Vector3i cardinal_relation_vector( const CardinalRelation relation )
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
typedef std::pair<BlockIterator, uint8_t> FloodFillBlock;
typedef std::deque<FloodFillBlock> FloodFillQueue;

void breadth_first_flood_fill_light( FloodFillQueue& queue, const BlockIterator& iterator, const uint8_t light_level, BlockV& blocks_visited )
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
            flood_block.first.block_->set_light_level( flood_block.second );

            const uint8_t attenuated_light_level = flood_block.second - 1;

            if ( attenuated_light_level > 0 )
            {
                FOR_EACH_CARDINAL_RELATION( relation )
                {
                    const Vector3i relation_vector = cardinal_relation_vector( relation );
                    BlockIterator neighbor = flood_block.first.chunk_->get_block_neighbor( flood_block.first.index_, relation_vector );

                    if ( neighbor.block_ &&
                         neighbor.block_->get_material() == BLOCK_MATERIAL_NONE &&
                         !neighbor.block_->is_visited() &&
                         !neighbor.block_->is_sunlight_source() &&
                         ( neighbor.block_->get_light_level() < attenuated_light_level ) )
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

                if ( above_ground )
                {
                    if ( block.get_material() == BLOCK_MATERIAL_NONE )
                    {
                        block.set_light_source( true );
                        block.set_sunlight_source( true );
                    }
                    else
                    {
                        above_ground = false;
                        block.set_light_level( 0 );
                    }
                }
                else if ( block.get_material() == BLOCK_MATERIAL_MAGMA )
                {
                    block.set_light_source( true );
                }
                else block.set_light_level( 0 );
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

            // TODO: This might be made more efficient by only checking the ABOVE, NORTH, and EAST neighbors
            //       for each Block, and adding faces for either the current block OR the neighbor.
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
                        vector_cast<Scalar>( relation_vector )
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

void Chunk::add_external_face( const Vector3i& block_index, const Vector3f& block_position, const Block& block, const CardinalRelation relation, const Vector3f& normal )
{
    // TODO: Uhh, make the vertex lighting PER VERTEX!
    const Vector3f lighting = calculate_vertex_lighting( block_index );
    external_faces_.push_back( BlockFace( normal, lighting, block.get_material() ) );

    #define V( x, y, z )\
        BlockFace::Vertex( block_position + Vector3f( x, y, z ),\
        calculate_vertex_lighting( block_index + Vector3i( x, y, z ) ) )

    switch ( relation )
    {
        case CARDINAL_RELATION_ABOVE:
            external_faces_.back().vertices_[0] = V( 0, 1, 0 );
            external_faces_.back().vertices_[1] = V( 1, 1, 0 );
            external_faces_.back().vertices_[2] = V( 1, 1, 1 );
            external_faces_.back().vertices_[3] = V( 0, 1, 1 );
            break;

        case CARDINAL_RELATION_BELOW:
            external_faces_.back().vertices_[0] = V( 0, 0, 0 );
            external_faces_.back().vertices_[1] = V( 0, 0, 1 );
            external_faces_.back().vertices_[2] = V( 1, 0, 1 );
            external_faces_.back().vertices_[3] = V( 1, 0, 0 );
            break;

        case CARDINAL_RELATION_NORTH:
            external_faces_.back().vertices_[0] = V( 1, 0, 1 );
            external_faces_.back().vertices_[1] = V( 0, 0, 1 );
            external_faces_.back().vertices_[2] = V( 0, 1, 1 );
            external_faces_.back().vertices_[3] = V( 1, 1, 1 );
            break;

        case CARDINAL_RELATION_SOUTH:
            external_faces_.back().vertices_[0] = V( 0, 0, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 0 );
            external_faces_.back().vertices_[2] = V( 1, 1, 0 );
            external_faces_.back().vertices_[3] = V( 0, 1, 0 );
            break;

        case CARDINAL_RELATION_EAST:
            external_faces_.back().vertices_[0] = V( 1, 0, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 1 );
            external_faces_.back().vertices_[2] = V( 1, 1, 1 );
            external_faces_.back().vertices_[3] = V( 1, 1, 0 );
            break;

        case CARDINAL_RELATION_WEST:
            external_faces_.back().vertices_[0] = V( 0, 0, 0 );
            external_faces_.back().vertices_[1] = V( 0, 1, 0 );
            external_faces_.back().vertices_[2] = V( 0, 1, 1 );
            external_faces_.back().vertices_[3] = V( 0, 0, 1 );
            break;

        default:
            throw std::runtime_error( "Invalid cardinal relation." );
    }
    #undef V
}

// TODO Cache the result of this function?
Vector3f Chunk::calculate_vertex_lighting( const Vector3i& vertex_index )
{
    const size_t NUM_SURROUNDING_BLOCKS = 8;

    const Block* surrounding_blocks[NUM_SURROUNDING_BLOCKS] =
    {
        get_block_neighbor( vertex_index, Vector3i(  0,  0,  0 ) ).block_,
        get_block_neighbor( vertex_index, Vector3i(  0,  0, -1 ) ).block_,
        get_block_neighbor( vertex_index, Vector3i(  0, -1,  0 ) ).block_,
        get_block_neighbor( vertex_index, Vector3i(  0, -1, -1 ) ).block_,
        get_block_neighbor( vertex_index, Vector3i( -1,  0,  0 ) ).block_,
        get_block_neighbor( vertex_index, Vector3i( -1,  0, -1 ) ).block_,
        get_block_neighbor( vertex_index, Vector3i( -1, -1,  0 ) ).block_,
        get_block_neighbor( vertex_index, Vector3i( -1, -1, -1 ) ).block_
    };

    int total_lighting = 0;
    int num_contributing_blocks = 0;

    for ( size_t i = 0; i < NUM_SURROUNDING_BLOCKS; ++i )
    {
        const Block* block = surrounding_blocks[i];

        if ( block && block->get_material() == BLOCK_MATERIAL_NONE )
        {
            total_lighting += block->get_light_level();
            ++num_contributing_blocks;
        }
    }

    Scalar average_lighting = 0;

    if ( num_contributing_blocks != 0 )
    {
        // TODO Proper attenuation.
        average_lighting = Scalar( total_lighting ) / Scalar( num_contributing_blocks ) / Scalar( Block::FULLY_LIT );
    }

    // TODO: How does ambient occlusion *really* work?
    average_lighting -= ( 8 - num_contributing_blocks ) * 0.09;

    return Vector3f( average_lighting, average_lighting, average_lighting );
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

        if ( block.is_light_source() )
        {
            breadth_first_flood_fill_light( queue, BlockIterator( &chunk, &block, index ), Block::FULLY_LIT, blocks_visited );

            for ( BlockV::iterator it = blocks_visited.begin(); it != blocks_visited.end(); ++it )
            {
                ( *it )->set_visited( false );
            }
        }
    }
}
