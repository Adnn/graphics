#pragma once

#include "shaders.h"
#include "vertices.h"

#include <arte/Image.h>

#include <graphics/AppInterface.h>
#include <graphics/Timer.h>

#include <renderer/FrameBuffer.h>
#include <renderer/Shading.h>
#include <renderer/Texture.h>
#include <renderer/TextureUtilities.h>
#include <renderer/VertexSpecification.h>

#include <test_commons/PathProvider.h>

namespace ad {
namespace graphics {

struct ScreenQuad
{
    ScreenQuad();

    VertexSpecification mVertexSpec;
};

inline ScreenQuad::ScreenQuad()
{
    mVertexSpec.mVertexBuffers.push_back(
        loadVertexBuffer(mVertexSpec.mVertexArray,
                         gVertexScreenDescription,
                         std::span<VertexScreenQuad>{gVerticesScreen},
                         BufferHint::StaticDraw));
}

struct CompleteFrameBuffer
{
    FrameBuffer frameBuffer;
    Texture colorTexture{GL_TEXTURE_2D};
};

template <std::size_t T_size>
std::array<CompleteFrameBuffer, T_size> initFrameBuffers(Size2<int> aSize)
{
    constexpr std::size_t gChainSize = T_size;
    std::array<CompleteFrameBuffer, gChainSize> buffers;

    for (auto & [frameBuffer, texture] : buffers)
    {
        allocateStorage(texture, GL_RGBA8, aSize.width(), aSize.height());
        attachImage(frameBuffer, texture, GL_COLOR_ATTACHMENT0, 0);
    }
    return buffers;
}

struct FirstRender
{
    FirstRender();

    VertexArrayObject mVAO;
    VertexBufferObject mVertexData;
    Program mProgram;
};

inline FirstRender::FirstRender() :
    mVAO(),
    mVertexData(loadVertexBuffer(mVAO, 
                                 gVertexSceneDescription,
                                 std::span<VertexScene>{gVerticesScene},
                                 BufferHint::StaticDraw)),
    mProgram(makeLinkedProgram({
        {GL_VERTEX_SHADER, gInitialVertex},
        {GL_FRAGMENT_SHADER, gInitialFragment}}))
{}

struct Scene
{
    Scene(const char * argv[], const AppInterface * aAppInterface);
    void blur();
    void step(const Timer & aTimer);

    const AppInterface * mAppInterface;
    FirstRender mInitial;
    Size2<int> mRenderTargetsSize;
    ScreenQuad mScreenQuad;
    //std::array<Texture, 2> mColors{GL_TEXTURE_RECTANGLE, GL_TEXTURE_RECTANGLE};
    std::array<CompleteFrameBuffer, 2> mBuffers;
    CompleteFrameBuffer mSceneFB;
    Texture mNeonTexture;
    Program mHBlurProgram;
    Program mVBlurProgram;
    Program mScreenProgram;
};

Scene::Scene(const char * argv[], const AppInterface * aAppInterface) :
    mAppInterface(aAppInterface),
    mInitial(),
    // Note: Renders target are of fixed size, not changing with window's frambuffer size
    mRenderTargetsSize(aAppInterface->getFramebufferSize().width(),
                       aAppInterface->getFramebufferSize().height()),
    mBuffers(initFrameBuffers<2>(mRenderTargetsSize)),
    mSceneFB(),
    mNeonTexture(GL_TEXTURE_2D),
    mHBlurProgram(makeLinkedProgram({
        {GL_VERTEX_SHADER, gScreenVertex},
        {GL_FRAGMENT_SHADER, gHBlurFragment}})),
    mVBlurProgram(makeLinkedProgram({
        {GL_VERTEX_SHADER, gScreenVertex},
        {GL_FRAGMENT_SHADER, gVBlurFragment}})),
    mScreenProgram(makeLinkedProgram({
        {GL_VERTEX_SHADER, gScreenVertex},
        {GL_FRAGMENT_SHADER, gScreenFragment}}))
{
    {
        auto & [frameBuffer, texture] = mSceneFB;
        allocateStorage(texture, GL_RGBA8, mRenderTargetsSize.width(), mRenderTargetsSize.height());

        attachImage(frameBuffer, texture, GL_COLOR_ATTACHMENT0);
        attachImage(frameBuffer, mBuffers.at(0).colorTexture, GL_COLOR_ATTACHMENT1);

        // This list of draw buffer is part of the FBO state
        // see: https://stackoverflow.com/a/34973291/1027706
        ScopedBind bound{frameBuffer};
        unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, attachments);
    }

