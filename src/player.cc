#include <SDL/SDL.h>

#include <boost/foreach.hpp>

#include <limits>
#include <iomanip>

#include "cardinal_relation.h"
#include "player.h"

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for Player:
//////////////////////////////////////////////////////////////////////////////////

const Vector3f
    Player::SIZE( 0.50f, 1.9f, 0.50f ),
    Player::HALFSIZE( SIZE / 2.0f );

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Player:
//////////////////////////////////////////////////////////////////////////////////

Player::Player( const Vector3f& position, const Scalar pitch, const Scalar yaw ) :
    position_( position ),
    pitch_( pitch ),
    yaw_( yaw ),
    requesting_fast_move_( false ),
    requesting_move_forward_( false ),
    requesting_move_backward_( false ),
    requesting_strafe_left_( false ),
    requesting_strafe_right_( false ),
    requesting_jump_( false ),
    requesting_crouch_( false ),
    requesting_primary_fire_( false ),
    noclip_mode_( true ),
    feet_contacting_block_( false ),
    last_jump_at_( 0 )
{
}

void Player::do_one_step( const float step_time, World& world )
{
    if ( noclip_mode_ )
    {
        do_one_step_noclip( step_time );
    }
    else do_one_step_clip( step_time, world );

    do_primary_fire( step_time, world );
}

void Player::do_one_step_noclip( const float step_time )
{
    velocity_ = Vector3f();

    Scalar movement_units = step_time * NOCLIP_SPEED;
    if ( requesting_fast_move_ ) movement_units *= NOCLIP_FAST_MOVE_FACTOR;

    if ( requesting_move_forward_ ) noclip_move_forward( movement_units );
    if ( requesting_move_backward_ ) noclip_move_forward( -movement_units );
    if ( requesting_strafe_left_ ) noclip_strafe( movement_units );
    if ( requesting_strafe_right_ ) noclip_strafe( -movement_units );
    if ( requesting_jump_ ) position_ += Vector3f( 0.0f, movement_units, 0.0f );
    if ( requesting_crouch_ ) position_ -= Vector3f( 0.0f, movement_units, 0.0f );
}

void Player::do_one_step_clip( const float step_time, const World& world )
{
    accelerate( step_time );
    feet_contacting_block_ = false;
    Scalar time_simulated = 0.0f;

    // TODO: Is a maximum of 3 step slices sufficient?

    for ( int steps = 0; steps < 3 && time_simulated < step_time; ++steps )
    {
        const Scalar step_time_slice = ( step_time - time_simulated );
        const Vector3f movement = velocity_ * step_time_slice;

        BlockCollision collision;

        if ( find_collision( world, movement, collision ) )
        {
            resolve_collision( movement, collision );
            time_simulated += collision.normalized_time_ * step_time_slice;
        }
        else
        {
            position_ += movement;
            break;
        }
    }
}

void Player::do_primary_fire( const float step_time, World& world )
{
    const long now = SDL_GetTicks();

    if ( requesting_primary_fire_ && last_primary_fire_at_ + PRIMARY_FIRE_INTERVAL_MS < now )
    {
        last_primary_fire_at_ = now;

        const Vector3f
            fire_begin = get_eye_position(),
            fire_end = fire_begin + get_eye_direction() * PRIMARY_FIRE_DISTANCE;

        const gmtl::LineSegf fire_line = gmtl::LineSegf( gmtl::Point3f( fire_begin ), gmtl::Point3f( fire_end ) );

        PotentialObstructionSet potential_obstructions;
        get_potential_obstructions( world, fire_line.mOrigin, fire_line.mDir, Vector3f(), potential_obstructions );

        Scalar normalized_hit_time = std::numeric_limits<Scalar>::max();
        bool hit_found;
        Vector3i hit_block_position;

        BOOST_FOREACH( const PotentialObstruction& obstruction, potential_obstructions )
        {
            const Vector3f block_position = obstruction.block_position_;
            const AABoxf block_bounds( block_position, block_position + Block::SIZE );

            unsigned num_hits;

            Scalar
                normalized_time_in,
                normalized_time_out;

            const bool intersected =
                gmtl::intersect( fire_line, block_bounds, num_hits, normalized_time_in, normalized_time_out );

            if ( intersected && normalized_time_in < normalized_hit_time )
            {
                normalized_hit_time = normalized_time_in;
                hit_found = true;
                hit_block_position = vector_cast<int>( block_position );
            }
        }

        if ( hit_found )
        {
            BlockIterator block_it = world.get_block( hit_block_position );
            block_it.block_->set_material( BLOCK_MATERIAL_AIR );
            world.mark_chunk_for_update( block_it.chunk_ ); 
        }
    }
}

void Player::accelerate( const float step_time )
{
    accelerate_lateral( step_time );
    accelerate_vertical( step_time );
}

