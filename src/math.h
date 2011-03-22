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

typedef gmtl::AABoxf AABoxf;
typedef gmtl::AABox<int> AABoxi;

template <typename ToDataType, typename FromDataType, unsigned Size>
gmtl::Vec<ToDataType, Size> vector_cast( const gmtl::Vec<FromDataType, Size>& from )
{
    gmtl::Vec<ToDataType, Size> to;

    for ( unsigned i = 0; i < Size; ++i )
    {
        to[i] = ToDataType( from[i] );
    }

    return to;
}

template <typename DataType, unsigned Size>
gmtl::Vec<DataType, Size> pointwise_product(
        const gmtl::Vec<DataType, Size>& a,
        const gmtl::Vec<DataType, Size>& b
)
{
    gmtl::Vec<DataType, Size> result;

    for ( unsigned i = 0; i < Size; ++i )
    {
        result[i] = a[i] * b[i];
    }

    return result;
}

template <typename DataType, unsigned Size>
gmtl::Vec<DataType, Size> pointwise_floor( const gmtl::Vec<DataType, Size>& v )
{
    gmtl::Vec<DataType, Size> result;

    for ( unsigned i = 0; i < Size; ++i )
    {
        result[i] = gmtl::Math::floor( v[i] );
    }

    return result;
}

template <typename DataType, unsigned Size>
gmtl::Vec<DataType, Size> pointwise_ceil( const gmtl::Vec<DataType, Size>& v )
{
    gmtl::Vec<DataType, Size> result;

    for ( unsigned i = 0; i < Size; ++i )
    {
        result[i] = gmtl::Math::ceil( v[i] );
    }

    return result;
}

template <typename DataType, unsigned Size>
gmtl::Vec<DataType, Size> major_axis( const gmtl::Vec<DataType, Size>& v )
{
    gmtl::Vec<DataType, Size> result;
    DataType max = 0.0f;
    unsigned major = 0;

    for ( unsigned i = 0; i < Size; ++i )
    {
        if ( v[i] > max )
        {
            major = i;
        }
    }

    result[major] = 1.0f;
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
struct VectorLess : public std::binary_function <T, T, bool>
{
    bool operator()( const T& x, const T& y ) const
    {
        for ( unsigned i = 0; i < T::Size; ++i )
        {
            if ( x[i] < y[i] ) return true;
            else if ( x[i] > y[i] ) return false;
        }

        return false;
    }
};

#endif // MATH_H
