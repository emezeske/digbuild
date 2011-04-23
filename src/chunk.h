#ifndef CHUNK_H
#define CHUNK_H

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "math.h"
#include "cardinal_relation.h"
#include "block.h"

#define FOREACH_BLOCK( x_name, y_name, z_name )\
    for ( int x_name = 0; x_name < Chunk::SIZE_X; ++x_name )\
        for ( int y_name = 0; y_name < Chunk::SIZE_Y; ++y_name )\
            for ( int z_name = 0; z_name < Chunk::SIZE_Z; ++z_name )

#define FOREACH_SURROUNDING( x_name, y_name, z_name )\
    for ( int x_name = -1; x_name <= 1; ++x_name )\
        for ( int y_name = -1; y_name <= 1; ++y_name )\
            for ( int z_name = -1; z_name <= 1; ++z_name )

struct Chunk;

struct BlockIterator
{
    BlockIterator( Chunk* chunk = 0, Block* block = 0, Vector3i index = Vector3i( 0, 0, 0 ) ) :
        chunk_( chunk ),
        block_( block ),
        index_( index )
    {
    }

    Chunk* chunk_;
    Block* block_;
    Vector3i index_;
};

struct Chunk : public boost::noncopyable
{
    static const int
        SIZE_X = 16,
        SIZE_Y = 16,
        SIZE_Z = 16;

    static const Vector3i SIZE;

    Chunk( const Vector3i& position );

    const Vector3i& get_position() const { return position_; }

    Block* maybe_get_block( const Vector3i& index )
    {
        if( block_in_range( index ) )
        {
            return &blocks_[index[0]][index[1]][index[2]];
        }
        else return 0;
    }

    Block& get_block( const Vector3i& index )
    {
        assert( block_in_range( index ) );
        return blocks_[index[0]][index[1]][index[2]];
    }

    void set_block( const Vector3i& index, const Block& block )
    {
        assert( block_in_range( index ) );
        blocks_[index[0]][index[1]][index[2]] = block;
    }

    BlockIterator get_block_neighbor( const Vector3i& index, const Vector3i& relation )
    {
        assert( relation_in_range( relation ) );
        Vector3i neighbor_index = index + relation;
        Vector3i neighbor_chunk_relation( 0, 0, 0 );

        for ( int i = 0; i < 3; ++i )
        {
            if ( neighbor_index[i] == -1 )
            {
                neighbor_index[i] = SIZE[i] - 1;
                neighbor_chunk_relation[i] = -1;
            }
            else if ( neighbor_index[i] == SIZE[i] )
            {
                neighbor_index[i] = 0;
                neighbor_chunk_relation[i] = 1;
            }
        }

        Chunk* neighbor_chunk = get_neighbor( neighbor_chunk_relation );
        Block* neighbor_block = neighbor_chunk ? &neighbor_chunk->get_block( neighbor_index ) : 0;
        return BlockIterator( neighbor_chunk, neighbor_block, neighbor_index );
    }

    Chunk* get_neighbor( const Vector3i& relation )
    {
        return get_neighbor_impl( relation );
    }

    void set_neighbor( const Vector3i& relation, Chunk* new_neighbor )
    {
        const Vector3i reverse_relation = -relation;

        assert( !new_neighbor || !new_neighbor->get_neighbor( reverse_relation ) );

        Chunk* existing_neighbor = get_neighbor( relation );

        if ( existing_neighbor )
        {
            assert( !new_neighbor );
            existing_neighbor->get_neighbor_impl( reverse_relation ) = 0;
        }

        if ( new_neighbor )
        {
            new_neighbor->get_neighbor_impl( reverse_relation ) = this;
        }

        get_neighbor_impl( relation ) = new_neighbor;
    }

    Chunk* get_column_bottom()
    {
        return get_extreme( CARDINAL_RELATION_BELOW );
    }

    Chunk* get_column_top()
    {
        return get_extreme( CARDINAL_RELATION_ABOVE );
    }

    void reset_lighting();
    void unset_nop_sunlight_sources();
    void apply_lighting_to_self();
    void apply_lighting_to_neighbors();
    void update_geometry();

    const BlockFaceV& get_external_faces() const { return external_faces_; }

private:

    bool relation_in_range( const Vector3i& relation )
    {
        return relation[0] >= -1 && relation[0] <= 1 &&
               relation[1] >= -1 && relation[1] <= 1 &&
               relation[2] >= -1 && relation[2] <= 1;
    }

    bool block_in_range( const Vector3i& index )
    {
        return index[0] >= 0 && index[1] >= 0 && index[2] >= 0 &&
               index[0] < SIZE_X && index[1] < SIZE_Y && index[2] < SIZE_Z;
    }

    Chunk*& get_neighbor_impl( const Vector3i& relation )
    {
        assert( relation_in_range( relation ) );
        return neighbors_[relation[0] + 1][relation[1] + 1][relation[2] + 1];
    }

    Chunk* get_extreme( const CardinalRelation relation )
    {
        const Vector3i direction = cardinal_relation_vector( relation );
        Chunk* next = this;
        Chunk* extreme = 0;

        while ( next )
        {
            extreme = next;
            next = next->get_neighbor( direction );
        }

        assert( extreme );
        return extreme;
    }

    void add_external_face(
        const Vector3i& block_index,
        const Vector3f& block_position,
        const Block& block,
        const CardinalRelation relation,
        const Vector3i& relation_vector
    );

    void calculate_vertex_lighting(
        const Vector3i& primary_index,
        const Vector3i& primary_relation,
        const Vector3i& neighbor_relation_a,
        const Vector3i& neighbor_relation_b,
        Vector3f& vertex_lighting,
        Vector3f& vertex_sunlighting
    );

    Vector3i position_;

    Block blocks_[SIZE_X][SIZE_Y][SIZE_Z];

    BlockFaceV external_faces_;

    Chunk* neighbors_[3][3][3];
};

typedef boost::shared_ptr<Chunk> ChunkSP;
typedef std::vector<ChunkSP> ChunkSPV;
typedef std::vector<Chunk*> ChunkV;
typedef std::map<Vector3i, ChunkSP, VectorLess<Vector3i> > ChunkMap;

void chunk_stitch_into_map( ChunkSP chunk, ChunkMap& chunks );
void chunk_unstich_from_map( ChunkSP chunk, ChunkMap& chunks );

#endif // CHUNK_H
