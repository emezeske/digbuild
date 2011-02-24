#ifndef TRILINEAR_BOX_H
#define TRILINEAR_BOX_H

#include "math.h"

struct TrilinearBox
{
    TrilinearBox
    (
        const uint64_t base_seed,
        const Vector3i position,
        const Vector3i size,
        const int period
    );

    Scalar interpolate( const Scalar px, const Scalar py, const Scalar pz ) const;

private:

    size_t vertex_field_index( const Vector3i& index ) const;

    Scalar& get_vertex( const Vector3i& index );
    const Scalar& get_vertex( const Vector3i& index ) const;

    Vector3i vertex_field_size_;

    std::vector<Scalar> vertex_field_;

    int period_;
};

#endif // TRILINEAR_BOX_H
