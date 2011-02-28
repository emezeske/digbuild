#ifndef REGION_H
#define REGION_H

#include <GL/gl.h>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/pool/pool.hpp>

#include "math.h"
#include "block.h"

struct Region;
typedef boost::shared_ptr<Region> RegionSP;
typedef std::map<Vector2i, RegionSP, Vector2LexicographicLess<Vector2i> > RegionMap;

struct Chunk
{
    enum { CHUNK_SIZE = 16 };

    Chunk();

    void add_block_to_column( const Vector2i& index, Block* block );

    const Block* get_column( const Vector2i& index ) const
    {
        assert( index[0] >= 0 );
        assert( index[1] >= 0 );
        assert( index[0] < CHUNK_SIZE );
        assert( index[1] < CHUNK_SIZE );

        return columns_[index[0]][index[1]];
    }

    void update_external_faces(
        const Vector3i& chunk_index,
        const Region& region,
        const RegionMap& regions
    );

    const BlockFaceV& external_faces() const { return external_faces_; }

private:

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

    enum FaceDirection
    {
        FACE_DIRECTION_NORTH,
        FACE_DIRECTION_SOUTH,
        FACE_DIRECTION_EAST,
        FACE_DIRECTION_WEST,
        FACE_DIRECTION_UP,
        FACE_DIRECTION_DOWN
    };

    void update_column_faces(
        const Vector3i& chunk_position,
        const Chunk* neighbor_chunks[NEIGHBOR_RELATION_SIZE]
    );

    void add_block_faces(
        const Vector3i column_world_position,
        const Block& block,
        const Block* adjacent_column,
        const FaceDirection direction
    );

    void add_block_endcaps(
        const Vector3i column_world_position,
        const Block& block,
        const Block* lower_block,
        const Block* neighbor_columns[NEIGHBOR_RELATION_SIZE]
    );

    void add_block_face(
        const Vector3i column_world_position,
        const Block::HeightT block_bottom, 
        const Block::HeightT block_top, 
        const BlockMaterial material,
        const FaceDirection direction
    );

    Block* columns_[CHUNK_SIZE][CHUNK_SIZE];

    BlockFaceV external_faces_;
};

typedef boost::shared_ptr<Chunk> ChunkSP;
typedef std::map<Vector3i, ChunkSP, Vector3LexicographicLess<Vector3i> > ChunkMap;

struct Region
{
    enum
    {
        CHUNKS_PER_EDGE = 8,
        REGION_SIZE = CHUNKS_PER_EDGE * Chunk::CHUNK_SIZE
    };

    Region( const uint64_t base_seed, const Vector2i position );

    void add_block_to_column( const Vector2i& column_index, const unsigned bottom, const unsigned top, const BlockMaterial material );
    const ChunkMap& chunks() const { return chunks_; }
    ChunkMap& chunks() { return chunks_; } // TODO: Maybe remove this?
    const Vector2i& position() const { return position_; }

protected:

    boost::pool<> block_pool_;

    ChunkMap chunks_;

    Vector2i position_;
};

#endif // REGION_H
