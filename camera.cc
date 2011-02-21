#include <GL/gl.h>

#include "camera.h"

Camera::Camera( Vector3f position, Scalar sensitivity, Scalar pitch, Scalar yaw ) :
    position_( position ),
    mouse_sensitivity_( sensitivity ),
    pitch_( pitch ),
    yaw_( yaw ),
    moving_fast_( false ),
    moving_forward_( false ),
    moving_backward_( false ),
    moving_left_( false ),
    moving_right_( false ),
    moving_up_( false ),
    moving_down_( false )
{
}

void Camera::render() const
{
    glRotatef( -pitch_, 1.0f, 0.0f, 0.0f );
    glRotatef( -yaw_, 0.0f, 1.0f, 0.0f );
    glTranslatef( -position_[0], -position_[1], -position_[2] );
}

void Camera::do_one_step( float step_time )
{
    Scalar movement_units = step_time * CAMERA_SPEED;

    if ( moving_fast_ ) movement_units *= CAMERA_FAST_MODE_FACTOR;

    if ( moving_forward_ ) move_forward( movement_units );
    if ( moving_backward_ ) move_forward( -movement_units );
    if ( moving_left_ ) strafe( movement_units );
    if ( moving_right_ ) strafe( -movement_units );
    if ( moving_up_ ) position_ += Vector3f( 0.0f, movement_units, 0.0f );
    if ( moving_down_ ) position_ -= Vector3f( 0.0f, movement_units, 0.0f );
}

void Camera::handle_mouse_motion( int xrel, int yrel )
{
    pitch_ -= mouse_sensitivity_ * Scalar( yrel );
    yaw_ -= mouse_sensitivity_ * Scalar( xrel );

    if ( yaw_ > 360.0f || yaw_ < -360.0f ) yaw_ = 0;
    if ( pitch_ > 90.0f ) pitch_ = 90.0f;
    else if ( pitch_ < -90.0f ) pitch_ = -90.f;
}

Vector3f Camera::get_direction()
{
    float
        pitch_radians = pitch_ * gmtl::Math::PI / 180.0f,
        yaw_radians = yaw_ * gmtl::Math::PI / 180.0f,
        yd = gmtl::Math::sin( pitch_radians ),
        forward_factor = gmtl::Math::cos( pitch_radians ),
        xd = -gmtl::Math::sin( yaw_radians ) * forward_factor,
        zd = -gmtl::Math::cos( yaw_radians ) * forward_factor;

    Vector3f direction = Vector3f( xd, yd, zd );
    gmtl::normalize( direction );
    return direction;
}

void Camera::move_forward( Scalar movement_units )
{
    position_ += get_direction() * movement_units;
}

void Camera::strafe( Scalar movement_units )
{
    float
        yaw_radians = yaw_ * gmtl::Math::PI / 180.0f + 0.5f * gmtl::Math::PI,
        xd = movement_units * -gmtl::Math::sin( yaw_radians ),
        zd = movement_units * -gmtl::Math::cos( yaw_radians );

    position_ += Vector3f( xd, 0.0f, zd );
}
