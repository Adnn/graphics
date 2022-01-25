#pragma once

#include "commons.h"

#include "Sprite.h"

#include <math/Homogeneous.h>

#include <renderer/Drawing.h>

#include <glad/glad.h>

#include <vector>


namespace ad {
namespace graphics {


/// \brief Draws a list of sprites (loaded from a single spritesheet with ::load()) at given positions.
///
/// The instance data is a span of association between a (rendering) position and a sprite (in the spritesheet).
class Spriting
{
public:
    struct Instance
    {
        Instance(Position2<GLfloat> aRenderingPosition, LoadedSprite aSprite, GLfloat aOpacity = 1.f, Vec2<int> aAxisMirroring = {1, 1}):
            mPosition{std::move(aRenderingPosition)},
            mLoadedSprite{std::move(aSprite)},
            mOpacity{aOpacity},
            mAxisMirroring{std::move(aAxisMirroring)}
        {}

        Instance & mirrorHorizontal(bool aMirror = true)
        {
            mAxisMirroring.x() = aMirror ? -1 : 1;
            return *this; 
        }

        Instance & mirrorVertical(bool aMirror = true)
        {
            mAxisMirroring.y() = aMirror ? -1 : 1;
            return *this; 
        }

        Position2<GLfloat> mPosition;
        LoadedSprite mLoadedSprite;
        GLfloat mOpacity;
        Vec2<int> mAxisMirroring;
    };

    Spriting(GLfloat aPixelSize = 1.f);

    void updateInstances(gsl::span<const Instance> aInstances);

    void render(const sprite::LoadedAtlas & aAtlas) const;

    /// \brief Define the size of a pixel in world units.
    /// 
    /// When rendering pixel art, it is likely that one sprite pixel should always be the same world size,
    /// independently from the render buffer resolution.
    void setPixelWorldSize(GLfloat aPixelSize);

    void setCameraTransformation(const math::AffineMatrix<3, GLfloat> & aTransformation);
    void setProjectionTransformation(const math::AffineMatrix<3, GLfloat> & aTransformation);


    static constexpr GLint gTextureUnit{0};

private:
    VertexSpecification mVertexSpecification;
    Program  mProgram;
    GLsizei mInstanceCount{0};
};


} // namespace graphics
} // namespace ad
