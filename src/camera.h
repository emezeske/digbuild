#ifndef CAMERA_H
#define CAMERA_H

#include "sdl_gl_interface.h"
#include "math.h"

struct Camera
{
    Camera( Vector3f position, Scalar sensitivity, Scalar pitch, Scalar yaw );

    void rotate() const;
    void translate() const;
    void do_one_step( float step_time );
    void handle_mouse_motion( int xrel, int yrel );

    Vector3f get_direction() const;
    Vector3f get_position() const { return position_; }

    void fast_move_mode( bool m ) { moving_fast_ = m; }
    void move_forward( bool m ) { moving_forward_ = m; }
    void move_backward( bool m ) { moving_backward_ = m; }
    void move_left( bool m ) { moving_left_ = m; }
    void move_right( bool m ) { moving_right_ = m; }
    void move_up( bool m ) { moving_up_ = m; }
    void move_down( bool m ) { moving_down_ = m; }

private:

    void move_forward( Scalar movement_units );
    void strafe( Scalar movement_units );

    static const float
        CAMERA_SPEED = 30.0f,
        CAMERA_FAST_MODE_FACTOR = 5.0f;

    Vector3f position_;

    Scalar
        mouse_sensitivity_,
        pitch_,
        yaw_;

    bool
        moving_fast_,
        moving_forward_,
        moving_backward_,
        moving_left_,
        moving_right_,
        moving_up_,
        moving_down_;
};

#endif // CAMERA_H
