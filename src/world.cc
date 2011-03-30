#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/uniform_on_sphere.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/foreach.hpp>

#include "world.h"
#include "sdl_utilities.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

enum SkyMode
{
    SKY_MODE_MIDDAY,
    SKY_MODE_SUNSET,
    SKY_MODE_NIGHT
};

const Sky::Profile SKY_PROFILES[] =
{
    Sky::Profile( // SKY_MODE_MIDDAY
        0.0f,
        Vector3f( 0.10f, 0.36f, 0.61f ),
        Vector3f( 0.81f, 0.89f, 0.89f ),
        Vector3f( 1.00f, 1.00f, 1.00f ),
        Vector3f( 1.00f, 1.00f, 1.00f ),
        Vector3f( 1.00f, 1.00f, 1.00f ),
        Vector3f( 0.00f, 0.00f, 0.00f )
    ),
    Sky::Profile( // SKY_MODE_SUNSET
        0.0f,
        Vector3f( 0.68f, 0.00f, 1.00f ),
        Vector3f( 1.00f, 0.41f, 0.00f ),
        Vector3f( 1.00f, 0.41f, 0.10f ),
        Vector3f( 1.00f, 0.97f, 0.29f ),
        Vector3f( 1.00f, 1.00f, 1.00f ),
        Vector3f( 0.00f, 0.00f, 0.00f )
    ),
    Sky::Profile( // SKY_MODE_NIGHT
        1.0f,
        Vector3f( 0.00f, 0.00f, 0.00f ),
        Vector3f( 0.05f, 0.18f, 0.30f ),
        Vector3f( 1.00f, 0.41f, 0.10f ),
        Vector3f( 0.00f, 0.00f, 0.00f ),
        Vector3f( 1.00f, 1.00f, 1.00f ),
        Vector3f( 0.20f, 0.20f, 0.50f )
    )
};

bool highest_chunk( const Chunk* a, const Chunk* b )
{
    return a->get_position()[1] > b->get_position()[1];
}

void generate_region_concurrently( WorldGenerator& generator, const Vector2i& position, ChunkMap& chunks )
{
    static boost::mutex lock;

    ChunkSPV new_chunks = generator.generate_region( position );

    boost::lock_guard<boost::mutex> guard( lock );

    BOOST_FOREACH( ChunkSP chunk, new_chunks )
    {
        chunk_stitch_into_map( chunk, chunks );
    }
}

bool are_chunks_doubly_separated( const Chunk& chunk_a, const Chunk& chunk_b )
{
    const Vector3i& a = chunk_a.get_position();
    const Vector3i& b = chunk_b.get_position();

    for ( int i = 0; i < Vector3i::Size; ++i )
    {
        if ( abs( a[i] - b[i] ) > Chunk::SIZE[i] * 3 )
        {
            return true;
        }
    }

    return false;
}

// This function takes a set of Chunks, and returns a subset of them that are spaced
// out such that apply_lighting_to_neighbors() may be safely called on them in parallel.
void find_doubly_separated_chunks( const ChunkSet& chunks, ChunkSet& separated_chunks )
{
    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        bool separated = true;

        BOOST_FOREACH( Chunk* separated_chunk, separated_chunks )
        {
            if ( !are_chunks_doubly_separated( *chunk, *separated_chunk ) )
            {
                separated = false;
                break;
            }
        }

        if ( separated )
        {
            separated_chunks.insert( chunk );
        }
    }
}

bool reset_changes_base_sunlight( Chunk& chunk )
{
    bool base_sunlight[Chunk::SIZE_X][Chunk::SIZE_Z];

    for ( int x = 0; x < Chunk::SIZE_X; ++x )
    {
        for ( int z = 0; z < Chunk::SIZE_Z; ++z )
        {
            Block& block = chunk.get_block( Vector3i( x, 0, z ) );
            base_sunlight[x][z] = block.is_sunlight_source();
        }
    }

    chunk.reset_lighting();

    for ( int x = 0; x < Chunk::SIZE_X; ++x )
    {
        for ( int z = 0; z < Chunk::SIZE_Z; ++z )
        {
            Block& block = chunk.get_block( Vector3i( x, 0, z ) );

            if ( base_sunlight[x][z] != block.is_sunlight_source() )
            {
                return true;
            }
        }
    }

    return false;
}

