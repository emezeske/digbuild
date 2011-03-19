#ifndef MATH_H
#define MATH_H

#include <gmtl/gmtl.h>
#include <algorithm>

typedef float Scalar;

// FIXME: Use gmlt::Vec2f, etc.
typedef gmtl::Vec<Scalar, 2> Vector2f;
typedef gmtl::Vec<Scalar, 3> Vector3f;
typedef gmtl::Vec<Scalar, 4> Vector4f;

typedef gmtl::Vec<int32_t, 2> Vector2i;
typedef gmtl::Vec<int32_t, 3> Vector3i;
typedef gmtl::Vec<int32_t, 4> Vector4i;

template <typename ToDataType, typename FromDataType, unsigned Size>
gmtl::VecBase<ToDataType, Size> vector_cast( const gmtl::VecBase<FromDataType, Size>& from )
{
    gmtl::VecBase<ToDataType, Size> to;

    for ( unsigned i = 0; i < Size; ++i )
    {
        to[i] = ToDataType( from[i] );
    }

    return to;
}

template <typename DataType, unsigned Size>
gmtl::VecBase<DataType, Size> pointwise_product(
        const gmtl::VecBase<DataType, Size>& a,
        const gmtl::VecBase<DataType, Size>& b
)
{
    gmtl::VecBase<DataType, Size> result;

    for ( unsigned i = 0; i < Size; ++i )
    {
        result[i] = DataType( a[i] * b[i] );
    }

    return result;
}

template <typename DataType, unsigned Size>
gmtl::VecBase<DataType, Size> pointwise_quotient(
        const gmtl::VecBase<DataType, Size>& n,
        const gmtl::VecBase<DataType, Size>& d
)
{
    gmtl::VecBase<DataType, Size> result;

    for ( unsigned i = 0; i < Size; ++i )
    {
        result[i] = DataType( n[i] / d[i] );
    }

    return result;
}

inline Vector3f spherical_to_cartesian( const Vector3f& spherical )
{
    return Vector3f(
        spherical[0] * gmtl::Math::sin( spherical[1] ) * gmtl::Math::sin( spherical[2] ),
        spherical[0] * gmtl::Math::cos( spherical[1] ),
        spherical[0] * gmtl::Math::sin( spherical[1] ) * gmtl::Math::cos( spherical[2] )
    );
}

template <typename T>
struct Vector2LexicographicLess : std::binary_function <T, T, bool>
{
    bool operator()( const T& x, const T& y ) const
    {
        if ( x[0] < y[0] )
            return true;
        if ( x[0] > y[0] )
            return false;
        if ( x[1] < y[1] )
            return true;
        return false;
    }
};

template <typename T>
struct Vector3LexicographicLess : std::binary_function <T, T, bool>
{
    bool operator()( const T& x, const T& y ) const
    {
        if ( x[0] < y[0] )
            return true;
        if ( x[0] > y[0] )
            return false;
        if ( x[1] < y[1] )
            return true;
        if ( x[1] > y[1] )
            return false;
        if ( x[2] < y[2] )
            return true;
        return false;
    }
};

#endif // MATH_H
