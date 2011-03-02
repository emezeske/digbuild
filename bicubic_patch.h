#ifndef BICUBIC_PATCH_H
#define BICUBIC_PATCH_H

#include "math.h"
#include "stdint.h"

struct BicubicPatchCornerFeatures
{
    BicubicPatchCornerFeatures(
        const Vector2f& height_range,
        const Vector2f& dx_range,
        const Vector2f& dz_range,
        const Vector2f& dxz_range
    );

    Vector2f
        height_range_,
        dx_range_,
        dz_range_,
        dxz_range_;
};

struct BicubicPatchFeatures
{
    BicubicPatchFeatures(
        const BicubicPatchCornerFeatures features_ll,
        const BicubicPatchCornerFeatures features_lr,
        const BicubicPatchCornerFeatures features_ul,
        const BicubicPatchCornerFeatures features_ur
    );

    BicubicPatchCornerFeatures
        features_ll_,
        features_lr_,
        features_ul_,
        features_ur_;
};

struct BicubicPatch
{
    BicubicPatch();

    BicubicPatch(
        const uint64_t base_seed,
        const Vector2i position,
        const Vector2i size,
        const BicubicPatchFeatures& features
    );

    Scalar interpolate( const Vector2f& position ) const;

protected:

    Scalar coefficients_[16];
};

#endif // BICUBIC_PATCH_H
