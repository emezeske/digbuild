#ifndef WORLD_H
#define WORLD_H

#include <cstdlib>

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

struct World
{
    World( const uint64_t world_seed );

    void do_one_step( float step_time );

    const Sky& get_sky() const { return sky_; }
    const ChunkMap& get_chunks() const { return chunks_; }

    const Block* get_block( const Vector3i& position ) const
    {
        // Use std::div() instead of '%' to ensure rounding towards zero.
        const std::div_t
            div_x = std::div( position[0], Chunk::SIZE_X ),
            div_y = std::div( position[1], Chunk::SIZE_Y ),
            div_z = std::div( position[2], Chunk::SIZE_Z );

        // The remainder result of std::div() has the same sign of the numerator,
        // but we just want its magnitude. 
        const Vector3i block_index =
            Vector3i( abs( div_x.rem ), abs( div_y.rem ), abs( div_z.rem ) );

        const Vector3i chunk_position = position - block_index;

        ChunkMap::const_iterator chunk_it = chunks_.find( chunk_position );

        if ( chunk_it != chunks_.end() )
        {
            return &chunk_it->second->get_block( block_index );
        }

        return 0;
    }

protected:

    WorldGenerator generator_;

    Sky sky_;

    ChunkMap chunks_;
};

#endif // WORLD_H
