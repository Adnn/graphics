#pragma once


#include <arte/Freetype.h>

#include <renderer/Texture.h>

#include <math/Vector.h>

#include <functional>
#include <optional>

// TODO move
#include <utf8.h> // utfcpp lib

namespace ad {
namespace graphics {



constexpr math::Vec<2, GLint> gRecommendedGlyphAtlasMargins{1, 1};

constexpr GLenum gGlyphAtlasTarget = GL_TEXTURE_RECTANGLE;


/// @brief  POD type used as argument to the GlyphCallback argument of `makeTightGlyphAtlas()`.
struct RenderedGlyph
{
    // TODO Storing a naked texture pointer is not ideal
    // Note: the texture is stored here for the cases where several textures are used for a single logical font atlas (dynamic)
    // Ideally, this association should be handled by the client, but it would mean 1 GlyphMap / texture (complicating lookups).
    Texture * texture;
    GLuint offsetInTexture; // The texture is a "1D" strip, only horizontal position. This is the position where the left margin starts.
    math::Size<2, GLfloat> controlBoxSize; // Including added margin if any
    math::Vec<2, GLfloat> bearing; // Including the added margin if any
    math::Vec<2, GLfloat> penAdvance;
    unsigned int freetypeIndex; // Notably usefull for kerning queries.
};


using GlyphCallback = std::function<void(arte::CharCode, const RenderedGlyph &)>;


/// @brief Returns a texture atlas of the fontface characters in range [aFirst, aLast[.
/// @return The texture Atlas (currently, a ribon).
Texture makeTightGlyphAtlas(const arte::FontFace & aFontFace,
                            arte::CharCode aFirst,
                            arte::CharCode aLast,
                            const GlyphCallback & aGlyphCallback,
                            math::Vec<2, GLint> aMargins = gRecommendedGlyphAtlasMargins);


/// @brief Helper class to keep track of the current pen position while iterating the glyphs of a string.
class PenPosition
{
public:
    PenPosition(math::Position<2, GLfloat> aInitialPosition = {0.f, 0.f}) :
        mLocalPenPosition{aInitialPosition}
    {}
    
    /// @return The pen position for the current glyph, computed from all previous glyphs.
    math::Position<2, GLfloat> advance(math::Vec<2, float> aPenAdvance,
                                       unsigned int aFreetypeIndex,
                                       const arte::FontFace & aFontFace);

    /// @return The pen position for the current glyph, computed from all previous glyphs.
    math::Position<2, GLfloat> advance(const graphics::RenderedGlyph & aGlyph,
                                       const arte::FontFace & aFontFace)
    { return advance(aGlyph.penAdvance, aGlyph.freetypeIndex, aFontFace); }

private:
    std::optional<unsigned int> mPreviousFreetypeIndex;
    math::Position<2, GLfloat> mLocalPenPosition;
};


using GlyphPositionCallback = 
    std::function<void(const RenderedGlyph &, math::Position<2, GLfloat>)>;

// TODO move to an impl file if kept around, due to utf8 import.
// TODO a CodePoint iterator would be much cleaner.
template <class T_glyphMap>
void forEachGlyph(const std::string & aString,
                  const arte::FontFace & aFontFace,
                  const T_glyphMap & aGlyphMap,
                  const GlyphPositionCallback & aGlyphCallback,
                  math::Position<2, GLfloat> aPenOrigin_p = {0.f, 0.f})
{
    unsigned int previousIndex = 0;
    for (std::string::const_iterator it = aString.begin();
         it != aString.end();
         /* in body */)
    {
        // Decode utf8 encoded string to individual Unicode code points
        arte::CharCode codePoint = utf8::next(it, aString.end());
        const RenderedGlyph & rendered = aGlyphMap.at(codePoint);

        // Kerning
        if (previousIndex != 0)
        {
            aPenOrigin_p +=
                aFontFace.kern(previousIndex, rendered.freetypeIndex);
        }
        previousIndex = rendered.freetypeIndex;

        aGlyphCallback(rendered, aPenOrigin_p);
        aPenOrigin_p += rendered.penAdvance;
    }
}


// TODO Provide dimension in the two dimensions.
//      Only provide the dimension in the direction of writing at the moment (the other will be zero).
template <class T_glyphMap>
math::Size<2, GLfloat> getStringDimension(const std::string & aString,
                                          const T_glyphMap & aGlyphMap,
                                          const arte::FontFace & aFontFace)
{
    math::Size<2, GLfloat> result{
        static_cast<math::Size<2, GLfloat>>(aFontFace.getPixelSize())
    };

    forEachGlyph(
        aString, 
        aFontFace,
        aGlyphMap,
        [&result](const auto & rendered, auto position) 
        {
            result = (position + rendered.penAdvance).template as<math::Size>();
        });

    return result;
}


} // namespace graphics
} // namespace ad
