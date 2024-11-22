
#include "TextUtilities.h"

#include <renderer/TextureUtilities.h>


namespace ad {
namespace graphics {


namespace {


    /// @brief Converts the fixed-point value `aPos` to floating point.
    inline GLfloat fixedToFloat(FT_Pos aPos, int aFixedDecimals = 6)
    {
        return (GLfloat)(aPos >> aFixedDecimals);
    }


    /// @brief Unidimensionnal array of rasters.
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

        GLint isFitting(GLint aCandidateWidth)
        { return aCandidateWidth <= (width - nextXOffset); }

        /// @brief Write the raw bitmap `aData` to the ribon.
        /// @return The horizontal offset to the written data.
        GLint write(const std::byte * aData, InputImageParameters aInputParameters);

        Texture texture;
        GLint width{0};
        math::Vec<2, GLint> margins;
        GLint nextXOffset{0}; // The left margin before the first glyph will be added by  write()
    };


    // Note: Linear offers smoother translations, at the cost of sharpness.
    // Note: Nearest currently has a drawback that all letters of a string do not necessarily advance a pixel together.
    inline TextureRibon make_TextureRibon(math::Size<2, GLint> aDimensions, GLenum aInternalFormat, math::Vec<2, GLint> aMargins, GLenum aTextureFiltering)
    {
        TextureRibon ribon{Texture{gGlyphAtlasTarget}, aDimensions.width(), aMargins};
        allocateStorage(ribon.texture, aInternalFormat, aDimensions.width(), aDimensions.height());
        // Note: Only the first (red) value will be used for a GL_R8 texture, but the API requires a 4-channel color.
        clear(ribon.texture, {math::hdr::gBlack<GLfloat>, 0.f});

        bind(ribon.texture);
        glTexParameteri(gGlyphAtlasTarget, GL_TEXTURE_MIN_FILTER, aTextureFiltering);
        glTexParameteri(gGlyphAtlasTarget, GL_TEXTURE_MAG_FILTER, aTextureFiltering);
        glTexParameteri(gGlyphAtlasTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(gGlyphAtlasTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);

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


} // anonymous namespace



Texture makeTightGlyphAtlas(const arte::FontFace & aFontFace,
                            arte::CharCode aFirst,
                            arte::CharCode aLast,
                            const GlyphCallback & aGlyphCallback,
                            math::Vec<2, GLint> aMargins)
{
    std::vector<arte::CharCode> glyphs;

    //
    // Compute the atlas dimension
    //
    math::Size<2, GLint> atlasDimensions{0, 0};
    for (; aFirst != aLast; ++aFirst)
    {
        if (aFontFace.hasGlyph(aFirst))
        {
            FT_GlyphSlot slot = aFontFace.loadChar(
                aFirst, FT_LOAD_RENDER | FT_LOAD_TARGET_(FT_RENDER_MODE_SDF));
            // Note: The glyph metrics available in FT_GlyphSlot are not
            // available in FT_Glyph so we store them now... see:
            // https://lists.gnu.org/archive/html/freetype/2010-09/msg00036.html
            glyphs.push_back(aFirst);
            math::Size<2, int> glyphSize{
                static_cast<int>(slot->bitmap.width) + 2 * aMargins.x(),
                static_cast<int>(slot->bitmap.rows) + 2 * aMargins.y()};
            atlasDimensions.width() += glyphSize.width();
            atlasDimensions.height() =
                std::max(atlasDimensions.height(), glyphSize.height());
        }
    }

    //
    // Fill in the atlas
    //
    TextureRibon ribon =
        make_TextureRibon(atlasDimensions, GL_R8, aMargins, GL_LINEAR);
    for (auto & charcode : glyphs)
    {
        FT_GlyphSlot slot = aFontFace.loadChar(
            charcode, FT_LOAD_RENDER | FT_LOAD_TARGET_(FT_RENDER_MODE_SDF));
        FT_Bitmap bitmap = slot->bitmap;

        assert(ribon.isFitting(bitmap.width));

        InputImageParameters inputParams{
            {static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows)},
            GL_RED,
            GL_UNSIGNED_BYTE,
            1};
        GLuint horizontalOffset =
            ribon.write( reinterpret_cast<const std::byte *>(bitmap.buffer), inputParams);

        RenderedGlyph rendered{
            // TODO Ad 2024/11/15: #UB THIS IS PLAIN WRONG
            // The ribon is local to the function, and its texture is allocated on the stack frame
            .texture = &ribon.texture,
            .offsetInTexture = horizontalOffset,
            // See DynamicGlyphCache::at() for the rationale
            // behind the addition
            .controlBoxSize = {
                static_cast<float>(slot->bitmap.width) + 2 * aMargins.x(),
                static_cast<float>(slot->bitmap.rows) + 2 * aMargins.y()
            },
            .bearing = {
                static_cast<float>(slot->bitmap_left) - 2 * aMargins.x(),
                static_cast<float>(slot->bitmap.rows - slot->bitmap_top) + aMargins.y()
            },
            .penAdvance = {
                fixedToFloat(slot->metrics.horiAdvance),
                0.f /* hardcoded horizontal layout */
            },
            .freetypeIndex = slot->glyph_index
        };

        aGlyphCallback(charcode, rendered);
    }

    return std::move(ribon.texture);
}


math::Position<2, GLfloat> PenPosition::advance(math::Vec<2, float> aPenAdvance,
                                                unsigned int aFreetypeIndex,
                                                const arte::FontFace & aFontFace)
{
    if(mPreviousFreetypeIndex)
    {
        mLocalPenPosition += aFontFace.kern(*mPreviousFreetypeIndex, aFreetypeIndex);
    }
    math::Position<2, GLfloat> result = mLocalPenPosition;
    mPreviousFreetypeIndex = aFreetypeIndex;
    mLocalPenPosition += aPenAdvance;
    return result;
}


} // namespace graphics
} // namespace ad
