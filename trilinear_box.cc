#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>

#include "trilinear_box.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

uint64_t get_seed_for_coordinates( const uint64_t base_seed, const Vector3i position )
{
    return base_seed ^ ( position[0] * 91387 + position[1] * 75181 + position[2] * 40591 ); // TODO: Are these prime numbers good?
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for TrilinearBox:
//////////////////////////////////////////////////////////////////////////////////

TrilinearBox::TrilinearBox()
{
}

TrilinearBox::TrilinearBox(
    const uint64_t base_seed,
    const Vector3i position,
    const Vector3i size,
    const int period
) :
    vertex_field_size_( size[0] / period + 1, size[1] / period + 1, size[2] / period + 1 ),
    period_( period )
{
    assert( size[0] % period == 0 );
    assert( size[1] % period == 0 );
    assert( size[2] % period == 0 );

    vertices_.resize( vertex_field_index( vertex_field_size_ ) + 1 );

    // The PRNG seed for values on the edges of the box need to each be seeded with
    // the positions of the values, to ensure that adjacent TrilinearBoxes will line
    // up seamlessly.  However, seeding things this way results in similarities between
    // neighbors.  Thus, the interior points are not individually seeded -- they use one
    // seed and then successive values from the same PRNG, which yields a better random look.

    boost::uniform_real<> distribution( 0.0f, 1.0f );
    boost::rand48 interior_generator( get_seed_for_coordinates( base_seed, position ) );
    boost::variate_generator<boost::rand48&, boost::uniform_real<> >
        interior_random( interior_generator, distribution );

    for ( int x = 0; x < vertex_field_size_[0]; ++x )
    {
        for ( int y = 0; y < vertex_field_size_[1]; ++y )
        {
            for ( int z = 0; z < vertex_field_size_[2]; ++z )
            {
                const Vector3i index( x, y, z );
                Scalar value;

                if ( x == 0 || y == 0 || z == 0     ||
                     x == vertex_field_size_[0] - 1 ||
                     y == vertex_field_size_[1] - 1 ||
                     z == vertex_field_size_[2] - 1 )
                {
                    boost::rand48 exterior_generator( get_seed_for_coordinates( base_seed, position + index * period ) );
                    boost::variate_generator<boost::rand48&, boost::uniform_real<> >
                        exterior_random( exterior_generator, distribution );
                    value = Scalar( exterior_random() );
                }
                else value = Scalar( interior_random() );

                get_vertex( index ) = value;
            }
        }
    }
}

Scalar TrilinearBox::interpolate( const Vector3f& position ) const
{
    assert( position[0] >= 0.0f && position[0] <= 1.0 );
    assert( position[1] >= 0.0f && position[1] <= 1.0 );
    assert( position[2] >= 0.0f && position[2] <= 1.0 );

    const Vector3f vertex_space_position
    (
        position[0] * Scalar( vertex_field_size_[0] - 1 ),
        position[1] * Scalar( vertex_field_size_[1] - 1 ),
        position[2] * Scalar( vertex_field_size_[2] - 1 )
    );

    const Vector3i vertex_index = vector_cast<Vector3i>( vertex_space_position );

    const Scalar
        p000 = get_vertex( vertex_index + Vector3i( 0, 0, 0 ) ),
        p001 = get_vertex( vertex_index + Vector3i( 0, 0, 1 ) ),
        p010 = get_vertex( vertex_index + Vector3i( 0, 1, 0 ) ),
        p011 = get_vertex( vertex_index + Vector3i( 0, 1, 1 ) ),
        p100 = get_vertex( vertex_index + Vector3i( 1, 0, 0 ) ),
        p101 = get_vertex( vertex_index + Vector3i( 1, 0, 1 ) ),
        p110 = get_vertex( vertex_index + Vector3i( 1, 1, 0 ) ),
        p111 = get_vertex( vertex_index + Vector3i( 1, 1, 1 ) );

    const Vector3f t = vertex_space_position - vector_cast<Vector3f>( vertex_index );

    // NOTE: I tried a few different caching schemes, to potentially avoid recalculating
    //       the first six interpolants here, but all of the schemes ran MUCH more slowly,
    //       likely due to the fact that they used more memory and cache misses ruin any
    //       advantage gained by avoiding a few interpolations.

    Scalar tx00, tx01, tx10, tx11;
    gmtl::Math::lerp( tx00, t[0], p000, p100 );
    gmtl::Math::lerp( tx01, t[0], p001, p101 );
    gmtl::Math::lerp( tx10, t[0], p010, p110 );
    gmtl::Math::lerp( tx11, t[0], p011, p111 );

    Scalar ty0, ty1;
    gmtl::Math::lerp( ty0, t[1], tx00, tx10 );
    gmtl::Math::lerp( ty1, t[1], tx01, tx11 );

    Scalar tz;
    gmtl::Math::lerp( tz, t[2], ty0, ty1 );

    return tz;
}

size_t TrilinearBox::vertex_field_index( const Vector3i& index ) const
{
    return index[0] + index[1] * vertex_field_size_[0] + index[2] * vertex_field_size_[0] * vertex_field_size_[1];
}

Scalar& TrilinearBox::get_vertex( const Vector3i& index )
{
    return vertices_[vertex_field_index( index )];
}

const Scalar& TrilinearBox::get_vertex( const Vector3i& index ) const
{
    return vertices_[vertex_field_index( index )];
}
