#pragma once

#include <math/Rectangle.h>
#include <math/Vector.h>


namespace ad {
namespace graphics {


using MacroDefine = std::string;


// TODO Ad 2024/11/21: Remove those aliases below
template <class T_number>
using Rectangle=math::Rectangle<T_number>;

template <class T_number>
using Vec2=math::Vec<2, T_number>;

template <class T_number>
using Vec3=math::Vec<3, T_number>;

template <class T_number>
using Vec4=math::Vec<4, T_number>;

template <class T_number>
using Size2=math::Size<2, T_number>;

template <class T_number>
using Position2=math::Position<2, T_number>;

template <class T_number>
using Position3=math::Position<3, T_number>;

static const std::array<float, 4> gBlack = {0.f, 0.f, 0.f, 1.f};


} // namespace graphics
} // namespace ad
