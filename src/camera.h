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

#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"

struct Camera
{
    Camera( const Vector3f& position, const Scalar pitch, const Scalar yaw, const Scalar draw_distance );

    void rotate() const;
    void translate() const;

    const Vector3f& get_position() const { return position_; }
    Scalar get_draw_distance() const { return draw_distance_; }

private:

    Vector3f position_;

    Scalar
        pitch_,
        yaw_,
        draw_distance_;
};

#endif // CAMERA_H
