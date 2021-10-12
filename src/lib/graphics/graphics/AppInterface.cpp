#include "AppInterface.h"


namespace ad {
namespace graphics {


AppInterface::AppInterface() :
    mWindowSize(0, 0),
    mFramebufferSize(0, 0)
{
    //
    // General OpenGL setups
    //

    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Frame buffer clear color
    glClearColor(0.1f, 0.2f, 0.3f, 1.f);
}


void AppInterface::callbackWindowSize(int width, int height)
{
    //glViewport(0, 0, width, height);
    mWindowSize.width() = width;
    mWindowSize.height() = height;

    //for (auto & listener : mSizeCallbacks)
    //{
    //    listener({width, height});
    //}
}

void AppInterface::callbackFramebufferSize(int width, int height)
{
    glViewport(0, 0, width, height);
    mFramebufferSize.width() = width;
    mFramebufferSize.height() = height;
    mFramebufferSizeSubject.dispatch(Size2<int>{width, height});
}

void AppInterface::clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

} // namespace graphics
} // namespace ad
