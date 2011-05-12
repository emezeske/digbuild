///////////////////////////////////////////////////////////////////////////
// Copyright 2011 Evan Mezeske.
//
// This file is part of Digbuild.
// 
// Digbuild is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.
// 
// Digbuild is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Digbuild.  If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////

#include <SDL/SDL.h>

#include <boost/foreach.hpp>

#include <limits>
#include <iomanip>

#include "log.h"
#include "cardinal_relation.h"
#include "player.h"

//////////////////////////////////////////////////////////////////////////////////
// Static constant definitions for Player:
//////////////////////////////////////////////////////////////////////////////////

const Vector3f
    Player::SIZE( 0.50f, 1.9f, 0.50f ),
    Player::HALFSIZE( SIZE / 2.0f );

const float
    Player::EYE_HEIGHT,
    Player::NOCLIP_SPEED,
    Player::NOCLIP_SPRINT_FACTOR,
    Player::GROUND_ACCELERATION,
    Player::AIR_ACCELERATION,
    Player::SWIMMING_ACCELERATION,
    Player::GRAVITY_ACCELERATION,
    Player::WALKING_SPEED,
    Player::SWIMMING_SPEED,
    Player::SPRINT_FACTOR,
    Player::JUMP_VELOCITY,
    Player::PRIMARY_FIRE_DISTANCE,
    Player::SECONDARY_FIRE_DISTANCE;

const long
    Player::JUMP_INTERVAL_MS,
    Player::PRIMARY_FIRE_INTERVAL_MS;

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Player:
//////////////////////////////////////////////////////////////////////////////////

Player::Player( const Vector3f& position, const Scalar pitch, const Scalar yaw ) :
    position_( position ),
    pitch_( pitch ),
    yaw_( yaw ),
    requesting_move_forward_( false ),
    requesting_move_backward_( false ),
    requesting_strafe_left_( false ),
    requesting_strafe_right_( false ),
    requesting_jump_( false ),
    requesting_walk_( false ),
    requesting_sprint_( false ),
    requesting_primary_fire_( false ),
    requesting_secondary_fire_( false ),
    noclip_mode_( true ),
    feet_contacting_block_( false ),
    material_selection_( BLOCK_MATERIAL_GRASS ),
    last_jump_at_( 0 ),
    last_primary_fire_at_( 0 ),
    last_secondary_fire_at_( 0 )
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
    do_secondary_fire( step_time, world );
}

void Player::select_next_material()
{
    const int material = material_selection_ + 1;

    if ( material == NUM_BLOCK_MATERIALS )
    {
        material_selection_ = BlockMaterial( 0 );
    }
    else material_selection_ = BlockMaterial( material );
}

void Player::select_previous_material()
{
    if ( material_selection_ == BLOCK_MATERIAL_AIR )
    {
        material_selection_ = BlockMaterial( NUM_BLOCK_MATERIALS - 1 );
    }
    else material_selection_ = BlockMaterial( material_selection_ - 1 );
}

void Player::do_one_step_noclip( const float step_time )
{
    velocity_ = Vector3f();

    Scalar movement_units = step_time * NOCLIP_SPEED;
    if ( requesting_sprint_ ) movement_units *= NOCLIP_SPRINT_FACTOR;

    if ( requesting_move_forward_ ) noclip_move_forward( movement_units );
    if ( requesting_move_backward_ ) noclip_move_forward( -movement_units );
    if ( requesting_strafe_left_ ) noclip_strafe( movement_units );
    if ( requesting_strafe_right_ ) noclip_strafe( -movement_units );
    if ( requesting_jump_ ) position_ += Vector3f( 0.0f, movement_units, 0.0f );
    if ( requesting_walk_ ) position_ -= Vector3f( 0.0f, movement_units, 0.0f );
}

