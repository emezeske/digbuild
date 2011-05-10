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