void add_chunks_affected_by_sunlight( ChunkSet& chunks )
{
    ChunkV height_sorted_chunks;

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        height_sorted_chunks.push_back( chunk );
    }

    std::sort( height_sorted_chunks.begin(), height_sorted_chunks.end(), highest_chunk );

    BOOST_FOREACH( Chunk* chunk, height_sorted_chunks )
    {
        Chunk* next = chunk;

        while ( next )
        {
            if ( reset_changes_base_sunlight( *next ) )
            {
                next = next->get_neighbor( cardinal_relation_vector( CARDINAL_RELATION_BELOW ) );

                if ( chunks.find( next ) != chunks.end() )
                {
                    next = 0;
                }

                chunks.insert( next );
            }
            else next = 0;
        }
    }
}

unsigned hardware_concurrency()
{
    const unsigned concurrency = boost::thread::hardware_concurrency();
    return concurrency == 0 ? 1 : concurrency;
}

} // anonymous namespace

Sky::Profile::Profile(
    const Scalar star_intensity,
    const Vector3f& zenith_color,
    const Vector3f& horizon_color,
    const Vector3f& sun_color,
    const Vector3f& sun_light_color,
    const Vector3f& moon_color,
    const Vector3f& moon_light_color
) :
    star_intensity_( star_intensity ),
    zenith_color_( zenith_color ),
    horizon_color_( horizon_color ),
    sun_color_( sun_color ),
    sun_light_color_( sun_light_color ),
    moon_color_( moon_color ),
    moon_light_color_( moon_light_color )
{
}

