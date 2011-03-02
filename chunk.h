#ifndef CHUNK_H
#define CHUNK_H

#include <vector>
#include <map>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "math.h"
#include "block.h"

enum NeighborRelation
{
    NEIGHBOR_ABOVE,
    NEIGHBOR_BELOW,
    NEIGHBOR_NORTH,
    NEIGHBOR_SOUTH,
    NEIGHBOR_EAST,
    NEIGHBOR_WEST,
    NEIGHBOR_RELATION_SIZE
};

inline NeighborRelation reverse_neighbor_relation( const NeighborRelation relation )
{
    switch ( relation )
    {
        case NEIGHBOR_ABOVE: return NEIGHBOR_BELOW;
        case NEIGHBOR_BELOW: return NEIGHBOR_ABOVE;
        case NEIGHBOR_NORTH: return NEIGHBOR_SOUTH;
        case NEIGHBOR_SOUTH: return NEIGHBOR_NORTH;
        case NEIGHBOR_EAST:  return NEIGHBOR_WEST;
        case NEIGHBOR_WEST:  return NEIGHBOR_EAST;
        default: throw std::runtime_error( "Invalid neighbor relation." );
    }
}

struct Chunk : public boost::noncopyable
{
    enum { CHUNK_SIZE = 16 };

    Chunk( const Vector3i& position );

    const Vector3i& get_position() const { return position_; }

    Block& get_block( const Vector3i& index )
    {
        assert( block_in_range( index ) );
        return blocks_[index[0]][index[1]][index[2]];
    }

    Block* get_block_neighbor( Vector3i index, const NeighborRelation relation )
    {
        assert( relation >= 0 && relation < NEIGHBOR_RELATION_SIZE );

        bool use_chunk_neighbor = false;

        #define _increment_wrap( c )\
            do\
                if ( ++index[c] == CHUNK_SIZE )\
                {\
                    index[c] = 0;\
                    use_chunk_neighbor = true;\
                }\
            while ( false )

        #define _decrement_wrap( c )\
            do\
                if ( --index[c] == -1 )\
                {\
                    index[c] = CHUNK_SIZE - 1;\
                    use_chunk_neighbor = true;\
                }\
            while ( false )

        switch ( relation )
        {
            case NEIGHBOR_ABOVE: _increment_wrap( 1 ); break;
            case NEIGHBOR_BELOW: _decrement_wrap( 1 ); break;
            case NEIGHBOR_NORTH: _increment_wrap( 2 ); break;
            case NEIGHBOR_SOUTH: _decrement_wrap( 2 ); break;
            case NEIGHBOR_EAST:  _increment_wrap( 0 ); break;
            case NEIGHBOR_WEST:  _decrement_wrap( 0 ); break;
            default: throw std::runtime_error( "Invalid neighbor relation." );
        };

        #undef _decrement_wrap
        #undef _increment_wrap

        Chunk* chunk = use_chunk_neighbor ? get_neighbor( relation ) : this;

        if ( use_chunk_neighbor )
        {
            Chunk* chunk_neighbor = get_neighbor( relation );
            return chunk_neighbor ? &chunk_neighbor->get_block( index ) : 0;
        }
        else return &get_block( index );
    }

    Chunk* get_neighbor( const NeighborRelation relation )
    {
        assert( relation >= 0 && relation < NEIGHBOR_RELATION_SIZE );
        return neighbors_[relation];
    }

    void set_neighbor( const NeighborRelation relation, Chunk* new_neighbor )
    {
        assert( relation >= 0 && relation < NEIGHBOR_RELATION_SIZE );
        assert( !new_neighbor || !new_neighbor->get_neighbor( reverse_relation ) );

        Chunk* existing_neighbor = neighbors_[relation];
        const NeighborRelation reverse_relation = reverse_neighbor_relation( relation );

        if ( existing_neighbor )
        {
            assert( !new_neighbor );
            neighbors_[relation]->neighbors_[reverse_relation] = 0;
        }

        if ( new_neighbor )
        {
            new_neighbor->neighbors_[reverse_relation] = this;
        }

        neighbors_[relation] = new_neighbor;
    }

    void update_external_faces();

    const BlockFaceV& get_external_faces() const { return external_faces_; }

private:

    bool block_in_range( const Vector3i& index )
    {
        return index[0] >= 0 && index[1] >= 0 && index[2] >= 0 &&
               index[0] < CHUNK_SIZE && index[1] < CHUNK_SIZE && index[2] < CHUNK_SIZE;
    }

    void maybe_add_external_face( const Vector3f& block_position, const Vector3i& block_index, const Block& block, const NeighborRelation relation );
    void add_external_face( const Vector3f& block_position, const Block& block, const NeighborRelation relation );

    Vector3i position_;

    Block blocks_[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

    BlockFaceV external_faces_;

    Chunk* neighbors_[NEIGHBOR_RELATION_SIZE];
};

typedef boost::shared_ptr<Chunk> ChunkSP;
typedef std::vector<ChunkSP> ChunkV;
typedef std::map<Vector3i, ChunkSP, Vector3LexicographicLess<Vector3i> > ChunkMap;

void chunk_stitch_into_map( ChunkSP chunk, ChunkMap& chunks );
void chunk_unstich_from_map( ChunkSP chunk, ChunkMap& chunks );

#endif // CHUNK_H
