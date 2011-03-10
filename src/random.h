#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

#include "math.h"

inline uint64_t get_seed_for_coordinates( const uint64_t base_seed, const Vector3i& position )
{
    return base_seed ^ ( position[0] * 91387 + position[1] * 75181 + position[2] * 40591 ); // TODO: Are these prime numbers good?
}

inline uint64_t get_seed_for_coordinates( const uint64_t base_seed, const Vector2i& position )
{
    return get_seed_for_coordinates( base_seed, Vector3i( position[0], position[1], 0 ) );
}

#endif // RANDOM_H
