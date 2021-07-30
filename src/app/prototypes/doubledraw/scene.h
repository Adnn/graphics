#pragma once

#include "shaders.h"

#include <renderer/Drawing.h>
#include <renderer/Image.h>
#include <renderer/Shading.h>
#include <renderer/Texture.h>
#include <renderer/VertexSpecification.h>

#include <resource/PathProvider.h>

#include <math/Vector.h>

#include <glad/glad.h>


namespace ad
{

struct Vertex
{
    math::Vec<4, GLfloat> mPosition;
    math::Vec<2, GLfloat> mUV;
};

constexpr size_t gVerticesCount{4};

Vertex gVerticesEggman[gVerticesCount] = {
    Vertex{
        {-1.0f, .0f, 0.0f, 1.0f},
        {0.0f, 0.0f},
    },
    Vertex{
        {-1.0f,  1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f},
    },
    Vertex{
        { .0f, .0f, 0.0f, 1.0f},
        {1.0f, 0.0f},
    },
    Vertex{
        { .0f,  1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f},
    },
};

Vertex gVerticesRing[gVerticesCount] = {
    Vertex{
        {-.3f, -1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f},
    },
    Vertex{
        {-.3f,  .3f, 0.0f, 1.0f},
        {0.0f, 1.0f},
    },
    Vertex{
        { 1.0f, -1.0f, 0.0f, 1.0f},
        {1.0f, 0.0f},
    },
    Vertex{
        { 1.0f,  .3f, 0.0f, 1.0f},
        {1.0f, 1.0f},
    },
};


struct Entity
{
    Entity(DrawContext aDrawContext,
           std::function<void(Entity &, double)> aUpdater,
           std::function<void(const Entity &)> aDrawer) :
        mDrawContext(std::move(aDrawContext)),
        mUpdater(std::move(aUpdater)),
        mDrawer(std::move(aDrawer))
    {}

    void update(double aTime)
    {
        mUpdater(*this, aTime);
    }

    void draw() const
    {
        glBindVertexArray(mDrawContext.mVertexSpecification.mVertexArray);
        glUseProgram(mDrawContext.mProgram);
        mDrawer(*this);
    }

