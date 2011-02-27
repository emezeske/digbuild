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

uint64_t get_seed_for_coordinates( const uint64_t base_seed, const Vector2i position )
{
    return base_seed ^ ( position[0] * 91387 + position[1] * 75181 ); // TODO: Are these prime numbers good?
}

struct BicubicPatchCorner
{
    BicubicPatchCorner( const uint64_t base_seed, const Vector2i& position, const BicubicPatchCornerFeatures& features )
    {
        boost::rand48 generator( get_seed_for_coordinates( base_seed, position ) );
        boost::variate_generator<boost::rand48&, boost::uniform_real<> >
            height_random( generator, boost::uniform_real<>( features.height_range_[0], features.height_range_[1] ) ),
            dx_random    ( generator, boost::uniform_real<>( features.dx_range_[0],     features.dx_range_[1]     ) ),
            dz_random    ( generator, boost::uniform_real<>( features.dz_range_[0],     features.dz_range_[1]     ) ),
            dxz_random   ( generator, boost::uniform_real<>( features.dxz_range_[0],    features.dxz_range_[1]    ) );

        height_ = Scalar( height_random() );
        dx_ = Scalar( dx_random() );
        dz_ = Scalar( dz_random() );
        dxz_ = Scalar( dxz_random() );
    }

    Scalar
        height_,
        dx_,
        dz_,
        dxz_;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for BicubicPatchCornerFeatures:
//////////////////////////////////////////////////////////////////////////////////

BicubicPatchCornerFeatures::BicubicPatchCornerFeatures
(
    const Vector2f& height_range,
    const Vector2f& dx_range,
    const Vector2f& dz_range,
    const Vector2f& dxz_range
) :
    height_range_( height_range ),
    dx_range_( dx_range ),
    dz_range_( dz_range ),
    dxz_range_( dxz_range )
{
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for BicubicPatchFeatures:
//////////////////////////////////////////////////////////////////////////////////

BicubicPatchFeatures::BicubicPatchFeatures
(
    const BicubicPatchCornerFeatures features_ll,
    const BicubicPatchCornerFeatures features_lr,
    const BicubicPatchCornerFeatures features_ul,
    const BicubicPatchCornerFeatures features_ur
) :
    features_ll_( features_ll ),
    features_lr_( features_lr ),
    features_ul_( features_ul ),
    features_ur_( features_ur )
{
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for BicubicPatch:
//////////////////////////////////////////////////////////////////////////////////

BicubicPatch::BicubicPatch
(
    const uint64_t base_seed,
    const Vector2i position,
    const Vector2i size,
    const BicubicPatchFeatures& features
)
{
    const BicubicPatchCorner
        corner_ll( base_seed, position + Vector2i( 0,       0       ), features.features_ll_ ),
        corner_lr( base_seed, position + Vector2i( size[0], 0       ), features.features_lr_ ),
        corner_ul( base_seed, position + Vector2i( 0,       size[1] ), features.features_ul_ ),
        corner_ur( base_seed, position + Vector2i( size[0], size[1] ), features.features_ur_ );

    // Ultimately, to determine the coefficients for the bicubic equation, we use the surface
    // equations P(0,0), P(1,0), P(1,1), and P(0,1) (and similarly, their derivatives in both
    // x, z, and xz) to come up with a set of 16 equations, which can then be solved as a system.
    //
    // The 'x' matrix contains all of the known surface features, and the 'a_inverse' matrix is
    // the inverse of the matrix obtained from the 16 equations mentioned above.  Thus, to solve
    // for the coefficients, we take { beta = inv(a) * x }.

    const float x_data[] =
    {
        corner_ll.height_, corner_lr.height_, corner_ul.height_, corner_ur.height_,
        corner_ll.dx_,     corner_lr.dx_,     corner_ul.dx_,     corner_ur.dx_,
        corner_ll.dz_,     corner_lr.dz_,     corner_ul.dz_,     corner_ur.dz_,
        corner_ll.dxz_,    corner_lr.dxz_,    corner_ul.dxz_,    corner_ur.dxz_
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

Scalar BicubicPatch::interpolate( const Vector2f& position ) const
{
    assert( position[0] >= 0.0f && position[0] <= 1.0 );
    assert( position[1] >= 0.0f && position[1] <= 1.0 );

    // This is simply the expansion of the bicubic equation using the coefficients obtained above.

    const Scalar
        px  = position[0],
        py  = position[1],
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
