#ifndef REGION_H
#define REGION_H

#include <GL/gl.h>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/pool/pool.hpp>

#include "math.h"
#include "block.h"

struct Chunk
{
    enum { CHUNK_SIZE = 32 };

    Chunk();

    void add_block_to_column( const Vector2i index, Block* block );

    const Block* get_column( const Vector2i index ) const
    {
        assert( index[0] >= 0 );
        assert( index[1] >= 0 );
        assert( index[0] < CHUNK_SIZE );
        assert( index[1] < CHUNK_SIZE );

        return columns_[index[0]][index[1]];
    }

private:

    Block* columns_[CHUNK_SIZE][CHUNK_SIZE];
};

typedef boost::shared_ptr<Chunk> ChunkSP;
typedef std::map<Vector3i, ChunkSP, Vector3LexicographicLess<Vector3i> > ChunkMap;

struct Region
{
    enum
    {
        CHUNKS_PER_EDGE = 4,
        REGION_SIZE = CHUNKS_PER_EDGE * Chunk::CHUNK_SIZE
    };

    Region( const uint64_t base_seed, const Vector2i position );

    void add_block_to_column( const Vector2i column_index, const unsigned bottom, const unsigned top, const BlockMaterial material );

    const ChunkMap& chunks() const { return chunks_; }

    const Vector2i& position() const { return position_; }

protected:

    boost::pool<> block_pool_;

    ChunkMap chunks_;

    Vector2i position_;
};

typedef boost::shared_ptr<Region> RegionSP;
typedef std::map<Vector2i, RegionSP, Vector2LexicographicLess<Vector2i> > RegionMap;

#endif // REGION_H
