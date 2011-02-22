#ifndef BICUBIC_PATCH_H
#define BICUBIC_PATCH_H

#include "math.h"
#include "stdint.h"

struct BicubicPatch
{
    // TODO: Parameterize height / derivative distribution

    BicubicPatch( const uint64_t world_seed, const Vector2i position, const Vector2i size );

    Scalar interpolate( const Scalar px, const Scalar py ) const;

protected:

    Scalar coefficients_[16];
};

#endif // BICUBIC_PATCH_H