Sky::Profile Sky::Profile::lerp( const Scalar t, const Profile& other )const
{
    Profile result;
    gmtl::Math::lerp( result.star_intensity_, t, star_intensity_, other.star_intensity_ );
    gmtl::lerp( result.zenith_color_, t, zenith_color_, other.zenith_color_ );
    gmtl::lerp( result.horizon_color_, t, horizon_color_, other.horizon_color_ );
    gmtl::lerp( result.sun_color_, t, sun_color_, other.sun_color_ );
    gmtl::lerp( result.sun_light_color_, t, sun_light_color_, other.sun_light_color_ );
    gmtl::lerp( result.moon_color_, t, moon_color_, other.moon_color_ );
    gmtl::lerp( result.moon_light_color_, t, moon_light_color_, other.moon_light_color_ );
    return result;
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Sky:
//////////////////////////////////////////////////////////////////////////////////

Sky::Sky( const uint64_t world_seed ) :
    time_of_day_( 0.25f ),
    profile_( SKY_PROFILES[SKY_MODE_NIGHT] ),
    sun_angle_( 0.0f, 0.0f ),
    moon_angle_( 0.0f, 0.0f )
{
    boost::rand48 generator( world_seed );

    boost::variate_generator<boost::rand48&, boost::uniform_int<> >
        num_stars_random( generator, boost::uniform_int<>( 1000, 1500 ) );

    boost::variate_generator<boost::rand48&, boost::uniform_real<> >
        size_random( generator, boost::uniform_real<>( 0.01f, 0.03f ) );

    boost::variate_generator<boost::rand48&, boost::uniform_on_sphere<> >
        sphere_random( generator, boost::uniform_on_sphere<>( 3 ) );

    const int num_stars = num_stars_random();

    for ( int i = 0; i < num_stars; ++i )
    {
        const std::vector<double> on_sphere = sphere_random();
        stars_.push_back(
            Vector3f(
                size_random(),
                gmtl::Math::aCos( on_sphere[2] ),
                gmtl::Math::aTan2( on_sphere[1], on_sphere[0] )
            )
        );
    }
}

void Sky::do_one_step( const float step_time )
{
    time_of_day_ += DAY_CYCLE_SPEED * step_time;
    time_of_day_ -= gmtl::Math::floor( time_of_day_ );

    // TODO: This is pretty ugly.  It would be nice to make it a bit more general.

    if ( time_of_day_ > 0.20 && time_of_day_ <= 0.30 )
    {
        const Scalar t = ( time_of_day_ - 0.20 ) / 0.10f;
        profile_ = SKY_PROFILES[SKY_MODE_NIGHT].lerp( t, SKY_PROFILES[SKY_MODE_SUNSET] );
    }
    else if ( time_of_day_ > 0.30f && time_of_day_ <= 0.35 )
    {
        const Scalar t = ( time_of_day_ - 0.30f ) / 0.05f;
        profile_ = SKY_PROFILES[SKY_MODE_SUNSET].lerp( t, SKY_PROFILES[SKY_MODE_MIDDAY] );
    }
    else if ( time_of_day_ > 0.65 && time_of_day_ <= 0.70f )
    {
        const Scalar t = ( time_of_day_ - 0.65f ) / 0.05f;
        profile_ = SKY_PROFILES[SKY_MODE_MIDDAY].lerp( t, SKY_PROFILES[SKY_MODE_SUNSET] );
    }
    else if ( time_of_day_ > 0.70f && time_of_day_ <= 0.80f )
    {
        const Scalar t = ( time_of_day_ - 0.70f ) / 0.10f;
        profile_ = SKY_PROFILES[SKY_MODE_SUNSET].lerp( t, SKY_PROFILES[SKY_MODE_NIGHT] );
    }

    sun_angle_[0] = ( 1.0f - 2.0f * time_of_day_ ) * gmtl::Math::PI;
    sun_angle_[1] = 0.01f * gmtl::Math::PI * gmtl::Math::sin( time_of_day_ * 2.0f * gmtl::Math::PI );

    moon_angle_[0] = sun_angle_[0] - gmtl::Math::PI;
    moon_angle_[1] = sun_angle_[1] - 2.0f * gmtl::Math::PI;
}

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for World:
//////////////////////////////////////////////////////////////////////////////////

World::World( const uint64_t world_seed ) :
    generator_( world_seed ),
    sky_( world_seed ),
    worker_pool_( hardware_concurrency() )
{
    SCOPE_TIMER_BEGIN( "World generation" )

    for ( int x = 0; x < 3; ++x )
    {
        for ( int z = 0; z < 3; ++z )
        {
            const Vector2i position( x * WorldGenerator::REGION_SIZE, z * WorldGenerator::REGION_SIZE );
            worker_pool_.schedule(
                boost::bind( &generate_region_concurrently, boost::ref( generator_ ), position, boost::ref( chunks_ ) ) );
        }
    }

    worker_pool_.wait();

    SCOPE_TIMER_END

    ChunkSet chunks;

    BOOST_FOREACH( ChunkMap::value_type& chunk_it, chunks_ )
    {
        chunks.insert( chunk_it.second.get() );
    }

    reset_lighting( chunks );
    apply_lighting_to_self( chunks );
    apply_lighting_to_neighbors( chunks );
    update_geometry( chunks );
}

void World::do_one_step( const float step_time )
{
    sky_.do_one_step( step_time );
}

ChunkSet World::update_chunks()
{
    if ( chunks_needing_update_.empty() )
    {
        return ChunkSet();
    }

    // FIXME: The below comment needs updating.
    //
    // If a Chunk is modified, it is not sufficient to simply rebuild the lighting/geometry
    // for that Chunk.  Rather, all adjacent Chunks, and any Chunks below it or its neighbors
    // must be updated, to ensure that lighting is propagated correctly (including sunlight).
    //
    // TODO: Explain neighbor lighting.
    //
    //   A A A  
    // A R R R A
    // A R X R A
    // A R R R A
    //   A A A  
    //
    //
    // TODO: This function should operate in a background thread so that it does not block
    //       the main game loop.  That raises some interesting questions RE: locking the
    //       Chunks.

    ChunkSet reset_chunks;
    ChunkSet possibly_modified_chunks;
    ChunkSet neighbor_chunks;

    add_chunks_affected_by_sunlight( chunks_needing_update_ );

    BOOST_FOREACH( Chunk* chunk, chunks_needing_update_ )
    {
        FOREACH_SURROUNDING( x, y, z )
        {
            const Vector3i position =
                chunk->get_position() + pointwise_product( Chunk::SIZE, Vector3i( x, y, z ) );

            Chunk* possibly_modified_chunk = get_chunk( position );
            
            if ( possibly_modified_chunk )
            {
                if ( chunks_needing_update_.find( possibly_modified_chunk ) == chunks_needing_update_.end() )
                {
                    // The Chunks in chunks_needing_update_ were already
                    // reset by add_chunks_affected_by_sunlight().
                    reset_chunks.insert( possibly_modified_chunk );
                }

                possibly_modified_chunks.insert( possibly_modified_chunk );
                neighbor_chunks.insert( possibly_modified_chunk );

                FOREACH_CARDINAL_RELATION( relation )
                {
                    Chunk* neighbor_chunk =
                        possibly_modified_chunk->get_neighbor( cardinal_relation_vector( relation ) );

                    if ( neighbor_chunk )
                    {
                        neighbor_chunks.insert( neighbor_chunk );
                    }
                }
            }
        }
    }

    // TODO: This reset might be doable in parallel without any sorting, due to the fact that
    //       the Chunks in which the sunlighting may have changed were already reset in top-down
    //       order by add_chunks_affected_by_sunlight().
    reset_lighting( reset_chunks );
    apply_lighting_to_self( possibly_modified_chunks );
    apply_lighting_to_neighbors( neighbor_chunks );
    update_geometry( possibly_modified_chunks );

    chunks_needing_update_.clear();
    return possibly_modified_chunks;
}

void World::reset_lighting( const ChunkSet& chunks )
{
    SCOPE_TIMER_BEGIN( "Resetting lighting" )

    ChunkV height_sorted_chunks;

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        height_sorted_chunks.push_back( chunk );
    }

    std::sort( height_sorted_chunks.begin(), height_sorted_chunks.end(), highest_chunk );

    // TODO: This can be done in parallel IF it's still done in top-down order.
    BOOST_FOREACH( Chunk* chunk, height_sorted_chunks )
    {
        chunk->reset_lighting();
    }

    SCOPE_TIMER_END
}