void Player::accelerate_lateral( const float step_time )
{
    const Vector2f current_velocity( velocity_[0], velocity_[2] );
    const Vector2f forward_direction( gmtl::Math::sin( yaw_ ), gmtl::Math::cos( yaw_ ) );
    const Vector2f strafe_direction( -forward_direction[1], forward_direction[0] );

    Vector2f target_velocity;
    if ( requesting_move_forward_ ) target_velocity += forward_direction;
    if ( requesting_move_backward_ ) target_velocity -= forward_direction;
    if ( requesting_strafe_left_ ) target_velocity -= strafe_direction;
    if ( requesting_strafe_right_ ) target_velocity += strafe_direction;
    gmtl::normalize( target_velocity );
    target_velocity *= WALKING_SPEED;

    Vector2f acceleration_direction = ( target_velocity - current_velocity );
    const Scalar velocity_difference = gmtl::length( acceleration_direction );
    gmtl::normalize( acceleration_direction );

    const Scalar acceleration_power =
        ( feet_contacting_block_ ? GROUND_ACCELERATION : AIR_ACCELERATION ) *
        std::min( velocity_difference / WALKING_SPEED, 1.0f );

    const Vector2f acceleration = acceleration_direction * acceleration_power;

    const Vector2f dvelocity = acceleration * step_time;
    velocity_ += Vector3f( dvelocity[0], 0.0f, dvelocity[1] );
}

void Player::accelerate_vertical( const float step_time )
{
    if ( feet_contacting_block_ )
    {
        const long now = SDL_GetTicks();

        if ( requesting_jump_ && last_jump_at_ + JUMP_INTERVAL_MS < now )
        {
            last_jump_at_ = now;
            velocity_[1] += JUMP_VELOCITY;
        }
    }
    else velocity_[1] += GRAVITY_ACCELERATION * step_time;
}

bool Player::find_collision( const World& world, const Vector3f& movement, BlockCollision& collision )
{
    // TODO: Decompose this function.

    bool collision_found = false;
    collision.normalized_time_ = std::numeric_limits<Scalar>::max();
    collision.block_position_ = Vector3f();

    PotentialObstructionSet potential_obstructions;
    get_potential_obstructions( world, vector_cast<Scalar>( position_ ), movement, vector_cast<Scalar>( SIZE ), potential_obstructions );

    // Determine whether each potential obstruction actually intersects with the Player's AABB at
    // some point in its movement.  If there are multiple potential collisions, only return the
    // one that would happen at the earliest point in time.
    BOOST_FOREACH( const PotentialObstruction& obstruction, potential_obstructions )
    {
        const Vector3f block_position = obstruction.block_position_;
        const Block& block = *obstruction.block_;
        const AABoxf
            player_bounds = get_aabb(),
            block_bounds( block_position, block_position + Block::SIZE );

        Scalar normalized_first_contact;

        const bool intersected =
            gmtl::intersect_bugfix( player_bounds, movement, block_bounds, normalized_first_contact );

        // The normalized contact time can be outside the range [0,1], but we're only interested
        // in values in [0,1], because they map to vectors within the movement vector.
        if ( intersected &&
             normalized_first_contact >= 0.0f &&
             normalized_first_contact <= 1.0f &&
             normalized_first_contact < collision.normalized_time_ )
        {
            // Determine which face of the Block the Player is colliding with by measuring the
            // distance between the corresponding pairs of AABB planes (e.g. bottom & top) on the
            // Player and the Block, and choosing the face with the smallest distance.
            Scalar min_distance_squared = std::numeric_limits<Scalar>::max();
            Vector3f collision_normal;
            CardinalRelation collision_relation = CARDINAL_RELATION_BELOW;

            FOREACH_CARDINAL_RELATION( relation )
            {
                // Make sure that the face that the Player is colliding with is reachable;
                // e.g. its not obstructed by another block.
                const Vector3i block_neighbor_offset =
                    cardinal_relation_vector( cardinal_relation_reverse( relation ) );
                const Block* block_neighbor =
                    world.get_block( vector_cast<int>( block_position ) + block_neighbor_offset ).block_;

                // TODO: Use a material attribute instead.
                if ( !block_neighbor || block_neighbor->get_material() == BLOCK_MATERIAL_AIR )
                {
                    const Vector3f
                        player_centroid = position_ + normalized_first_contact * movement + HALFSIZE,
                        block_centroid = block_position + Block::HALFSIZE,
                        player_normal = vector_cast<Scalar>( cardinal_relation_vector( relation ) ),
                        block_normal = -player_normal,
                        player_plane_point = player_centroid + pointwise_product( player_normal, HALFSIZE ),
                        block_plane_point = block_centroid + pointwise_product( block_normal, Block::HALFSIZE );

                    const Scalar
                        player_plane_offset = dot( player_plane_point, player_normal ),
                        block_plane_offset = dot( block_plane_point, player_normal ),
                        dplane_offset = gmtl::Math::abs( player_plane_offset - block_plane_offset );

                    if ( dplane_offset < min_distance_squared )
                    {
                        min_distance_squared = dplane_offset;
                        collision_normal = player_normal;
                        collision_relation = relation;
                    }
                }
            }

            // If normalized_first_contact is zero, it indicates that the Player was already intersecting
            // with the Block before it moved.  In this case, the collision is ignored if the Player's
            // velocity is directed away from the block.
            if ( normalized_first_contact > 0.0f || gmtl::dot( movement, collision_normal ) > 0.0f )
            {
                if ( collision_relation == CARDINAL_RELATION_BELOW )
                {
                    feet_contacting_block_ = true;
                }

                collision_found = true;
                collision.normalized_time_ = normalized_first_contact;
                collision.block_position_ = block_position;
                collision.player_face_ = collision_relation;
            }
        }
    }

    return collision_found;
}

