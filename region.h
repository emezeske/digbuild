#ifndef REGION_H
#define REGION_H

#include <GL/gl.h>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/pool/pool.hpp>

#include "math.h"
#include "block.h"

struct Region
{
    enum
    {
        REGION_SIZE = 64
    };

    Region( const uint64_t base_seed, const Vector2i position );

    void add_block_to_column( const Vector2i index, Block* block );

    const Block* get_column( const Vector2i index ) const
    {
        assert( index[0] >= 0 );
        assert( index[1] >= 0 );
        assert( index[0] < REGION_SIZE );
        assert( index[1] < REGION_SIZE );

        return columns_[index[0]][index[1]];
    }

    Vector2i position_;

protected:

    boost::pool<> block_pool_;

    Block* columns_[REGION_SIZE][REGION_SIZE];
};

typedef boost::shared_ptr<Region> RegionSP;
typedef std::map<Vector2i, RegionSP, Vector2LexicographicLess<Vector2i> > RegionMap;

#endif // REGION_H
