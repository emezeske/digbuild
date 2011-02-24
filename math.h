#ifndef MATH_H
#define MATH_H

#include <gmtl/gmtl.h>
#include <algorithm>

typedef float Scalar;

typedef gmtl::Vec<Scalar, 2> Vector2f;
typedef gmtl::Vec<Scalar, 3> Vector3f;

typedef gmtl::Vec<int32_t, 2> Vector2i;
typedef gmtl::Vec<int32_t, 3> Vector3i;

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

#endif // MATH_H
