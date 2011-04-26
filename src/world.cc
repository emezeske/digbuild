#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/uniform_on_sphere.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/foreach.hpp>

#include "world.h"
#include "timer.h"

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

void sort_and_reset_lighting( ChunkV& chunks )
{
    std::sort( chunks.begin(), chunks.end(), highest_chunk );

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        chunk->reset_lighting();
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
            chunks.insert( next );

            if ( reset_changes_base_sunlight( *next ) )
            {
                next = next->get_neighbor( cardinal_relation_vector( CARDINAL_RELATION_BELOW ) );

                if ( chunks.find( next ) != chunks.end() )
                {
                    next = 0;
                }
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
    worker_pool_( hardware_concurrency() ),
    outstanding_jobs_( 0 )
{
    ChunkGuard chunk_guard( chunk_lock_ );

    SCOPE_TIMER_BEGIN( "World generation" )

    for ( int x = 0; x < 3; ++x )
    {
        for ( int z = 0; z < 3; ++z )
        {
            const Vector2i region_position( x * WorldGenerator::REGION_SIZE, z * WorldGenerator::REGION_SIZE );
            ChunkSPV region = generator_.generate_region( region_position, worker_pool_ );

            BOOST_FOREACH( ChunkSP chunk, region )
            {
                chunk_stitch_into_map( chunk, chunks_ );
            }
        }
    }

    yield( chunk_guard );

    SCOPE_TIMER_END

    ChunkSet chunks;

    BOOST_FOREACH( ChunkMap::value_type& chunk_it, chunks_ )
    {
        chunks.insert( chunk_it.second.get() );
    }

    // The lighting for each Chunk needs to be reset in top-down order to ensure
    // that sunlight is correctly propagated from the top Chunks to the ones below.
    reset_lighting_top_down( chunk_guard, chunks );

    apply_lighting_to_self( chunk_guard, chunks );
    apply_lighting_to_neighbors( chunk_guard, chunks );
    update_geometry( chunk_guard, chunks );
}

void World::do_one_step( const float step_time )
{
    sky_.do_one_step( step_time );
}

void World::update_chunks()
{
    ChunkGuard chunk_guard( chunk_lock_ );
    assert( updated_chunks_.empty() );

    if ( chunks_needing_update_.empty() )
    {
        return;
    }

    // If a Chunk is modified, it is not sufficient to simply rebuild the lighting/geometry
    // for that Chunk.  Lighting can travel up to 16 blocks, so a change to one Chunk might
    // spread light to other surrounding Chunks.  The Chunks are (at least) 16 blocks in size,
    // so it is sufficient to rebuild the lighting/geometry for just one layer of surrounding Chunks.
    //
    // To rebuild the lighting for a Chunk, its current lighting has to be reset.  Then, all of
    // the lights that Chunk contains are applied within the Chunk.  Finally, that Chunk, and any
    // other Chunks that it shares a face with must have their "neighbor" lighting applied.  This
    // propagates lighting from e.g. Chunks that were not rebuilt.
    //
    // Consider a Chunk 'M' that has been modified.  In this diagram, the Chunks labelled 'M'
    // and 'R' must be rebuilt.  Neighbor lighting from the Chunks labelled 'M', 'R', and 'N'
    // must be applied.  Of course, this diagram is only 2D, but it's easy to extrapolate to 3D.
    //
    //      N N N  
    //    N R R R N
    //    N R M R N
    //    N R R R N
    //      N N N  

    // Make a copy of the chunks_needing_update_ set so that chunks_needing_update_ can
    // be safely modified while this function yields.
    ChunkSet chunks_needing_update = chunks_needing_update_;
    chunks_needing_update_.clear();

    ChunkSet reset_chunks;
    ChunkSet possibly_modified_chunks;
    ChunkSet neighbor_chunks;

    add_chunks_affected_by_sunlight( chunks_needing_update );

    BOOST_FOREACH( Chunk* chunk, chunks_needing_update )
    {
        FOREACH_SURROUNDING( x, y, z )
        {
            const Vector3i position =
                chunk->get_position() + pointwise_product( Chunk::SIZE, Vector3i( x, y, z ) );

            Chunk* possibly_modified_chunk = get_chunk( position );
            
            if ( possibly_modified_chunk )
            {
                if ( chunks_needing_update.find( possibly_modified_chunk ) == chunks_needing_update.end() )
                {
                    // The Chunks in chunks_needing_update were already reset by add_chunks_affected_by_sunlight().
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

    // This reset might can be done without any sorting, due to the fact that
    // the Chunks in which the sunlighting may have changed were already reset
    // in top-down order by add_chunks_affected_by_sunlight().
    reset_lighting_unordered( chunk_guard, reset_chunks );

    apply_lighting_to_self( chunk_guard, possibly_modified_chunks );
    apply_lighting_to_neighbors( chunk_guard, neighbor_chunks );
    update_geometry( chunk_guard, possibly_modified_chunks );

    // TODO: Only add Chunks that were DEFINITELY modified to updated_chunks_.  This will
    //       save time because they won't need to be sent to the graphics card.
    updated_chunks_ = possibly_modified_chunks;
}

void World::reset_lighting_unordered( ChunkGuard& chunk_guard, const ChunkSet& chunks )
{
    SCOPE_TIMER_BEGIN( "Resetting lighting (unordered)" )

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        schedule( chunk_guard, boost::bind( &Chunk::reset_lighting, chunk ) );
    }

    yield( chunk_guard );

    SCOPE_TIMER_END
}

void World::reset_lighting_top_down( ChunkGuard& chunk_guard, const ChunkSet& chunks )
{
    SCOPE_TIMER_BEGIN( "Resetting lighting (top-down)" )

    // The reset in a single column needs to be performed from the top down,
    // but blocks in different columns can be reset in any order.

    typedef std::map<Vector2i, ChunkV, VectorLess<Vector2i> > ColumnMap;
    ColumnMap column_chunks;

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        const Vector2i column_position( chunk->get_position()[0], chunk->get_position()[2] );
        column_chunks[column_position].push_back( chunk );
    }

    BOOST_FOREACH( const ColumnMap::value_type& it, column_chunks )
    {
        schedule( chunk_guard, boost::bind( &sort_and_reset_lighting, it.second ) );
    }

    yield( chunk_guard );

    SCOPE_TIMER_END
}

void World::apply_lighting_to_self( ChunkGuard& chunk_guard, const ChunkSet& chunks )
{
    SCOPE_TIMER_BEGIN( "Self-lighting" )

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        schedule( chunk_guard, boost::bind( &Chunk::apply_lighting_to_self, chunk ) );
    }

    yield( chunk_guard );

    SCOPE_TIMER_END
}

// The neighbor lighting pass crosses between Chunk boundaries.  Thus, if neighbor lighting
// is performed on two nearby Chunks in parallel, threading issues could arise (e.g. two
// threads could be modifying the same Chunk at the same time).  This function takes this
// into account, and only performs parallel computation on Chunks that are too far apart
// for their lights to overlap the same Blocks.
void World::apply_lighting_to_neighbors( ChunkGuard& chunk_guard, ChunkSet chunks )
{
    SCOPE_TIMER_BEGIN( "Neighbor-lighting" )

    ChunkSet separated_chunks;

    while ( !chunks.empty() )
    {
        find_doubly_separated_chunks( chunks, separated_chunks );
        assert( !separated_chunks.empty() );

        BOOST_FOREACH( Chunk* chunk, separated_chunks )
        {
            schedule( chunk_guard, boost::bind( &Chunk::apply_lighting_to_neighbors, chunk ) );
            chunks.erase( chunk );
        }

        separated_chunks.clear();
        yield( chunk_guard );
    }

    SCOPE_TIMER_END
}

void World::update_geometry( ChunkGuard& chunk_guard, const ChunkSet& chunks )
{
    SCOPE_TIMER_BEGIN( "Updating geometry" )

    BOOST_FOREACH( Chunk* chunk, chunks )
    {
        schedule( chunk_guard, boost::bind( &Chunk::update_geometry, chunk ) );
    }

    yield( chunk_guard );

    SCOPE_TIMER_END
}

void World::schedule( ChunkGuard& chunk_guard, boost::threadpool::pool::task_type const& task )
{
    worker_pool_.schedule( task );

    // Don't yield until there are enough tasks scheduled to keep all of the processors
    // busy for a while (in the background).
    if ( ++outstanding_jobs_ > worker_pool_.size() )
    {
        outstanding_jobs_ = 0;
        yield( chunk_guard );
    }
}

void World::yield( ChunkGuard& chunk_guard )
{
    // First, ensure that all background access to the Chunks has ceased.
    worker_pool_.wait();

    // Now, briefly relinquish the lock so that other processes can have
    // a chance to access the Chunks.
    chunk_guard.unlock();
    boost::this_thread::yield();
    chunk_guard.lock();
}