void Player::do_one_step_clip( const float step_time, const World& world )
{
#ifdef DEBUG_COLLISIONS
    debug_collisions_.clear();
#endif

    const bool swimming = is_swimming( world );
    Vector3f acceleration = get_acceleration( world, swimming );
    feet_contacting_block_ = false;
    Scalar time_simulated = 0.0f;

    // A maximum of three integrator steps are simulated here, with the reasoning that
    // three steps can correctly resolve a collision occuring in the pocket-style corner
    // formed by three blocks.

    for ( int steps = 0; steps < 3 && time_simulated < step_time; ++steps )
    {
        const Scalar step_time_slice = ( step_time - time_simulated );
        const Vector3f
            dv = acceleration * step_time_slice,
            movement = ( velocity_ + dv ) * step_time_slice;

        BlockCollision collision;

        if ( find_collision( world, movement, collision ) )
        {
            resolve_collision( movement, dv, collision, acceleration );
            time_simulated += collision.normalized_time_ * step_time_slice;
        }
        else
        {
            velocity_ += dv;
            position_ += movement;
            break;
        }
    }

    if ( !swimming && feet_contacting_block_ )
    {
        const long now = SDL_GetTicks();

        if ( requesting_jump_ && last_jump_at_ + JUMP_INTERVAL_MS < now )
        {
            last_jump_at_ = now;
            velocity_[1] += JUMP_VELOCITY;
        }
    }
}

void Player::do_primary_fire( const float step_time, World& world )
{
    const long now = SDL_GetTicks();

    if ( requesting_primary_fire_ && last_primary_fire_at_ + PRIMARY_FIRE_INTERVAL_MS < now )
    {
        last_primary_fire_at_ = now;

        TargetBlock target;

        if ( get_target_block( PRIMARY_FIRE_DISTANCE, world, target ) )
        {
            BlockIterator block_it = world.get_block( target.block_position_ );
            assert( block_it.block_ );
            block_it.block_->set_material( BLOCK_MATERIAL_AIR );
            world.mark_chunk_for_update( block_it.chunk_ ); 
        }
    }
}

void Player::do_secondary_fire( const float step_time, World& world )
{
    const long now = SDL_GetTicks();

    if ( requesting_secondary_fire_ && last_secondary_fire_at_ + SECONDARY_FIRE_INTERVAL_MS < now )
    {
        last_secondary_fire_at_ = now;

        TargetBlock target;

        if ( get_target_block( SECONDARY_FIRE_DISTANCE, world, target ) )
        {
            const Vector3i new_block_position = target.block_position_ + target.face_direction_;

            const AABoxf
                player_bounds = get_aabb(),
                block_bounds( vector_cast<Scalar>( new_block_position ), vector_cast<Scalar>( new_block_position ) + Block::SIZE );

            if ( get_block_material_attributes( material_selection_ ).collision_mode_ != BLOCK_COLLISION_MODE_SOLID ||
                 !gmtl::intersect( player_bounds, block_bounds ) )
            {
                BlockIterator block_it = world.get_block( new_block_position );

                if ( !block_it.block_ )
                {
                    const Vector3i new_block_index = world.get_block_index( new_block_position );
                    world.extend_chunk_column( new_block_position - new_block_index );
                    block_it = world.get_block( new_block_position );
                    assert( block_it.block_ );
                }

                if ( block_it.block_->get_collision_mode() != BLOCK_COLLISION_MODE_SOLID )
                {
                    block_it.block_->set_material( material_selection_ );

                    // FIXME: For testing:
                    if ( material_selection_ == BLOCK_MATERIAL_WATER ||
                         material_selection_ == BLOCK_MATERIAL_LAVA )
                    {
                        BlockDataFlowable( *block_it.block_ ).make_source();
                    }

                    world.mark_chunk_for_update( block_it.chunk_ ); 
                }
            }
        }
    }
}

bool Player::get_target_block( const Scalar max_distance, World& world, TargetBlock& target ) const
{
    const gmtl::LineSegf sweep( get_eye_position(), Vector3f( get_eye_direction() * max_distance ) );
    PotentialObstructionSet potential_obstructions;
    get_potential_obstructions(
        world,
        sweep,
        Vector3f(),
        BLOCK_COLLISION_MODE_SOLID,
        potential_obstructions
    );

    Scalar normalized_hit_time = std::numeric_limits<Scalar>::max();
    bool target_block_found = false;

    BOOST_FOREACH( const PotentialObstruction& obstruction, potential_obstructions )
    {
        const Vector3f block_position = obstruction.block_position_;
        const AABoxf block_bounds( block_position, block_position + Block::SIZE );

        unsigned num_hits;

        Scalar
            normalized_time_in,
            normalized_time_out;

        const bool intersected =
            gmtl::intersect( sweep, block_bounds, num_hits, normalized_time_in, normalized_time_out );

        if ( intersected && normalized_time_in < normalized_hit_time )
        {
            normalized_hit_time = normalized_time_in;
            target_block_found = true;
            target.block_position_ = vector_cast<int>( block_position );
        }
    }

    if ( target_block_found )
    {
        const Vector3f
            intersection_position = sweep.mOrigin + normalized_hit_time * sweep.mDir,
            block_centroid = vector_cast<Scalar>( target.block_position_ ) + 0.5f * vector_cast<Scalar>( Block::SIZE ),
            centroid_to_intersection = intersection_position - block_centroid;

        const unsigned major = major_axis( centroid_to_intersection );
        target.face_direction_ = Vector3i();
        target.face_direction_[major] = ( centroid_to_intersection[major] > 0.0f ) ? 1 : -1;
    }

    return target_block_found;
}

