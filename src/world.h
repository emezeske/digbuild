#ifndef WORLD_H
#define WORLD_H

#include <cstdlib>
#include <set>

#include <boost/threadpool.hpp>

#include "world_generator.h"
#include "chunk.h"

struct Sky
{
    struct Profile
    {
        Profile(
            const Scalar star_intensity = 0.0f,
            const Vector3f& zenith_color = Vector3f(),
            const Vector3f& horizon_color = Vector3f(),
            const Vector3f& sun_color = Vector3f(),
            const Vector3f& sun_light_color = Vector3f(),
            const Vector3f& moon_color = Vector3f(),
            const Vector3f& moon_light_color = Vector3f()
        );

        Profile lerp( const Scalar t, const Sky::Profile& other ) const;

        Scalar star_intensity_;

        Vector3f
            zenith_color_,
            horizon_color_,
            sun_color_,
            sun_light_color_,
            moon_color_,
            moon_light_color_;
    };

    typedef std::vector<Vector3f> StarV; 

    Sky( const uint64_t world_seed );

    void do_one_step( float step_time );

    Scalar get_star_intensity() const { return profile_.star_intensity_; }
    const StarV& get_stars() const { return stars_; }
    const Vector3f& get_zenith_color() const { return profile_.zenith_color_; }
    const Vector3f& get_horizon_color() const { return profile_.horizon_color_; }
    const Vector3f& get_sun_color() const { return profile_.sun_color_; }
    const Vector3f& get_sun_light_color() const { return profile_.sun_light_color_; }
    const Vector2f& get_sun_angle() const { return sun_angle_; }
    const Vector3f& get_moon_color() const { return profile_.moon_color_; }
    const Vector3f& get_moon_light_color() const { return profile_.moon_light_color_; }
    const Vector2f& get_moon_angle() const { return moon_angle_; }

protected:

    static const Scalar DAY_CYCLE_SPEED = 0.01f;

    StarV stars_;

    Scalar time_of_day_;

    Profile profile_;

    Vector2f
        sun_angle_,
        moon_angle_;
};

typedef std::set<Chunk*> ChunkSet;

struct World
{
    typedef boost::unique_lock<boost::mutex> ChunkGuard;

    World( const uint64_t world_seed );

    void do_one_step( float step_time );

    const Sky& get_sky() const { return sky_; }
    const ChunkMap& get_chunks() const { return chunks_; }

    BlockIterator get_block( const Vector3i& position ) const
    {
        // Use std::div() instead of '%' to ensure rounding towards zero.
        const std::div_t
            div_x = std::div( position[0], Chunk::SIZE_X ),
            div_y = std::div( position[1], Chunk::SIZE_Y ),
            div_z = std::div( position[2], Chunk::SIZE_Z );

        BlockIterator result;
        result.index_ = Vector3i( div_x.rem, div_y.rem, div_z.rem );

        // Wrap negative remainders back into the correct block index range.
        for ( int i = 0; i < 3; ++i )
        {
            if ( result.index_[i] < 0 )
            {
                result.index_[i] += Chunk::SIZE[i];
            }
        }

        const Vector3i chunk_position = position - result.index_;
        ChunkMap::const_iterator chunk_it = chunks_.find( chunk_position );

        if ( chunk_it != chunks_.end() )
        {
            result.chunk_ = chunk_it->second.get();
            result.block_ = &chunk_it->second->get_block( result.index_ );
        }

        return result;
    }

    void mark_chunk_for_update( Chunk* chunk )
    {
        assert( chunk );
        chunks_needing_update_.insert( chunk );
    }

    // This function updates the Chunk lighting and geometry for all of the Chunks that
    // have been marked for update.  Since this might be a time-consuming process, it
    // periodically yields its execution for e.g. the rendering loop to continue.  When
    // the Chunks are all updated, you can call get_updated_chunks() to determine which
    // ones were affected.
    void update_chunks();

    bool is_chunk_update_in_progress() const { return chunk_update_in_progress_; }

    ChunkSet get_updated_chunks()
    {
        assert( !is_chunk_update_in_progress() );
        ChunkSet result = updated_chunks_;
        updated_chunks_.clear();
        return result;
    }

    // The Chunk lock is held by this class whenever it may be accessing the Chunks.  Any
    // code outside of this class should grab the lock before doing the same.
    boost::mutex& get_chunk_lock() { return chunk_lock_; }

protected:

    Chunk* get_chunk( const Vector3i& position )
    {
        ChunkMap::const_iterator it = chunks_.find( position );
        return it == chunks_.end() ? 0 : it->second.get();
    }

    void reset_lighting_unordered( ChunkGuard& chunk_guard, const ChunkSet& chunks );
    void reset_lighting_top_down( ChunkGuard& chunk_guard, const ChunkSet& chunks );
    void apply_lighting_to_self( ChunkGuard& chunk_guard, const ChunkSet& chunks );
    void apply_lighting_to_neighbors( ChunkGuard& chunk_guard, ChunkSet chunks );
    void update_geometry( ChunkGuard& chunk_guard, const ChunkSet& chunks );

    void schedule( ChunkGuard& chunk_guard, boost::threadpool::pool::task_type const& task );
    void yield( ChunkGuard& chunk_guard );

    ChunkSet
        chunks_needing_update_,
        updated_chunks_;

    bool chunk_update_in_progress_;

    WorldGenerator generator_;

    Sky sky_;

    ChunkMap chunks_;

    boost::threadpool::pool worker_pool_;

    unsigned outstanding_jobs_;

    boost::mutex chunk_lock_;
};

#endif // WORLD_H
