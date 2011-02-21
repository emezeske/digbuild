#ifndef BLOCK_H
#define BLOCK_H

#include <boost/integer_traits.hpp>

#include <vector>
#include <stdint.h>

struct Block
{
    enum
    {
        MIN_POSITION = boost::integer_traits<uint8_t>::const_min,
        MIN_HEIGHT = 1,
        MAX_HEIGHT = boost::integer_traits<uint8_t>::const_max
    };

    Block( const uint8_t position = 0, const uint8_t height = 1 );

    uint8_t
        position_,
        height_;
};

typedef std::vector<Block> BlockV;

#endif // BLOCK_H