Vector3f Player::get_acceleration( const World& world, const bool swimming )
{
    Vector3f
        target_velocity,
        acceleration;

    Scalar
        max_acceleration,
        target_speed;

    const Scalar strafe_angle = yaw_ + gmtl::Math::PI_OVER_2;
    const Vector3f strafe_direction( gmtl::Math::sin( strafe_angle ), 0.0f, gmtl::Math::cos( strafe_angle ) );
    if ( requesting_strafe_left_ ) target_velocity += strafe_direction;
    if ( requesting_strafe_right_ ) target_velocity -= strafe_direction;

    if ( swimming )
    {
        if ( requesting_move_forward_ ) target_velocity += get_eye_direction();
        if ( requesting_move_backward_ ) target_velocity -= get_eye_direction();
        if ( requesting_jump_ ) target_velocity += Vector3f( 0.0f, 1.0f, 0.0f );
        if ( requesting_walk_ ) target_velocity -= Vector3f( 0.0f, 1.0f, 0.0f );

        target_speed = SWIMMING_SPEED;
        if ( requesting_sprint_ ) target_speed *= SPRINT_FACTOR;

        gmtl::normalize( target_velocity );
        target_velocity *= target_speed;

        max_acceleration = SWIMMING_ACCELERATION;
    }
    else
    {
        const Vector3f forward_direction( gmtl::Math::sin( yaw_ ), 0.0f, gmtl::Math::cos( yaw_ ) );

        if ( requesting_move_forward_ ) target_velocity += forward_direction;
        if ( requesting_move_backward_ ) target_velocity -= forward_direction;

        target_speed = WALKING_SPEED;
        if ( requesting_sprint_ ) target_speed *= SPRINT_FACTOR;

        gmtl::normalize( target_velocity );
        target_velocity *= target_speed;
        target_velocity[1] = velocity_[1];

        acceleration[1] = GRAVITY_ACCELERATION;
        max_acceleration = ( feet_contacting_block_ ? GROUND_ACCELERATION : AIR_ACCELERATION );
    }

    Vector3f acceleration_direction = ( target_velocity - velocity_ );
    const Scalar velocity_difference = gmtl::length( acceleration_direction );
    gmtl::normalize( acceleration_direction );

    const Scalar acceleration_power =
        max_acceleration * std::min( velocity_difference / target_speed, 1.0f );

    acceleration += acceleration_direction * acceleration_power;
    return acceleration;
}

bool Player::is_swimming( const World& world ) const
{
    PotentialObstructionSet potential_obstructions;
    get_potential_obstructions(
        world,
        gmtl::LineSegf( position_, Vector3f() ),
        vector_cast<Scalar>( SIZE ),
        BLOCK_COLLISION_MODE_FLUID,
        potential_obstructions
    );

    BOOST_FOREACH( const PotentialObstruction& obstruction, potential_obstructions )
    {
        const Vector3f block_position = obstruction.block_position_;
        const Block& block = *obstruction.block_;
        const AABoxf
            player_bounds = get_aabb(),
            block_bounds( block_position, block_position + Block::SIZE );

        if ( gmtl::intersect( player_bounds, block_bounds ) )
        {
            return true;
        }
    }

    return false;
}

