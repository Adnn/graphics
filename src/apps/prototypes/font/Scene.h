#pragma once


#include <arte/Freetype.h>

#include "shaders.h"

#include <freetype/freetype.h>
#include <freetype/ftimage.h>
#include <graphics/AppInterface.h>
#include <graphics/CameraUtilities.h>
#include <graphics/Timer.h>
#include <graphics/shaders.h>

#include <graphics/detail/UnitQuad.h>

#include <renderer/Drawing.h>
#include <renderer/Texture.h>
#include <renderer/TextureUtilities.h>
#include <renderer/Uniforms.h>

#include <math/Homogeneous.h>
#include <math/Vector.h>

#include <GLFW/glfw3.h>


using namespace ad::graphics;

namespace ad {
namespace font {


using Vec2 = math::Vec<2, GLfloat>;
using Position2 = math::Position<2, GLfloat>;
using Size2 = math::Size<2, GLfloat>;

constexpr GLint gTextureUnit = 2;
constexpr FT_ULong gStartingCharcode = 0x20; // space 
constexpr int gGlyphPerLine = 0x7B /* { */ - gStartingCharcode;
constexpr int gHorizontalMargin = 1; // pixels


/// \brief A glyph info in the cache
struct Glyph
{
    GLint offsetInTexture; // The texture is a "1D" strip, only horizontal position.
    math::Size<2, GLfloat> boundingBox;
    Vec2 bearing;
    Vec2 penAdvance;
    FT_UInt ftIndex;
};


struct GlyphInstance
{
    GlyphInstance(Position2 aPosition_w, const Glyph & aGlyph) :
        position_w{std::move(aPosition_w)},
        offsetInTexture{aGlyph.offsetInTexture},
        boundingBox{aGlyph.boundingBox},
        bearing{aGlyph.bearing}
    {}

    Position2 position_w;
    GLint offsetInTexture;
    math::Size<2, GLfloat> boundingBox;
    math::Vec<2, GLfloat> bearing;
};


constexpr AttributeDescriptionList gGlyphInstanceDescription{
    { 2, 2, offsetof(GlyphInstance, position_w),  MappedGL<GLfloat>::enumerator},
    { {3, ShaderParameter::Access::Integer}, 1, offsetof(GlyphInstance, offsetInTexture), MappedGL<GLint>::enumerator},
    { 4, 2, offsetof(GlyphInstance, boundingBox), MappedGL<GLfloat>::enumerator},
    { 5, 2, offsetof(GlyphInstance, bearing),     MappedGL<GLfloat>::enumerator},
};


struct Scene
{
    Scene(const filesystem::path & aFontPath,
          int aGlyphPixelHeight,
          GLfloat aGlyphWorldHeight, 
          GLfloat aScreenWorldHeight,
          graphics::AppInterface & aAppInterface);

    void step(const Timer & aTimer);
    void render();

    void onKey(int key, int scancode, int action, int mods);

