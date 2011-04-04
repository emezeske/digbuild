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
unsigned major_axis( const gmtl::Vec<DataType, Size>& v )
{
    DataType max = 0.0f;
    unsigned major = 0;

    for ( unsigned i = 0; i < Size; ++i )
    {
        if ( gmtl::Math::abs( v[i] ) > max )
        {
            major = i;
        }
    }

    return major;
}

template <typename DataType, unsigned Size>
DataType max_component_magnitude( const gmtl::Vec<DataType, Size>& v )
{
    DataType max = DataType( 0 );

    for ( unsigned i = 0; i < Size; ++i )
    {
        const DataType magnitude = gmtl::Math::abs( v[i] );

        if ( magnitude > max )
        {
            max = magnitude;
        }
    }

    return max;
}

template <typename DataType, unsigned Size>
DataType min_component( const gmtl::Vec<DataType, Size>& v )
{
    DataType min = v[0];

    for ( unsigned i = 1; i < Size; ++i )
    {
        if ( v[i] < min )
        {
            min = v[i];
        }
    }

    return min;
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

// Given two AABoxes that are in contact, and the normal of the contact face, this function
// will return the minimum amount by which the Block's faces overlap (in two dimensions).
template <typename DataType>
DataType min_planar_overlap( const gmtl::AABox<DataType>& a, const gmtl::AABox<DataType>& b, const gmtl::Vec<DataType, 3>& normal )
{
    const unsigned ignore_axis = major_axis( normal );

    gmtl::Vec<DataType, 3> da( a.getMax() - b.getMin() );
    gmtl::Vec<DataType, 3> db( b.getMax() - a.getMin() );
    gmtl::Vec<DataType, 3> dr;

    for ( unsigned i = 0; i < 3; ++i )
    {
        if ( i != ignore_axis )
        {
            da[i] = da[i] > DataType( 0 ) ? da[i] : DataType( 0 );
            db[i] = db[i] > DataType( 0 ) ? db[i] : DataType( 0 );
            dr[i] = gmtl::Math::Min( da[i], db[i] );
        }
        else dr[i] = std::numeric_limits<DataType>::max();
    }

    return min_component( dr );
}

namespace gmtl
{
   // FIXME:  There is a bug in gmtl's default intersection function that causes it to
   //         falsely report intersections if any component of the relative path between
   //         box1 and box2 is zero.  I submitted a bug report with a patch, but until it
   //         is merged in, I'm using this locally fixed version.

    // FIXME: This function is really evolving into its own thing.  Maybe take it out
    //        of the gmtl namespace, and stop calling it a bugfix.  And clean it up!
    //
    // TODO: Actually, is it broken at all?

   template<class DATA_TYPE>
   bool intersect_bugfix( const AABox<DATA_TYPE>& box1, const Vec<DATA_TYPE, 3>& path1,
                          const AABox<DATA_TYPE>& box2,
                          DATA_TYPE& intersection_time )
   {
      // Get the relative path (in normalized time)
      const Vec<DATA_TYPE, 3> path = -path1;

      Vec<DATA_TYPE, 3> axis_intersection_time(DATA_TYPE(0), DATA_TYPE(0), DATA_TYPE(0));

      Vec<DATA_TYPE, 3> axis_gap_time(DATA_TYPE(1), DATA_TYPE(1), DATA_TYPE(1));

      for (int i=0; i<3; ++i)
      {
         if ((box1.getMax()[i] < box2.getMin()[i]))
         {
            if (path[i] < DATA_TYPE(0))
            {
                axis_intersection_time[i] = (box1.getMax()[i] - box2.getMin()[i]) / path[i];
            }
            else return false;
         }
         else if ((box2.getMax()[i] < box1.getMin()[i]))
         {
            if (path[i] > DATA_TYPE(0))
            {
                axis_intersection_time[i] = (box1.getMin()[i] - box2.getMax()[i]) / path[i];
            }
            else return false;
         }
         else axis_intersection_time[i] = DATA_TYPE(0);

         if ((box2.getMax()[i] > box1.getMin()[i]) && (path[i] < DATA_TYPE(0)))
         {
            axis_gap_time[i] = (box1.getMin()[i] - box2.getMax()[i]) / path[i];
         }
         else if ((box1.getMax()[i] > box2.getMin()[i]) && (path[i] > DATA_TYPE(0)))
         {
            axis_gap_time[i] = (box1.getMax()[i] - box2.getMin()[i]) / path[i];
         }
      }

      intersection_time = Math::Max(axis_intersection_time[0], axis_intersection_time[1], axis_intersection_time[2]);
      const DATA_TYPE gap_time = Math::Min(axis_gap_time[0], axis_gap_time[1], axis_gap_time[2]);

      return intersection_time <= gap_time;
   }
}

#endif // MATH_H
