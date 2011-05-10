///////////////////////////////////////////////////////////////////////////
// Copyright 2011 Evan Mezeske.
//
// This file is part of Digbuild.
// 
// Digbuild is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.
// 
// Digbuild is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Digbuild.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////

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
