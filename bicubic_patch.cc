#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>

#include "bicubic_patch.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

gmtl::Matrix<Scalar, 16, 16> make_a_inverse()
{
    const float a_inverse_data[] =
    {
        +1.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0,
        +0.0, +0.0, +0.0, +0.0, +1.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0,
        -3.0, +3.0, +0.0, +0.0, -2.0, -1.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0,
        +2.0, -2.0, +0.0, +0.0, +1.0, +1.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0,
        +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +1.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0,
        +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +1.0, +0.0, +0.0, +0.0,
        +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, -3.0, +3.0, +0.0, +0.0, -2.0, -1.0, +0.0, +0.0,
        +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +0.0, +2.0, -2.0, +0.0, +0.0, +1.0, +1.0, +0.0, +0.0,
        -3.0, +0.0, +3.0, +0.0, +0.0, +0.0, +0.0, +0.0, -2.0, +0.0, -1.0, +0.0, +0.0, +0.0, +0.0, +0.0,
        +0.0, +0.0, +0.0, +0.0, -3.0, +0.0, +3.0, +0.0, +0.0, +0.0, +0.0, +0.0, -2.0, +0.0, -1.0, +0.0,
        +9.0, -9.0, -9.0, +9.0, +6.0, +3.0, -6.0, -3.0, +6.0, -6.0, +3.0, -3.0, +4.0, +2.0, +2.0, +1.0,
        -6.0, +6.0, +6.0, -6.0, -3.0, -3.0, +3.0, +3.0, -4.0, +4.0, -2.0, +2.0, -2.0, -2.0, -1.0, -1.0,
        +2.0, +0.0, -2.0, +0.0, +0.0, +0.0, +0.0, +0.0, +1.0, +0.0, +1.0, +0.0, +0.0, +0.0, +0.0, +0.0,
        +0.0, +0.0, +0.0, +0.0, +2.0, +0.0, -2.0, +0.0, +0.0, +0.0, +0.0, +0.0, +1.0, +0.0, +1.0, +0.0,
        -6.0, +6.0, +6.0, -6.0, -4.0, -2.0, +4.0, +2.0, -3.0, +3.0, -3.0, +3.0, -2.0, -1.0, -2.0, -1.0,
        +4.0, -4.0, -4.0, +4.0, +2.0, +2.0, -2.0, -2.0, +2.0, -2.0, +2.0, -2.0, +1.0, +1.0, +1.0, +1.0
    };

    gmtl::Matrix<Scalar, 16, 16> m;
    m.setTranspose( a_inverse_data );
    return m;
}

const gmtl::Matrix<Scalar, 16, 16> A_INVERSE = make_a_inverse();

uint64_t get_seed_for_coordinates( const uint64_t world_seed, const Vector2i position )
{
    return world_seed ^ ( position[0] * 91387 + position[1] * 75181 ); // TODO: Are these prime numbers good?
}

struct BicubicPatchCorner
{
    BicubicPatchCorner( const uint64_t world_seed, const Vector2i position )
    {
        boost::rand48 generator( get_seed_for_coordinates( world_seed, position ) );
        boost::uniform_real<> distribution( 1, 128 );
        boost::variate_generator<boost::rand48&, boost::uniform_real<> > random( generator, distribution );
        height_ = Scalar( random() );
        dx_ = Scalar( random() );
        dz_ = Scalar( random() );
        dxz_ = Scalar( random() );
    }

    Scalar
        height_,
        dx_,
        dz_,
        dxz_;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for BicubicPatch:
//////////////////////////////////////////////////////////////////////////////////

BicubicPatch::BicubicPatch( const uint64_t world_seed, const Vector2i position, const Vector2i size )
{
    const BicubicPatchCorner
        corner_a( world_seed, position + Vector2i( 0,       0       ) ),
        corner_b( world_seed, position + Vector2i( size[0], 0       ) ),
        corner_c( world_seed, position + Vector2i( 0,       size[1] ) ),
        corner_d( world_seed, position + Vector2i( size[0], size[1] ) );

    // Ultimately, to determine the coefficients for the bicubic equation, we use the surface
    // equations P(0,0), P(1,0), P(1,1), and P(0,1) (and similarly, their derivatives in both
    // x, z, and xz) to come up with a set of 16 equations, which can then be solved as a system.
    //
    // The 'x' matrix contains all of the known surface features, and the 'a_inverse' matrix is
    // the inverse of the matrix obtained from the 16 equations mentioned above.  Thus, to solve
    // for the coefficients, we take { beta = inv(a) * x }.

    const float x_data[] =
    {
        corner_a.height_, corner_b.height_, corner_c.height_, corner_d.height_,
        corner_a.dx_, corner_b.dx_, corner_c.dx_, corner_d.dx_,
        corner_a.dz_, corner_b.dz_, corner_c.dz_, corner_d.dz_,
        corner_a.dxz_, corner_b.dxz_, corner_c.dxz_, corner_d.dxz_
    };

    gmtl::Matrix<Scalar, 16, 1> x;
    x.set( x_data );

    gmtl::Matrix<Scalar, 16, 1> beta_;
    gmtl::mult( beta_, A_INVERSE, x );

    for ( int i = 0; i < 16; ++i )
    {
        coefficients_[i]  = beta_[i][0];
    }
}

Scalar BicubicPatch::interpolate( const Scalar px, const Scalar py ) const
{
    // This is simply the expansion of the bicubic equation using the coefficients obtained above.

    const Scalar
        px2 = px * px,
        px3 = px * px2,
        py2 = py * py,
        py3 = py * py2;

    return
        coefficients_[0]  +
        coefficients_[1]  * px +
        coefficients_[2]  * px2 +
        coefficients_[3]  * px3 +
        coefficients_[4]  * py +
        coefficients_[5]  * px * py +
        coefficients_[6]  * px2 * py +
        coefficients_[7]  * px3 * py +
        coefficients_[8]  * py2 +
        coefficients_[9]  * px * py2 +
        coefficients_[10] * px2 * py2 +
        coefficients_[11] * px3 * py2 +
        coefficients_[12] * py3 +
        coefficients_[13] * px * py3 +
        coefficients_[14] * px2 * py3 +
        coefficients_[15] * px3 * py3;
}