bool Player::find_collision( const World& world, const Vector3f& movement, BlockCollision& collision )
{
    // TODO: Decompose this function.

    bool collision_found = false;
    collision.normalized_time_ = std::numeric_limits<Scalar>::max();
    collision.block_position_ = Vector3f();

    PotentialObstructionSet potential_obstructions;
    get_potential_obstructions(
        world,
        gmtl::LineSegf( position_, movement ),
        SIZE,
        BLOCK_COLLISION_MODE_SOLID,
        potential_obstructions
    );

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
            Scalar min_dplane_offset = std::numeric_limits<Scalar>::max();
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

                if ( !block_neighbor ||
                      block_neighbor->get_collision_mode() != BLOCK_COLLISION_MODE_SOLID )
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

                    if ( dplane_offset < min_dplane_offset )
                    {
                        min_dplane_offset = dplane_offset;
                        collision_normal = player_normal;
                        collision_relation = relation;
                    }
                }
            }

            const AABoxf contact_player_bounds(
                player_bounds.getMin() + normalized_first_contact * movement,
                player_bounds.getMax() + normalized_first_contact * movement
            );

            const Scalar planar_overlap =  min_planar_overlap( block_bounds, contact_player_bounds, collision_normal );

            // Generally it's not desirable for a collision to occur if the Player and Block just
            // barely have an edge or corner overlapping.  Allowing such collisions results in odd
            // behavior when the Player is e.g. against a wall and trying to jump.  Throw them out.
            if ( planar_overlap > 0.01f )
            {
                // Throw out the collision unless the minimum difference between plane offsets is fairly small.
                // This is a heuristic that helps avoid some degenerate cases arising from Blocks with very
                // few visible faces (where the "closest" face might be far away).
                if ( min_dplane_offset < 0.1f )
                {
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
        }
    }

    return collision_found;
}

void Player::resolve_collision( const Vector3f& movement, const Vector3f& dv, const BlockCollision& collision, Vector3f& acceleration )
{
#ifdef DEBUG_COLLISIONS
    DebugCollision debug_collision;
    debug_collision.block_position_ = collision.block_position_;
    debug_collision.block_face_ = cardinal_relation_reverse( collision.player_face_ );
    debug_collisions_.push_back( debug_collision );
    debug_collisions_.push_back( debug_collision );
#endif

    if ( collision.player_face_ == CARDINAL_RELATION_BELOW )
    {
        feet_contacting_block_ = true;
    }

    velocity_ += dv * collision.normalized_time_;
    position_ += movement * collision.normalized_time_;

    const Vector3f normal =
        vector_cast<Scalar>( cardinal_relation_vector( collision.player_face_ ) ),
        velocity_collision_component = gmtl::dot( velocity_, normal ) * normal,
        acceleration_collision_component = gmtl::dot( acceleration, normal ) * normal;

    // Reverse the component of the Player's velocity that is directed toward the
    // block, and apply a little rebound as well.
    velocity_ -= 1.05f * velocity_collision_component;

    // Do the same thing with the Player's acceleration.  This isn't physically accurate,
    // but it makes the collision solver work better.
    acceleration -= 1.05f * acceleration_collision_component;
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
    const gmtl::LineSegf& sweep,
    const Vector3f& swept_box_size,
    const BlockCollisionMode collision_mode,
    PotentialObstructionSet& potential_obstructions
) const
{
    // This function slides a moving bounding box along its sweep vector in steps of one unit,
    // and collects all of the Blocks that the bounding box might intersect with along the way.
    // Right now it's overzealous, and probably returns more Blocks than it really needs to.

    const float
        sweep_length_squared = gmtl::lengthSquared( sweep.mDir ),
        sweep_length =  gmtl::Math::sqrt( sweep_length_squared );

    const Vector3f unit_sweep = ( sweep_length == 0.0f ) ? Vector3f() : sweep.mDir / sweep_length;
    Vector3f sweep_step;

    do
    {
        const AABoxi index_bounds(
            vector_cast<int>( pointwise_floor( Vector3f( sweep.mOrigin + sweep_step - Block::SIZE ) ) ),
            vector_cast<int>( pointwise_ceil( Vector3f( sweep.mOrigin + sweep_step + Block::SIZE + swept_box_size ) ) )
        );

        for ( int x = index_bounds.getMin()[0]; x < index_bounds.getMax()[0]; ++x )
        {
            for ( int y = index_bounds.getMin()[1]; y < index_bounds.getMax()[1]; ++y )
            {
                for ( int z = index_bounds.getMin()[2]; z < index_bounds.getMax()[2]; ++z )
                {
                    const Vector3i block_position( x, y, z );
                    const Block* block = world.get_block( block_position ).block_;

                    if ( block && block->get_collision_mode() == collision_mode )
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
    while ( gmtl::lengthSquared( sweep_step ) < sweep_length_squared );
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
