#pragma once


#include <arte/Freetype.h>

#include <math/Vector.h>

#include <renderer/Texture.h>
#include <renderer/TextureUtilities.h>

#include <glad/glad.h>

#include <list>
#include <unordered_map>


namespace ad {
namespace graphics {
namespace detail {


/// Unidimensionnal array of rasters
struct TextureRibon
{
    /// \param aMargins The empty margin on each side of the glyph.
    /// This is the margin that will be left empty on the left and below each glyph when copying
    /// their bitmap to the texture.
    /// In particular, this makes no guarantee about the margin on top, which
    /// is implicitly texture_height - glyph_height - margin_y.
    TextureRibon(Texture aTexture, GLint aWidth, math::Vec<2, GLint> aMargins) :
        texture{std::move(aTexture)},
        width{aWidth},
        margins{aMargins}
    {}

    Texture texture;
    GLint width{0};
    math::Vec<2, GLint> margins;
    GLint nextXOffset{0}; // The left margin before the first glyph will be added by  write()

    static constexpr math::Vec<2, GLint> gRecommendedMargins{1, 1};

    GLint isFitting(GLint aCandidateWidth)
    { return aCandidateWidth <= (width - nextXOffset); }

    /// @brief Write the raw bitmap `aData` to the ribon.
    /// @return The horizontal offset to the written data.
    GLint write(const std::byte * aData, InputImageParameters aInputParameters);
};


// Note: Linear offers smoother translations, at the cost of sharpness.
// Note: Nearest currently has a drawback that all letters of a string do not necessarily advance a pixel together.
inline TextureRibon make_TextureRibon(math::Size<2, GLint> aDimensions, GLenum aInternalFormat, math::Vec<2, GLint> aMargins, GLenum aTextureFiltering)
{
    TextureRibon ribon{Texture{GL_TEXTURE_RECTANGLE}, aDimensions.width(), aMargins};
    allocateStorage(ribon.texture, aInternalFormat, aDimensions.width(), aDimensions.height());
    // Note: Only the first (red) value will be used for a GL_R8 texture, but the API requires a 4-channel color.
    clear(ribon.texture, {math::hdr::gBlack<GLfloat>, 0.f});

    bind(ribon.texture);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, aTextureFiltering);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, aTextureFiltering);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);

    return ribon;
}


inline GLint TextureRibon::write(const std::byte * aData, InputImageParameters aInputParameters)
{
    // Start writing after the left margin.
    writeTo(texture, aData, aInputParameters, math::Position<2, GLint>{margins.x() + nextXOffset, margins.y()});
    // The pointed-to bitmap includes the left margin (i.e. the offset does not get the margin added).
    // (which is consistent with controlBoxSize including the margin)
    GLint thisOffset = nextXOffset; 
    // The next glyph can start after the current glyph width plus the margins on both sides.
    nextXOffset += margins.x() + aInputParameters.resolution.width() + margins.x();
    return thisOffset;
}


struct RenderedGlyph
{
    // TODO Storing a naked texture pointer is not ideal
    // Note: the texture is stored here for the cases where several textures are used for a single logical font atlas (dynamic)
    // Ideally, this association should be handled by the client, but it would mean 1 GlyphMap / texture (complicating lookups).
    Texture * texture;
    GLint offsetInTexture; // The texture is a "1D" strip, only horizontal position. This is the position where the left margin starts.
    math::Size<2, GLfloat> controlBoxSize; // Including added margin if any
    math::Vec<2, GLfloat> bearing; // Including the added margin if any
    math::Vec<2, GLfloat> penAdvance;
    unsigned int freetypeIndex; // Notably usefull for kerning queries.
};


using GlyphMap = std::unordered_map<arte::CharCode, RenderedGlyph>;


inline GLfloat fixedToFloat(FT_Pos aPos, int aFixedDecimals = 6)
{
    return (GLfloat)(aPos >> aFixedDecimals);
}


Texture makeTightGlyphAtlas(const arte::FontFace & aFontFace,
                            arte::CharCode aFirst, arte::CharCode aLast,
                            GlyphMap & aGlyphMap,
                            math::Vec<2, GLint> aMargins = TextureRibon::gRecommendedMargins);


struct StaticGlyphCache
{
    // The empty cache
    StaticGlyphCache() = default;

    StaticGlyphCache(const arte::FontFace & aFontFace,
                     arte::CharCode aFirst, arte::CharCode aLast,
                     math::Vec<2, GLint> aDimensionExtension = TextureRibon::gRecommendedMargins);

    RenderedGlyph at(arte::CharCode aCharCode) const;

    Texture atlas{0};
    GlyphMap glyphMap;
    static constexpr arte::CharCode placeholder = 0x3F; // '?'
};


struct DynamicGlyphCache
{
    // Since we are storing texture pointer, growing the atlas should not re-allocate!
    std::list<TextureRibon> atlases;
    GlyphMap glyphMap;
    GLenum textureFiltering = GL_LINEAR;
    math::Size<2, GLint> ribonDimension = {0, 0};
    math::Vec<2, GLint> margins = {0, 0};
    arte::CharCode placeholder = 0x3F; // '?'

    DynamicGlyphCache() = default;

    // The empty cache
    DynamicGlyphCache(math::Size<2, GLint> aRibonDimension_p, math::Vec<2, GLint> aMargins, GLenum aTextureFiltering);

    void growAtlas()
    {
        atlases.push_back(make_TextureRibon(ribonDimension, GL_R8, margins, textureFiltering));
    }

    RenderedGlyph at(arte::CharCode aCharCode, const arte::FontFace & aFontFace);

    void preloadGlyphs(const arte::FontFace & aFontFace, arte::CharCode aFirst, arte::CharCode aLast)
    {
        for(; aFirst != aLast; ++aFirst)
        {
            at(aFirst, aFontFace);
        }
    }
};


// TODO aPixelToLocal should be removed, when the Texting rendering does all "local layout" in pixel coordinates
/// \deprecated
void forEachGlyph(const std::string & aString,
                  math::Position<2, GLfloat> aPenOrigin_w,
                  DynamicGlyphCache & aGlyphCache,
                  arte::FontFace & aFontFace,
                  math::Size<2, GLfloat> aPixelToLocal,
                  std::function<void(RenderedGlyph, math::Position<2, GLfloat>)> aGlyphCallback);


/// @brief Prepare a string for rendering by invoking `aGlyphCallback` for each glyph in `aString`.
/// @param aPenOrigin_u Pen origin when starting to write the string, expressed in pixels of the render target.
void forEachGlyph(const std::string & aString,
                  math::Position<2, GLfloat> aPenOrigin_p,
                  const StaticGlyphCache & aGlyphCache,
                  const arte::FontFace & aFontFace,
                  std::function<void(const RenderedGlyph &, math::Position<2, GLfloat>)> aGlyphCallback);


// TODO only provide the dimension in the direction of writing at the moment (the other will be zero).
math::Size<2, GLfloat> getStringDimension(const std::string & aString,
                                          const StaticGlyphCache & aGlyphCache,
                                          const arte::FontFace & aFontFace);


} // namespace detail
} // namespace graphics
} // namespace ad
