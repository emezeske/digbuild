#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/uniform_on_sphere.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>

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

void generate_region_concurrently( WorldGenerator& generator, const Vector2i& position, ChunkMap& chunks )
{
    static boost::mutex lock;

    ChunkV new_chunks = generator.generate_region( position );

    boost::lock_guard<boost::mutex> guard( lock );

    for ( ChunkV::iterator chunk_it = new_chunks.begin(); chunk_it != new_chunks.end(); ++chunk_it )
    {
        chunk_stitch_into_map( *chunk_it, chunks );
    }
}

void reset_and_apply_lighting_to_self( Chunk* chunk )
{
    chunk->reset_lighting();
    chunk->apply_lighting_to_self();
}

typedef std::pair<Vector3i, ChunkSP> PositionChunkPair;
typedef std::vector<PositionChunkPair> ChunkMapValueV;

bool highest_chunk( const PositionChunkPair& a, const PositionChunkPair& b )
{
    return a.first[1] > b.first[1];
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
    SCOPE_TIMER_BEGIN( "Lighting" )

    ChunkMapValueV height_sorted_chunks;

    for ( ChunkMap::iterator chunk_it = chunks_.begin(); chunk_it != chunks_.end(); ++chunk_it )
    {
        height_sorted_chunks.push_back( *chunk_it );
    }

    std::sort( height_sorted_chunks.begin(), height_sorted_chunks.end(), highest_chunk );

    for ( ChunkMapValueV::iterator chunk_it = height_sorted_chunks.begin(); chunk_it != height_sorted_chunks.end(); ++chunk_it )
    {
        worker_pool_.schedule( boost::bind( &Chunk::reset_lighting, boost::ref( chunk_it->second ) ) );
    }

    worker_pool_.wait();

    for ( ChunkMapValueV::iterator chunk_it = height_sorted_chunks.begin(); chunk_it != height_sorted_chunks.end(); ++chunk_it )
    {
        worker_pool_.schedule( boost::bind( &Chunk::apply_lighting_to_self, chunk_it->second.get() ) );
    }

    worker_pool_.wait();

    for ( ChunkMapValueV::iterator chunk_it = height_sorted_chunks.begin(); chunk_it != height_sorted_chunks.end(); ++chunk_it )
    {
        // TODO: Chunks that do not have any common neighbors could be updated
        //       in parallel.
        chunk_it->second->apply_lighting_to_neighbors();
    }

    SCOPE_TIMER_END
    SCOPE_TIMER_BEGIN( "Updating geometry" )

    for ( ChunkMap::iterator chunk_it = chunks_.begin(); chunk_it != chunks_.end(); ++chunk_it )
    {
        worker_pool_.schedule( boost::bind( &Chunk::update_geometry, chunk_it->second.get() ) );
    }

    worker_pool_.wait();

    SCOPE_TIMER_END
}

void World::do_one_step( const float step_time )
{
    sky_.do_one_step( step_time );
}

ChunkSet World::update_chunks()
{
    if ( columns_needing_update_.empty() )
    {
        return ChunkSet();
    }

    // If a Chunk is modified, it is not sufficient to simply rebuild the lighting/geometry
    // for that Chunk.  Rather, all adjacent Chunks, and any Chunks below it or its neighbors
    // must be updated, to ensure that lighting is propagated correctly (including sunlight).
    //
    // TODO: Explain neighbor lighting.
    //
    // TODO: Currently, the entire column that a Chunk is part of (and all adjacent columns)
    //       are fully rebuilt.  Most of the time this is big overkill.

    // TODO: This function should operate in a background thread so that it does not block
    //       the main game loop.  That raises some interesting questions RE: locking the
    //       Chunks.
    
    ChunkSet full_lighting_columns;
    ChunkSet neighbor_lighting_columns;

    for ( ChunkSet::const_iterator column_it = columns_needing_update_.begin();
          column_it != columns_needing_update_.end();
          ++column_it )
    {
        const Vector3i base_position = ( *column_it )->get_position();

        // TODO Explain
        //
        // A A A A A
        // A R R R A
        // A R R R A
        // A R R R A
        // A A A A A

        for ( int x = -2; x <= 2; ++x )
        {
            for ( int z = -2; z <= 2; ++z )
            {
                const Vector3i position =
                    base_position + Vector3i( x * Chunk::SIZE_X, 0, z * Chunk::SIZE_Z );

                Chunk* chunk = get_chunk( position );
                
                if ( chunk )
                {
                    Chunk* column_top = chunk->get_column_top();
                    neighbor_lighting_columns.insert( column_top );

                    if ( abs( x ) != 2 && abs( z ) != 2 )
                    {
                        full_lighting_columns.insert( column_top );
                    }
                }
            }
        }
    }

    columns_needing_update_.clear();
    ChunkSet chunks_updated;

    // TODO: Remove some of this for/while duplication.
    SCOPE_TIMER_BEGIN( "Self-lighting columns" )

    for ( ChunkSet::const_iterator column_it = full_lighting_columns.begin();
          column_it != full_lighting_columns.end();
          ++column_it )
    {
        Chunk* chunk = *column_it;

        while ( chunk )
        {
            worker_pool_.schedule( boost::bind( &reset_and_apply_lighting_to_self, chunk ) );
            chunks_updated.insert( chunk );
            chunk = chunk->get_neighbor( cardinal_relation_vector( CARDINAL_RELATION_BELOW ) );
        }
    }

    worker_pool_.wait();
    SCOPE_TIMER_END
    SCOPE_TIMER_BEGIN( "Neighbor-lighting columns" )

    for ( ChunkSet::const_iterator column_it = neighbor_lighting_columns.begin();
          column_it != neighbor_lighting_columns.end();
          ++column_it )
    {
        Chunk* chunk = *column_it;

        while ( chunk )
        {
            // TODO: Chunks that do not have any common neighbors could be updated
            //       in parallel.
            chunk->apply_lighting_to_neighbors();
            chunk = chunk->get_neighbor( cardinal_relation_vector( CARDINAL_RELATION_BELOW ) );
        }
    }

    SCOPE_TIMER_END
    SCOPE_TIMER_BEGIN( "Updating geometry" )

    for ( ChunkSet::const_iterator chunk_it = chunks_updated.begin();
          chunk_it != chunks_updated.end();
          ++chunk_it )
    {
        worker_pool_.schedule( boost::bind( &Chunk::update_geometry, *chunk_it ) );
    }

    worker_pool_.wait();
    SCOPE_TIMER_END

    return chunks_updated;
}
