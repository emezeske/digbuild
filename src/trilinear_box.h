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

#ifndef TRILINEAR_BOX_H
#define TRILINEAR_BOX_H

#include "math.h"

struct TrilinearBox
{
    TrilinearBox();

    TrilinearBox(
        const uint64_t base_seed,
        const Vector3i position,
        const Vector3i size,
        const int period
    );

    Scalar interpolate( const Vector3f& position ) const;

private:

    size_t vertex_field_index( const Vector3i& index ) const;
    Scalar& get_vertex( const Vector3i& index );
    const Scalar& get_vertex( const Vector3i& index ) const;

    Vector3i vertex_field_size_;

    std::vector<Scalar> vertices_;

    int period_;
};

#endif // TRILINEAR_BOX_H
