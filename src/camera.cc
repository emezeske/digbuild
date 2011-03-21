#include <GL/glew.h>

#include "camera.h"

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for Camera:
//////////////////////////////////////////////////////////////////////////////////

Camera::Camera( const Vector3f& position, const Scalar pitch, const Scalar yaw, const Scalar draw_distance ) :
    position_( position ),
    pitch_( pitch ),
    yaw_( yaw ),
    draw_distance_( draw_distance )
{
}

void Camera::rotate() const
{
    glRotatef( -90.0f + ( 180.0f * ( pitch_ / gmtl::Math::PI ) ), 1.0f, 0.0f, 0.0f );
    glRotatef( 180.0f - ( 180.0f * ( yaw_ / gmtl::Math::PI ) ), 0.0f, 1.0f, 0.0f );
}

void Camera::translate() const
{
    glTranslatef( -position_[0], -position_[1], -position_[2] );
}