    Size2 mPixelToWorld;
    std::vector<Glyph> mGlyphCache;
    graphics::Texture mFontAtlas;
    graphics::Program mPassthrough;
    graphics::Program mFontProgram;
    arte::Freetype mFreetype;
    arte::FontFace mFontface;
    graphics::VertexSpecification mScreenQuadVertex{graphics::detail::make_UnitQuad()};
    graphics::VertexSpecification mGlyphQuadVertex{
        graphics::detail::make_Rectangle({ {0.f, -1.f}, {1.f, 1.f} })};
    GLsizei mGlyphCount{0};
    bool mShowAtlas{false};
    bool mKerning{true};
};


// Hardcoded for 26.6
GLfloat toFloat(FT_Pos aPos)
{
    return (GLfloat)aPos / (1 << 6);
}


inline Scene::Scene(const filesystem::path & aFontPath,
                    int aGlyphPixelHeight,
                    GLfloat aGlyphWorldHeight, 
                    GLfloat aScreenWorldHeight,
                    graphics::AppInterface & aAppInterface) :
    mPixelToWorld{aGlyphWorldHeight / static_cast<float>(aGlyphPixelHeight), aGlyphWorldHeight / static_cast<float>(aGlyphPixelHeight)},
    mFontAtlas{GL_TEXTURE_RECTANGLE},
    mPassthrough{ makeLinkedProgram({
        {GL_VERTEX_SHADER, graphics::gPassthroughVertexShader},
        {GL_FRAGMENT_SHADER, gFontFragmentShader}
    })},
    mFontProgram{ makeLinkedProgram({
        {GL_VERTEX_SHADER,   gFontVertexShader},
        {GL_FRAGMENT_SHADER, gFontFragmentShader}
    })},
    mFontface{mFreetype.load(aFontPath)}
{
    // Keyboard callback
    using namespace std::placeholders;
    aAppInterface.registerKeyCallback(std::bind(&Scene::onKey, this, _1, _2, _3, _4));

    // Glyph instance buffer
    mGlyphQuadVertex.mVertexBuffers.push_back(
        initVertexBuffer<GlyphInstance>(mGlyphQuadVertex.mVertexArray, gGlyphInstanceDescription, 1));

    // Screen program
    setUniform(mPassthrough, "inputTexture", gTextureUnit);

    // Font program
    setUniform(mFontProgram, "inputTexture", gTextureUnit);
    setUniform(mFontProgram, "u_PixelToWorld", mPixelToWorld);
    setUniform(mFontProgram, "u_WorldToCamera", math::AffineMatrix<4, GLfloat>::Identity());
    setUniform(mFontProgram, "u_Projection", 
        math::trans3d::orthographicProjection<GLfloat>(
            getViewVolumeRightHanded(aAppInterface.getFramebufferSize(), aScreenWorldHeight, 0, 1)));

    
    // Font atlas generation
    auto startingCharcode = gStartingCharcode;
    GLsizei textureWidth = gGlyphPerLine * (aGlyphPixelHeight + gHorizontalMargin);
    // Add 1 in height, for example character 253 '�' from DejaVuSans
    // has 129 rows when the pixel height is set to 128...
    allocateStorage(mFontAtlas, GL_R8, textureWidth, aGlyphPixelHeight + 1);
    bind(mFontAtlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    mFontface.inverseYAxis(true);
    mFontface.setPixelHeight(aGlyphPixelHeight);

    GLint textureX = 0;
    while(true)
    {
        auto charcode = startingCharcode++;
        if (mFontface.hasGlyph(charcode))
        {
            FT_GlyphSlot slot = mFontface.loadChar(charcode, FT_LOAD_RENDER | FT_LOAD_TARGET_(FT_RENDER_MODE_SDF));
            FT_Bitmap bitmap = slot->bitmap;
            InputImageParameters inputParams{
                {static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows)},
                GL_RED,
                GL_UNSIGNED_BYTE,
                1
            };

            if ( (textureX + bitmap.width + gHorizontalMargin) > textureWidth )
            {
                break;
            }
            writeTo(mFontAtlas, reinterpret_cast<std::byte *>(bitmap.buffer), inputParams, {textureX, 0});

            mGlyphCache.push_back({
                textureX,
                {toFloat(slot->bitmap.width), toFloat(slot->bitmap.rows)},
                {toFloat(slot->bitmap_left), toFloat(slot->bitmap.rows - slot->bitmap_top)},
                {toFloat(slot->metrics.horiAdvance), 0.f /* hardcoded horizontal layout */},
                slot->glyph_index,
            });

            textureX += static_cast<int>(bitmap.width) + gHorizontalMargin;
        }
    }

    setUniform(mPassthrough, "u_UVScaling", Size2{(GLfloat)textureX, (GLfloat)aGlyphPixelHeight});
}


void Scene::onKey(int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
    {
        mShowAtlas = !mShowAtlas;
    }
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        mKerning = !mKerning;
    }
}


inline void Scene::step(const Timer & aTimer)
{
    std::vector<GlyphInstance> instances;
    auto addMessage = [&](const std::string message, Position2 penPosition)
    {
        FT_UInt previousIndex = 0;
        for (char c : message)
        {
            std::size_t cacheIndex = (std::size_t)c - gStartingCharcode;

            if (mKerning && (previousIndex != 0))
            {
                Vec2 kerning = mFontface.kern(previousIndex, mGlyphCache[cacheIndex].ftIndex);
                penPosition += kerning.cwMul(mPixelToWorld.as<math::Vec>());
            }
            previousIndex = mGlyphCache[cacheIndex].ftIndex;

            instances.emplace_back(penPosition, mGlyphCache[cacheIndex]);
            penPosition += mGlyphCache[cacheIndex].penAdvance.cwMul(mPixelToWorld.as<math::Vec>());
        }
    };

    Position2 penPosition{-26.f, 0.f};
    addMessage({"Salut, monde."}, penPosition);

    penPosition = {-26.f, -10.f};
    addMessage({"WAV!"}, penPosition);

    respecifyBuffer(mGlyphQuadVertex.mVertexBuffers.back(),
                    std::span{instances},
                    BufferHint::DynamicDraw);
    mGlyphCount = static_cast<int>(instances.size());
}


inline void Scene::render()
{
    glActiveTexture(GL_TEXTURE0 + gTextureUnit);
    bind(mFontAtlas);

    if (mShowAtlas)
    {
        activate(mScreenQuadVertex, mPassthrough);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    else
    {
        activate(mGlyphQuadVertex, mFontProgram);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mGlyphCount);
    }
}


} // namespace font
} // namespace ad
