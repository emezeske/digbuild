#include "chunk.h"

namespace {

const Vector3f
    NORTH_NORMAL(  0,  0,  1 ),
    SOUTH_NORMAL(  0,  0, -1 ),
    EAST_NORMAL (  1,  0,  0 ),
    WEST_NORMAL ( -1,  0,  0 ),
    UP_NORMAL   (  0,  1,  0 ),
    DOWN_NORMAL (  0, -1,  0 );

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Chunk:
//////////////////////////////////////////////////////////////////////////////////

Chunk::Chunk( const Vector3i& position ) :
    position_( position )
{
}

void Chunk::update_external_faces()
{
    external_faces_.clear();

    // TODO: This might be made more efficient by only checking the ABOVE, NORTH, and EAST neighbors
    //       for each Block, and adding faces for either the current block OR the neighbors.

    for ( int x = 0; x < CHUNK_SIZE; ++x )
    {
        for ( int y = 0; y < CHUNK_SIZE; ++y )
        {
            for ( int z = 0; z < CHUNK_SIZE; ++z )
            {
                const Vector3i block_index( x, y, z );
                const Block& block = get_block( block_index );
                const Vector3f block_position = vector_cast<Vector3f>( position_ + block_index );

                maybe_add_external_face( block_position, block_index, block, NEIGHBOR_ABOVE );
                maybe_add_external_face( block_position, block_index, block, NEIGHBOR_BELOW );
                maybe_add_external_face( block_position, block_index, block, NEIGHBOR_NORTH );
                maybe_add_external_face( block_position, block_index, block, NEIGHBOR_SOUTH );
                maybe_add_external_face( block_position, block_index, block, NEIGHBOR_EAST );
                maybe_add_external_face( block_position, block_index, block, NEIGHBOR_WEST );
            }
        }
    }
}

void Chunk::maybe_add_external_face( const Vector3f& block_position, const Vector3i& block_index, const Block& block, const NeighborRelation relation )
{
    const Block* block_neighbor = get_block_neighbor( block_index, relation );

    if ( !block_neighbor )
    {
        add_external_face( block_position, block, relation );
    }
}

void Chunk::add_external_face( const Vector3f& block_position, const Block& block, const NeighborRelation relation )
{
    assert( relation >= 0 && relation <= NEIGHBOR_RELATION_SIZE );
    #define V( x, y, z ) block_position + Vector3f( x, y, z )
    switch ( relation )
    {
        case NEIGHBOR_ABOVE:
            external_faces_.push_back( BlockFace( UP_NORMAL, block.get_material() ) );
            external_faces_.back().vertices_[0] = V( 0, 1, 0 );
            external_faces_.back().vertices_[1] = V( 1, 1, 0 );
            external_faces_.back().vertices_[2] = V( 1, 1, 1 );
            external_faces_.back().vertices_[3] = V( 0, 1, 1 );
            break;

        case NEIGHBOR_BELOW:
            external_faces_.push_back( BlockFace( DOWN_NORMAL, block.get_material() ) );
            external_faces_.back().vertices_[0] = V( 0, 0, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 0 );
            external_faces_.back().vertices_[2] = V( 1, 0, 1 );
            external_faces_.back().vertices_[3] = V( 0, 0, 1 );
            break;

        case NEIGHBOR_NORTH:
            external_faces_.push_back( BlockFace( NORTH_NORMAL, block.get_material() ) );
            external_faces_.back().vertices_[0] = V( 1, 0, 1 );
            external_faces_.back().vertices_[1] = V( 0, 0, 1 );
            external_faces_.back().vertices_[2] = V( 0, 1, 1 );
            external_faces_.back().vertices_[3] = V( 1, 1, 1 );
            break;

        case NEIGHBOR_SOUTH:
            external_faces_.push_back( BlockFace( SOUTH_NORMAL, block.get_material() ) );
            external_faces_.back().vertices_[0] = V( 0, 0, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 0 );
            external_faces_.back().vertices_[2] = V( 1, 1, 0 );
            external_faces_.back().vertices_[3] = V( 0, 1, 0 );
            break;

        case NEIGHBOR_EAST:
            external_faces_.push_back( BlockFace( EAST_NORMAL, block.get_material() ) );
            external_faces_.back().vertices_[0] = V( 1, 0, 0 );
            external_faces_.back().vertices_[1] = V( 1, 0, 1 );
            external_faces_.back().vertices_[2] = V( 1, 1, 1 );
            external_faces_.back().vertices_[3] = V( 1, 1, 0 );
            break;

        case NEIGHBOR_WEST:
            external_faces_.push_back( BlockFace( WEST_NORMAL, block.get_material() ) );
            external_faces_.back().vertices_[0] = V( 0, 0, 0 );
            external_faces_.back().vertices_[1] = V( 0, 1, 0 );
            external_faces_.back().vertices_[2] = V( 0, 1, 1 );
            external_faces_.back().vertices_[3] = V( 0, 0, 1 );
            break;

        default:
            throw std::runtime_error( "Invalid neighbor relation." );
    }
    #undef V
}

//////////////////////////////////////////////////////////////////////////////////
// Free function definitions:
//////////////////////////////////////////////////////////////////////////////////

void chunk_stitch_into_map( ChunkSP chunk, ChunkMap& chunks )
{
    ChunkMap::iterator neighbors[NEIGHBOR_RELATION_SIZE];
    neighbors[NEIGHBOR_ABOVE] = chunks.find( chunk->get_position() + Vector3i( 0, Chunk::CHUNK_SIZE, 0 ) );
    neighbors[NEIGHBOR_BELOW] = chunks.find( chunk->get_position() + Vector3i( 0, -Chunk::CHUNK_SIZE, 0 ) );
    neighbors[NEIGHBOR_NORTH] = chunks.find( chunk->get_position() + Vector3i( 0, 0, Chunk::CHUNK_SIZE ) );
    neighbors[NEIGHBOR_SOUTH] = chunks.find( chunk->get_position() + Vector3i( 0, 0, -Chunk::CHUNK_SIZE ) );
    neighbors[NEIGHBOR_EAST]  = chunks.find( chunk->get_position() + Vector3i( Chunk::CHUNK_SIZE, 0, 0 ) );
    neighbors[NEIGHBOR_WEST]  = chunks.find( chunk->get_position() + Vector3i( -Chunk::CHUNK_SIZE, 0, 0 ) );

    for ( int relation = 0; relation < NEIGHBOR_RELATION_SIZE; ++relation )
    {
        if ( neighbors[relation] != chunks.end() )
        {
            chunk->set_neighbor( relation, neighbors[relation]->second.get() );
        }
    }

    chunks[chunk->get_position()] = chunk;
}

void chunk_unstich_from_map( ChunkSP chunk, ChunkMap& chunks )
{
    for ( NeighborRelation relation = 0; relation < NEIGHBOR_RELATION_SIZE; ++relation )
    {
        if ( neighbors[relation] != chunks.end() )
        {
            chunk->set_neighbor( relation, 0 );
        }
    }

    chunks.erase( chunk->get_position() );
}