void World::apply_lighting_to_self( const ChunkSet& chunks )
{
    SCOPE_TIMER_BEGIN( "Self-lighting" )

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        worker_pool_.schedule( boost::bind( &Chunk::apply_lighting_to_self, chunk ) );
    }

    worker_pool_.wait();

    SCOPE_TIMER_END
}

// The neighbor lighting pass crosses between Chunk boundaries.  Thus, if neighbor lighting
// is performed on two nearby Chunks in parallel, threading issues could arise (e.g. two
// threads could be modifying the same Chunk at the same time).  This function takes this
// into account, and only performs parallel computation on Chunks that are too far apart
// for their lights to overlap the same Blocks.
void World::apply_lighting_to_neighbors( ChunkSet chunks )
{
    SCOPE_TIMER_BEGIN( "Neighbor-lighting" )

    ChunkSet separated_chunks;

    while ( !chunks.empty() )
    {
        find_doubly_separated_chunks( chunks, separated_chunks );
        assert( !separated_chunks.empty() );

        BOOST_FOREACH( Chunk* chunk, separated_chunks )
        {
            worker_pool_.schedule( boost::bind( &Chunk::apply_lighting_to_neighbors, chunk ) );
            chunks.erase( chunk );
        }

        separated_chunks.clear();
        worker_pool_.wait();
    }

    SCOPE_TIMER_END
}

void World::update_geometry( const ChunkSet& chunks )
{
    SCOPE_TIMER_BEGIN( "Updating geometry" )

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        worker_pool_.schedule( boost::bind( &Chunk::update_geometry, chunk ) );
    }

    worker_pool_.wait();

    SCOPE_TIMER_END
}
