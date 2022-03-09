#pragma once


#include <math/Angle.h>


namespace ad {
namespace gltfviewer {


struct Polar
{
    GLfloat r;
    math::Radian<GLfloat> polar{math::pi<GLfloat> / 2.f};
    math::Radian<GLfloat> azimuthal{0.f};

    math::Position<3, GLfloat> toCartesian() const
    {
        return {
            r * sin(azimuthal) * sin(polar),
            r * cos(polar),
            r * cos(azimuthal) * sin(polar),
        };
    }

    math::Vec<3, GLfloat> getUpTangent() const
    {
        using namespace math::angle_literals;

        if (polar < math::pi<math::Radian<GLfloat>> / 2.f)
        {
            Polar under = *this;
            under.polar += 0.1_radf;
            return (toCartesian() - under.toCartesian()).normalize();
        }
        else
        {
            Polar above = *this;
            above.polar -= 0.1_radf;
            return (above.toCartesian() - toCartesian()).normalize();
        }
    }
};


} // namespace gltfviewer
} // namespace ad