    DrawContext mDrawContext;
    std::function<void(Entity &, double)> mUpdater;
    std::function<void(const Entity &)> mDrawer;
};


typedef std::vector<Entity> Scene;

//struct [[nodiscard]] Scene
//{
//    Scene(VertexSpecification aVertexSpecification,
//          Texture aTexture,
//          Program aProgram) :
//        mVertexSpecification{std::move(aVertexSpecification)},
//        mTexture{std::move(aTexture)},
//        mProgram{std::move(aProgram)}
//    {}
//
//    VertexSpecification mVertexSpecification;
//    Texture mTexture;
//    Program mProgram;
//};
//

DrawContext staticEggman()
{
    static const Image eggman(pathFor("ec1ccd86c2ddb52.png").string());
    DrawContext drawing = [&](){
        VertexSpecification specification;
        glBindVertexArray(specification.mVertexArray);

        specification.mVertexBuffers.emplace_back(
                makeLoadedVertexBuffer(
                    {
                        {0, 4, offsetof(Vertex, mPosition), MappedGL<GLfloat>::enumerator},
                        {1, 2, offsetof(Vertex, mUV),       MappedGL<GLfloat>::enumerator},
                    },
                    sizeof(Vertex),
                    sizeof(gVerticesEggman),
                    gVerticesEggman
                ));

        //
        // Program
        //
        Program program = makeLinkedProgram({
                                  {GL_VERTEX_SHADER, gVertexShader},
                                  {GL_FRAGMENT_SHADER, gFragmentShader},
                              });
        glUseProgram(program);

        glUniform1i(glGetUniformLocation(program, "spriteSampler"), 1);

        return DrawContext{std::move(specification), std::move(program)};
    }();

    //
    // Texture
    //
    {
        // First-sprite
        // Found by measuring in the image raster
        Texture texture{GL_TEXTURE_2D};
        loadSprite(texture, GL_TEXTURE1, eggman);

        drawing.mTextures.push_back(std::move(texture));
    }

    return drawing;
}

DrawContext staticRing(const Image &aImage, const math::Size<2, int> aFrame)
{
    DrawContext drawing = [&](){
        VertexSpecification specification;
        glBindVertexArray(specification.mVertexArray);

        specification.mVertexBuffers.emplace_back(
                makeLoadedVertexBuffer(
                    {
                        {0, 4, offsetof(Vertex, mPosition), MappedGL<GLfloat>::enumerator},
                        {1, 2, offsetof(Vertex, mUV),       MappedGL<GLfloat>::enumerator},
                    },
                    sizeof(Vertex),
                    sizeof(gVerticesRing),
                    gVerticesRing
                ));

        //
        // Program
        //
        Program program = makeLinkedProgram({
                                  {GL_VERTEX_SHADER, gVertexShader},
                                  {GL_FRAGMENT_SHADER, gFragmentShader},
                              });
        glUseProgram(program);

        glUniform1i(glGetUniformLocation(program, "spriteSampler"), 1);

        return DrawContext{std::move(specification), std::move(program)};
    }();

    //
    // Texture
    //
    {
        // First-sprite
        // Found by measuring in the image raster
        Image firstRing = aImage.crop({{3, 3}, aFrame});
        Texture texture{GL_TEXTURE_2D};
        loadSprite(texture, GL_TEXTURE1, firstRing);

        drawing.mTextures.push_back(std::move(texture));
    }

    return drawing;
}

DrawContext animatedRing(const Image &aImage, const math::Size<2, int> aFrame)
{
    DrawContext drawing = [&](){
        VertexSpecification specification;
        glBindVertexArray(specification.mVertexArray);

        specification.mVertexBuffers.emplace_back(
                makeLoadedVertexBuffer(
                    {
                        {0, 4, offsetof(Vertex, mPosition), MappedGL<GLfloat>::enumerator},
                        {1, 2, offsetof(Vertex, mUV),       MappedGL<GLfloat>::enumerator},
                    },
                    sizeof(Vertex),
                    sizeof(gVerticesRing),
                    gVerticesRing
                ));

        //
        // Program
        //
        Program program = makeLinkedProgram({
                                  {GL_VERTEX_SHADER, gVertexShader},
                                  {GL_FRAGMENT_SHADER, gAnimationFragmentShader},
                              });
        glUseProgram(program);

        glUniform1i(glGetUniformLocation(program, "spriteSampler"), 2);

        return DrawContext{std::move(specification), std::move(program)};
    }();

    //
    // Texture
    //
    {
        // Complete animation
        std::vector<math::Position<2, int>> framePositions = {
                {3,    3},
                {353,  3},
                {703,  3},
                {1053, 3},
                {1403, 3},
                {1753, 3},
                {2103, 3},
                {2453, 3},
        };
        Image animationArray = aImage.prepareArray(framePositions, aFrame);

        // First-sprite
        // Found by measuring in the image raster
        Texture texture{GL_TEXTURE_2D_ARRAY};
        loadAnimationAsArray(texture, GL_TEXTURE2, animationArray, aFrame, framePositions.size());

        drawing.mTextures.push_back(std::move(texture));
    }

    return drawing;
}

void drawRing(const Entity &aEntity)
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, gVerticesCount);
}

void rotateRing(Entity &aEntity, double aTimeSeconds)
{
    constexpr int rotationsPerSec = 1;
    constexpr int frames = 8;
    glUniform1i(glGetUniformLocation(aEntity.mDrawContext.mProgram, "frame"),
                static_cast<int>(aTimeSeconds*rotationsPerSec*frames) % frames);
}

void noop(const Entity &)
{}

Scene setupScene()
{

    static const Image ring(
        pathFor("sonic_big_ring_1991_sprite_sheet_by_augustohirakodias_dc3iwce.png").string());

    //
    // Sub-parts
    //
    constexpr GLsizei width  = 347-3;
    constexpr GLsizei height = 303-3;

    Scene scene;
    scene.emplace_back(
              staticEggman(),
              [](Entity &, double){},
              drawRing
          );
    scene.emplace_back(
              animatedRing(ring, {width, height}),
              rotateRing,
              drawRing
          );

    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Frame buffer clear color
    glClearColor(0.1f, 0.2f, 0.3f, 1.f);

    return scene;
}

void updateScene(Scene &aScene, double aTimeSeconds)
{
    for (Entity & entity : aScene)
    {
        entity.update(aTimeSeconds);
    }
}

void renderScene(Scene &aScene)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    for (const Entity & entity : aScene)
    {
        entity.draw();
    }
}

} // namespace ad
