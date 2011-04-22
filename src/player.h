#ifndef PLAYER_H
#define PLAYER_H

#ifdef DEBUG_COLLISIONS
# include <vector>
#endif

#include <set>

#include "math.h"
#include "world.h"

struct Player
{
    Player( const Vector3f& position, const Scalar pitch, const Scalar yaw );

    void do_one_step( const float step_time, World& world );
    void adjust_direction( const Scalar dpitch, const Scalar dyaw );

    const Vector3f& get_position() const { return position_; }
    Vector3f get_eye_position() const;
    Vector3f get_eye_direction() const;
    Scalar get_pitch() const { return pitch_; }
    Scalar get_yaw() const { return yaw_; }
    AABoxf get_aabb() const { return AABoxf( position_, position_ + SIZE ); }

    void request_fast_move( const bool r ) { requesting_fast_move_ = r; }
    void request_move_forward( const bool r ) { requesting_move_forward_ = r; }
    void request_move_backward( const bool r ) { requesting_move_backward_ = r; }
    void request_strafe_left( const bool r ) { requesting_strafe_left_ = r; }
    void request_strafe_right( const bool r ) { requesting_strafe_right_ = r; }
    void request_jump( const bool r ) { requesting_jump_ = r; }
    void request_crouch( const bool r ) { requesting_crouch_ = r; }
    void request_primary_fire( const bool r ) { requesting_primary_fire_ = r; }
    void request_secondary_fire( const bool r ) { requesting_secondary_fire_ = r; }

    void select_next_material();
    void select_previous_material();
    BlockMaterial get_material_selection() const { return material_selection_; }

    void toggle_noclip() { noclip_mode_ = !noclip_mode_; }

#ifdef DEBUG_COLLISIONS
    struct DebugCollision
    {
        Vector3f block_position_;
        CardinalRelation block_face_;
    };

    typedef std::vector<DebugCollision> DebugCollisionV;
    DebugCollisionV debug_collisions_;
#endif

private:

    static const Vector3f
        SIZE,
        HALFSIZE;

    struct TargetBlock
    {
        Vector3i
            block_position_,
            face_direction_;
    };

    struct BlockCollision
    {
        Scalar normalized_time_;
        Vector3f block_position_;
        CardinalRelation player_face_;
    };

    struct PotentialObstruction
    {
        PotentialObstruction( const Vector3f& block_position, const Block* block ) :
            block_position_( block_position ),
            block_( block )
        {
        }

        bool operator<( const PotentialObstruction& other ) const
        {
            if ( VectorLess<Vector3f>()( block_position_, other.block_position_ ) )
                return true;
            if ( block_position_ != other.block_position_ )
                return false;
            if ( block_ < other.block_ )
                return true;
            return false;
        }

        Vector3f block_position_;

        const Block* block_;
    };

    typedef std::set<PotentialObstruction> PotentialObstructionSet;

    void do_one_step_noclip( const float step_time );
    void do_one_step_clip( const float step_time, const World& world );

    void do_primary_fire( const float step_time, World& world );
    void do_secondary_fire( const float step_time, World& world );
    bool get_target_block( const Scalar max_distance, World& world, TargetBlock& target ) const;

    Vector3f get_acceleration();
    bool find_collision( const World& world, const Vector3f& movement, BlockCollision& collision );
    void resolve_collision( const Vector3f& movement, const Vector3f& dv, const BlockCollision& collision, Vector3f& acceleration );

    // TODO: Make get_potential_obstructions() take a LineSegF argument instead of origin+movement?
    void get_potential_obstructions(
        const World& world,
        const Vector3f& origin,
        const Vector3f& movement,
        const Vector3f& sweep_size,
        PotentialObstructionSet& potential_obstructions
    ) const;

    void noclip_move_forward( const Scalar movement_units );
    void noclip_strafe( const Scalar movement_units );

    static const float
        EYE_HEIGHT = 1.65f,
        NOCLIP_SPEED = 30.0f,
        NOCLIP_FAST_MOVE_FACTOR = 5.0f,
        GROUND_ACCELERATION = 35.0f,
        AIR_ACCELERATION = 10.0f,
        GRAVITY_ACCELERATION = -30.0f,
        WALKING_SPEED = 5.0f,
        JUMP_VELOCITY = 10.0f,
        PRIMARY_FIRE_DISTANCE = 3.0f,
        SECONDARY_FIRE_DISTANCE = 3.0f;

    static const long
        JUMP_INTERVAL_MS = 300,
        PRIMARY_FIRE_INTERVAL_MS = 300,
        SECONDARY_FIRE_INTERVAL_MS = 300;

    Vector3f
        position_,
        velocity_;

    Scalar
        pitch_,
        yaw_;

    bool
        requesting_fast_move_,
        requesting_move_forward_,
        requesting_move_backward_,
        requesting_strafe_left_,
        requesting_strafe_right_,
        requesting_jump_,
        requesting_crouch_,
        requesting_primary_fire_,
        requesting_secondary_fire_,
        noclip_mode_,
        feet_contacting_block_;

    BlockMaterial
        material_selection_;

    // TODO: Timer class
    long
        last_jump_at_,
        last_primary_fire_at_,
        last_secondary_fire_at_;
};

#endif // PLAYER_H