    // Texture
    {
        loadImage(mNeonTexture,
                  arte::ImageRgba{resource::pathFor("st_outline.png").string(),
                                  arte::ImageOrientation::InvertVerticalAxis});
    }
}

inline void Scene::step(const Timer & aTimer)
{
    /***
     * Rendering the scene to a texture (initial rendering)
     ***/
    // TODO first render should have depth buffer enabled, to be generic
    glBindFramebuffer(GL_FRAMEBUFFER, mSceneFB.frameBuffer);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // A different clear color for the GL_COLOR_ATTACHMENT1: black baground instead of blue
    // TODO Why blur stop working if alpha is 0?
    static const std::array<float, 4> gClearColor = {0.f, 0.f, 0.f, 1.f};
    glClearBufferfv(GL_COLOR, 1, gClearColor.data());

    glBindVertexArray(mInitial.mVAO);
    glUseProgram(mInitial.mProgram);

    // Neon texture is bound to GL_TEXTURE0 unit
    glActiveTexture(GL_TEXTURE0);
    bind(mNeonTexture);

    // glViewport is a global, providing the actual size of the render target
    // It is defaulted to the default framebuffer size, so it must be explicitly set in situations
    // where the active render target(s) size do not matche default FB size.
    // see: https://stackoverflow.com/a/33721834/1027706
    glViewport(0, 0, mRenderTargetsSize.width(), mRenderTargetsSize.height());

    glDrawArraysInstanced(GL_TRIANGLE_STRIP,
                          0,
                          4,
                          1);


    /* It is about rendering to screen quad for both remaining steps */
    glBindVertexArray(mScreenQuad.mVertexSpec.mVertexArray);

    /***
     * Filtering the rendered texture
     ***/

    // TODO should select the correct texture depending on the ping pong
    // or could be done only once if always using the same texture
    glProgramUniform1i(mHBlurProgram,
                       glGetUniformLocation(mHBlurProgram, "screenTexture"),
                       0);
    glProgramUniform1i(mVBlurProgram,
                       glGetUniformLocation(mVBlurProgram, "screenTexture"),
                       1);

    glActiveTexture(GL_TEXTURE0); // Already active, but good practice
    bind(mBuffers.at(0).colorTexture);

    glActiveTexture(GL_TEXTURE1);
    bind(mBuffers.at(1).colorTexture);

    for (int i = 0;
         // The blinking effect: bloom on odd second, no bloom on even second
         i != (static_cast<int>(aTimer.mTime)%2 == 0 ? 0 : 2);
         ++i)
    {
        glUseProgram(i%2 ? mVBlurProgram : mHBlurProgram);
        glBindFramebuffer(GL_FRAMEBUFFER, mBuffers.at((i+1)%2).frameBuffer);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }


    /***
     * Drawing to the screen
     ***/
    // TODO Can we reuse the program, changing only the fragment shader?
    // Binds the default (window) framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(mScreenProgram);

    // Scene texture (initial render) will be bound to unit 0, filtered texture to unit 1
    glProgramUniform1i(mScreenProgram,
                       glGetUniformLocation(mScreenProgram, "sceneTexture"),
                       0);
    glProgramUniform1i(mScreenProgram,
                       glGetUniformLocation(mScreenProgram, "bloomTexture"),
                       1);

    glActiveTexture(GL_TEXTURE0);
    bind(mSceneFB.colorTexture);

    // Last bloom pass rendered to first buffer
    glActiveTexture(GL_TEXTURE1);
    bind(mBuffers.at(0).colorTexture);

    // Viewport is global state, needs to be restored to the windows's framebuffer size
    const int width{mAppInterface->getFramebufferSize().width()};
    const int height{mAppInterface->getFramebufferSize().height()};
    glViewport(0, 0, width, height);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


} // namespace graphics
} // namespace ad
