#include <limits>

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
    noclip_mode_( true ),
    feet_contacting_block_( false )
{
}

void Player::do_one_step( const float step_time, const World& world )
{
    if ( noclip_mode_ )
    {
        do_one_step_noclip( step_time );
    }
    else do_one_step_clip( step_time, world );
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

    for ( int steps = 0; steps < 3 && time_simulated + gmtl::GMTL_EPSILON < step_time; ++steps ) // TODO: Sufficient?
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
            time_simulated += step_time_slice;
            position_ += movement;
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
        if ( requesting_jump_ )
        {
            velocity_[1] += 8.0f; // TODO Temporary
        }
    }

    velocity_[1] += GRAVITY_ACCELERATION * step_time;
}

bool Player::find_collision( const World& world, const Vector3f& movement, BlockCollision& collision )
{
    // TODO: Decompose this function.

    bool collision_found = false;
    collision.normalized_time_ = std::numeric_limits<Scalar>::max();
    collision.block_position_ = Vector3f();

    PotentialObstructionSet potential_obstructions;
    get_potential_obstructions( world, movement, potential_obstructions );

    for ( PotentialObstructionSet::const_iterator obstruction_it = potential_obstructions.begin();
          obstruction_it != potential_obstructions.end();
          ++obstruction_it )
    {
        const Vector3f block_position = vector_cast<Scalar>( obstruction_it->block_position_ );
        const Block& block = *obstruction_it->block_;
        const AABoxf
            player_bounds = get_aabb(),
            block_bounds( block_position, block_position + Block::SIZE );

        Scalar
            normalized_first_contact,
            normalized_second_contact;

        const bool intersected = gmtl::intersect(
            player_bounds,
            movement,
            block_bounds,
            Vector3f(),
            normalized_first_contact,
            normalized_second_contact
        );

        if ( intersected &&
             normalized_first_contact >= 0.0f &&
             normalized_first_contact <= 1.0f &&
             normalized_first_contact < collision.normalized_time_ )
        {
            Scalar min_distance_squared = std::numeric_limits<Scalar>::max();
            Vector3f collision_normal;
            CardinalRelation collision_relation = CARDINAL_RELATION_BELOW;

            FOR_EACH_CARDINAL_RELATION( relation )
            {
                const Vector3f
                    player_centroid = position_ + HALFSIZE,
                    block_centroid = block_position + Block::HALFSIZE,
                    player_normal = vector_cast<Scalar>( cardinal_relation_vector( relation ) ),
                    block_normal = -player_normal,
                    player_plane_point = player_centroid + pointwise_product( player_normal, HALFSIZE ),
                    block_plane_point = block_centroid + pointwise_product( block_normal, Block::HALFSIZE ),
                    block_face_to_player_face = player_plane_point - block_plane_point;

                const Scalar distance_squared = gmtl::lengthSquared( block_face_to_player_face );

                if ( distance_squared < min_distance_squared )
                {
                    min_distance_squared = distance_squared;
                    collision_normal = player_normal;
                    collision_relation = relation;
                }
            }

            if ( collision_relation == CARDINAL_RELATION_BELOW )
            {
                feet_contacting_block_ = true;
            }

            // If normalized_first_contact is very small, it indicates that the Player was already
            // intersecting with the Block before it moved.  In this case, the collision is ignored
            // if the Player's velocity is directed away from the block.
            if ( normalized_first_contact > 1.0e-6f || gmtl::dot( movement, collision_normal ) > 0.001f )
            {
                collision_found = true;
                collision.normalized_time_ = normalized_first_contact;
                collision.block_position_ = block_position;
                collision.player_face_ = collision_relation;
            }
        }
    }

    return collision_found;
}

void Player::get_potential_obstructions( const World& world, const Vector3f& movement, PotentialObstructionSet& potential_obstructions )
{
    // This function slides the Player's bounding box along its movement vector in steps
    // of one unit, and collects all of the Blocks that the bounding box might intersect
    // with along the way.  Right now it's overzealous, and probably returns more Blocks
    // than it really needs to.

    const float movement_magnitude_squared = gmtl::lengthSquared( movement );
    const Vector3f unit_movement = movement / gmtl::Math::sqrt( movement_magnitude_squared );
    Vector3f movement_step;

    while ( gmtl::lengthSquared( movement_step ) < movement_magnitude_squared )
    {
        const AABoxi index_bounds(
            vector_cast<int>( pointwise_floor( Vector3f( position_ + movement_step - Block::SIZE ) ) ),
            vector_cast<int>( pointwise_ceil( Vector3f( position_ + movement_step + SIZE + Block::SIZE ) ) )
        );

        for ( int x = index_bounds.getMin()[0]; x < index_bounds.getMax()[0]; ++x )
        {
            for ( int y = index_bounds.getMin()[1]; y < index_bounds.getMax()[1]; ++y )
            {
                for ( int z = index_bounds.getMin()[2]; z < index_bounds.getMax()[2]; ++z )
                {
                    const Vector3i block_position( x, y, z );
                    const Block* block = world.get_block( block_position );

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

        movement_step += unit_movement;
    }
}

void Player::resolve_collision( const Vector3f& movement, BlockCollision& collision )
{
    // FIXME debugging
    // std::cout << "p: " << position_ << ", m: " << movement << ", nt: " << collision.normalized_time_ << ", bp: " << collision.block_position_ << std::endl;
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

void Player::noclip_move_forward( const Scalar movement_units )
{
    position_ += spherical_to_cartesian( Vector3f( 1.0f, pitch_, yaw_ ) ) * movement_units;
}

void Player::noclip_strafe( const Scalar movement_units )
{
    const float
        xd = movement_units * gmtl::Math::sin( yaw_ + gmtl::Math::PI_OVER_2 ),
        zd = movement_units * gmtl::Math::cos( yaw_ + gmtl::Math::PI_OVER_2 );

    position_ += Vector3f( xd, 0.0f, zd );
}
