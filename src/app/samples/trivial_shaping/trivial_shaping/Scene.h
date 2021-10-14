#pragma once


#include <graphics/TrivialShaping.h>
#include <graphics/Timer.h>

#include <math/Angle.h>


namespace ad {
namespace graphics {


class Scene
{
public:
    Scene(Size2<int> aRenderResolution) :
        mTrivialShaping{std::move(aRenderResolution)}
    {}

    void step(const Timer & aTimer)
    {
        mTrivialShaping.clearShapes();

        mTrivialShaping.addRectangle({{{10.f, 20.f}, {100.f, 50.f}}, Color{255, 255, 255}});
        mTrivialShaping.addRectangle({{{100.f, 150.f}, {10.f, 90.f}}, Color{0, 255, 255}});

        if (static_cast<int>(aTimer.time()) % 2 == 0)
        {
            mTrivialShaping.addRectangle({ 
                {{200.f, 50.f}, {150.f, 120.f}}, 
                math::Degree<GLfloat>{15.},
                Color{255, 0, 255} });
        }
    }

    void render()
    {
        mTrivialShaping.render();
    }

private:
    TrivialShaping mTrivialShaping;
};


} // namespace graphics
} // namespace ad
