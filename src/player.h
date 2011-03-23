#ifndef PLAYER_H
#define PLAYER_H

#include <set>

#include "math.h"
#include "world.h"
#include "camera.h"

struct Player
{
    Player( const Vector3f& position, const Scalar pitch, const Scalar yaw );

    void do_one_step( const float step_time, const World& world );
    void adjust_direction( const Scalar dpitch, const Scalar dyaw );

    const Vector3f& get_position() const { return position_; }
    Vector3f get_eye_position() const;
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
    void toggle_noclip() { noclip_mode_ = !noclip_mode_; }

    Vector3f obstructing_block_position_; // FIXME For debugging.

private:

    static const Vector3f
        SIZE,
        HALFSIZE;

    static const float EYE_HEIGHT = 1.65f;

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
    void accelerate( const float step_time );
    void accelerate_lateral( const float step_time );
    void accelerate_vertical( const float step_time );
    bool find_collision( const World& world, const Vector3f& movement, BlockCollision& collision );
    void get_potential_obstructions( const World& world, const Vector3f& movement, PotentialObstructionSet& potential_obstructions );
    void resolve_collision( const Vector3f& movement, BlockCollision& collision );
    
    void noclip_move_forward( const Scalar movement_units );
    void noclip_strafe( const Scalar movement_units );

    static const float
        NOCLIP_SPEED = 30.0f,
        NOCLIP_FAST_MOVE_FACTOR = 5.0f,
        GROUND_ACCELERATION = 100.0f,
        AIR_ACCELERATION = 10.0f,
        GRAVITY_ACCELERATION = -30.0f,
        WALKING_SPEED = 5.0f,
        JUMP_VELOCITY = 9.0f;

    static const long
        JUMP_INTERVAL_MS = 300;

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
        noclip_mode_,
        feet_contacting_block_;

    long last_jumped_at_;
};

#endif // PLAYER_H
