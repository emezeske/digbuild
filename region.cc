#include <assert.h>

#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>

#include "region.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

struct BicubicInterpolator
{
    BicubicInterpolator(
        const Scalar y_a, // The height of each corner.
        const Scalar y_b,
        const Scalar y_c,
        const Scalar y_d,

        const Scalar dx_a, // The derivative with respect to x for each corner.
        const Scalar dx_b,
        const Scalar dx_c,
        const Scalar dx_d,

        const Scalar dz_a, // The derivative with respect to z for each corner.
        const Scalar dz_b,
        const Scalar dz_c,
        const Scalar dz_d,

        const Scalar dxz_a, // The cross derivative with respect to x and z for each corner.
        const Scalar dxz_b,
        const Scalar dxz_c,
        const Scalar dxz_d
    )
    {
        // Ultimately, to determine the coefficients for the bicubic equation, we use the surface
        // equations P(0,0), P(1,0), P(1,1), and P(0,1) (and similarly, their derivatives in both
        // x, z, and xz) to come up with a set of 16 equations, which can then be solved as a system.
        //
        // The 'x' matrix contains all of the known surface features, and the 'a_inverse' matrix is
        // the inverse of the matrix obtained from the 16 equations mentioned above.  Thus, to solve
        // for the coefficients, we take { beta = inv(a) * x }.

        const float x_data[] =
        {
            y_a, y_b, y_c, y_d, dx_a, dx_b, dx_c, dx_d, dz_a, dz_b, dz_c, dz_d, dxz_a, dxz_b, dxz_c, dxz_d
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

    Scalar interpolate( const Scalar px, const Scalar py ) const
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

protected:

    static gmtl::Matrix<Scalar, 16, 16> make_a_inverse()
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

    static const gmtl::Matrix<Scalar, 16, 16> A_INVERSE;

    Scalar coefficients_[16];
};

const gmtl::Matrix<Scalar, 16, 16> BicubicInterpolator::A_INVERSE = make_a_inverse();

uint64_t get_seed_for_coordinates( const uint64_t world_seed, const Vector2i position )
{
    return world_seed ^ ( position[0] * 91387 + position[1] * 75181 ); // TODO: Are these prime numbers good?
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for RegionCorner:
//////////////////////////////////////////////////////////////////////////////////

RegionCorner::RegionCorner( const uint64_t world_seed, const Vector2i position )
{
    boost::rand48 generator( get_seed_for_coordinates( world_seed, position ) );
    boost::uniform_real<> distribution( 1, 128 );
    boost::variate_generator<boost::rand48&, boost::uniform_real<> > random( generator, distribution );
    height_ = Scalar( random() );
    dx_ = Scalar( random() );
    dz_ = Scalar( random() );
    dxz_ = Scalar( random() );
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Region:
//////////////////////////////////////////////////////////////////////////////////

Region::Region( const uint64_t world_seed, const Vector2i position ) :
    position_( position ),
    corner_a_( world_seed, position + Vector2i( 0, 0 ) ),
    corner_b_( world_seed, position + Vector2i( 1, 0 ) ),
    corner_c_( world_seed, position + Vector2i( 0, 1 ) ),
    corner_d_( world_seed, position + Vector2i( 1, 1 ) )
{
    const BicubicInterpolator interpolator(
        corner_a_.height_, corner_b_.height_, corner_c_.height_, corner_d_.height_,
        corner_a_.dx_,     corner_b_.dx_,     corner_c_.dx_,     corner_d_.dx_,
        corner_a_.dz_,     corner_b_.dz_,     corner_c_.dz_,     corner_d_.dz_,
        corner_a_.dxz_,    corner_b_.dxz_,    corner_c_.dxz_,    corner_d_.dxz_
    );

    for ( size_t x = 0; x < BLOCKS_PER_EDGE; ++x )
    {
        for ( size_t z = 0; z < BLOCKS_PER_EDGE; ++z )
        {
            Scalar height = interpolator.interpolate( 
                static_cast<float>( x ) / BLOCKS_PER_EDGE,
                static_cast<float>( z ) / BLOCKS_PER_EDGE
            );

            height = std::max( height, Scalar( Block::MIN_HEIGHT ) );
            height = std::min( height, Scalar( Block::MAX_HEIGHT ) );

            const Block block( 0, uint8_t( gmtl::Math::round( height * 0.25 ) ) );
            const Vector2i column( x % Chunk::BLOCKS_PER_EDGE, z % Chunk::BLOCKS_PER_EDGE );
            chunks_[x / Chunk::BLOCKS_PER_EDGE][z / Chunk::BLOCKS_PER_EDGE].add_block_to_column( column, block );

            const Block block2( uint8_t( gmtl::Math::round( height * 0.75 ) ), uint8_t( gmtl::Math::round( height ) ) );
            chunks_[x / Chunk::BLOCKS_PER_EDGE][z / Chunk::BLOCKS_PER_EDGE].add_block_to_column( column, block2 );
        }
    }
}

const Chunk& Region::get_chunk( const Vector2i chunk ) const
{
    assert( chunk[0] >= 0 );
    assert( chunk[1] >= 0 );
    assert( chunk[0] < BLOCKS_PER_EDGE );
    assert( chunk[1] < BLOCKS_PER_EDGE );

    return chunks_[chunk[0]][chunk[1]];
}