void Player::resolve_collision( const Vector3f& movement, BlockCollision& collision )
{
    position_ += movement * collision.normalized_time_;

    if ( collision.player_face_ == CARDINAL_RELATION_BELOW )
    {
        feet_contacting_block_ = true;
    }

    obstructing_block_position_ = collision.block_position_;

    const Vector3f normal =
        vector_cast<Scalar>( cardinal_relation_vector( collision.player_face_ ) );

    // Reverse the component of the player's velocity that is directed toward the
    // block, and apply a little rebound as well.
    const Vector3f collision_component = gmtl::dot( velocity_, normal ) * normal;
    velocity_ -= 1.05f * collision_component;
}

void Player::adjust_direction( const Scalar dpitch, const Scalar dyaw )
{
    pitch_ += dpitch;

    if ( pitch_ < 0.0f )
    {
        pitch_ = 0.0f;
    }
    else if ( pitch_ > gmtl::Math::PI )
    {
        pitch_ = gmtl::Math::PI;
    }

    yaw_ += dyaw;

    while ( yaw_ < 0.0f )
    {
        yaw_ += 2.0f * gmtl::Math::PI;
    }

    while ( yaw_ > 2.0f * gmtl::Math::PI )
    {
        yaw_ -= 2.0f * gmtl::Math::PI;
    }
}

Vector3f Player::get_eye_position() const
{
    return position_ + Vector3f( HALFSIZE[0], EYE_HEIGHT, HALFSIZE[2] );
}

Vector3f Player::get_eye_direction() const
{
    return spherical_to_cartesian( Vector3f( 1.0f, pitch_, yaw_ ) );
}

void Player::get_potential_obstructions(
    const World& world,
    const Vector3f& origin,
    const Vector3f& sweep,
    const Vector3f& sweep_size,
    PotentialObstructionSet& potential_obstructions
)
{
    // This function slides a moving bounding box along its sweep vector in steps of one unit,
    // and collects all of the Blocks that the bounding box might intersect with along the way.
    // Right now it's overzealous, and probably returns more Blocks than it really needs to.

    const float sweep_length_squared = gmtl::lengthSquared( sweep );
    const Vector3f unit_sweep = sweep / gmtl::Math::sqrt( sweep_length_squared );
    Vector3f sweep_step;

    while ( gmtl::lengthSquared( sweep_step ) < sweep_length_squared )
    {
        const AABoxi index_bounds(
            vector_cast<int>( pointwise_floor( Vector3f( origin + sweep_step - Block::SIZE ) ) ),
            vector_cast<int>( pointwise_ceil( Vector3f( origin + sweep_step + Block::SIZE + sweep_size ) ) )
        );

        for ( int x = index_bounds.getMin()[0]; x < index_bounds.getMax()[0]; ++x )
        {
            for ( int y = index_bounds.getMin()[1]; y < index_bounds.getMax()[1]; ++y )
            {
                for ( int z = index_bounds.getMin()[2]; z < index_bounds.getMax()[2]; ++z )
                {
                    const Vector3i block_position( x, y, z );
                    const Block* block = world.get_block( block_position ).block_;

                    // TODO: Use a material attribute instead.
                    if ( block && block->get_material() != BLOCK_MATERIAL_AIR )
                    {
                        potential_obstructions.insert(
                            PotentialObstruction( vector_cast<Scalar>( block_position ), block )
                        );
                    }
                }
            }
        }

        sweep_step += unit_sweep;
    }
}

void Player::noclip_move_forward( const Scalar movement_units )
{
    position_ += get_eye_direction() * movement_units;
}

void Player::noclip_strafe( const Scalar movement_units )
{
    const float
        xd = movement_units * gmtl::Math::sin( yaw_ + gmtl::Math::PI_OVER_2 ),
        zd = movement_units * gmtl::Math::cos( yaw_ + gmtl::Math::PI_OVER_2 );

    position_ += Vector3f( xd, 0.0f, zd );
}
