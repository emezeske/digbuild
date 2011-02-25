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

TrilinearBox::TrilinearBox
(
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

    vertex_field_.resize( vertex_field_index( vertex_field_size_ ) + 1 );

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

Scalar TrilinearBox::interpolate( const Scalar px, const Scalar py, const Scalar pz ) const
{
    assert( px >= 0.0f && px <= 1.0 && py >= 0.0f && py <= 1.0 && pz >= 0.0f && pz <= 1.0f );

    const Vector3f q
    (
        px * Scalar( vertex_field_size_[0] - 1 ),
        py * Scalar( vertex_field_size_[1] - 1 ),
        pz * Scalar( vertex_field_size_[2] - 1 )
    );

    const Vector3i p
    (
        int( gmtl::Math::trunc( q[0] ) ),
        int( gmtl::Math::trunc( q[1] ) ),
        int( gmtl::Math::trunc( q[2] ) )
    );

    const Scalar
        p000 = get_vertex( p + Vector3i( 0, 0, 0 ) ),
        p001 = get_vertex( p + Vector3i( 0, 0, 1 ) ),
        p010 = get_vertex( p + Vector3i( 0, 1, 0 ) ),
        p011 = get_vertex( p + Vector3i( 0, 1, 1 ) ),
        p100 = get_vertex( p + Vector3i( 1, 0, 0 ) ),
        p101 = get_vertex( p + Vector3i( 1, 0, 1 ) ),
        p110 = get_vertex( p + Vector3i( 1, 1, 0 ) ),
        p111 = get_vertex( p + Vector3i( 1, 1, 1 ) );

    const Vector3f t = q - Vector3f( Scalar( p[0] ), Scalar( p[1] ), Scalar( p[2] ) );

    // TODO: Possible optimisation: the txNN and tyM values could be precomputed for
    //       every cell in the TrilinearBox.  That could save a LOT of work.

    Scalar
        tx00,
        tx01,
        tx10,
        tx11;

    gmtl::Math::lerp( tx00, t[0], p000, p100 );
    gmtl::Math::lerp( tx01, t[0], p001, p101 );
    gmtl::Math::lerp( tx10, t[0], p010, p110 );
    gmtl::Math::lerp( tx11, t[0], p011, p111 );

    Scalar
        ty0,
        ty1;

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
    return vertex_field_[vertex_field_index( index )];
}

const Scalar& TrilinearBox::get_vertex( const Vector3i& index ) const
{
    return vertex_field_[vertex_field_index( index )];
}
