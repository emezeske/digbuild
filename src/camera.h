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
