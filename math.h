#ifndef MATH_H
#define MATH_H

#include <gmtl/gmtl.h>
#include <algorithm>

typedef float Scalar;

// FIXME: Use gmlt::Vec2f, etc.
typedef gmtl::Vec<Scalar, 2> Vector2f;
typedef gmtl::Vec<Scalar, 3> Vector3f;

typedef gmtl::Vec<int32_t, 2> Vector2i;
typedef gmtl::Vec<int32_t, 3> Vector3i;

template <typename To, typename From>
To vector_cast( const From& from )
{
   return To(
        static_cast<typename To::DataType>( from[0] ),
        static_cast<typename To::DataType>( from[1] ),
        static_cast<typename To::DataType>( from[2] )
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
