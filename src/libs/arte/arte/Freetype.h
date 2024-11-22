#pragma once

#include "ft2build.h"

#include "Logging.h"

#include <handy/Guard.h>
#include <math/Vector.h>
#include <platform/Filesystem.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H // For bbox measurements
#include FT_MODULE_H // For FT_Property_Set

#include <exception>
#include <iostream>

namespace ad {
namespace arte {


// The SDF spread used in FreeType sdf renderer.
// It means the distance, in texels from the edge (==128) to extreme values.
// see: https://freetype.org/freetype2/docs/reference/ft2-properties.html#spread
constexpr unsigned int gSdfSpread = 8;


#define GENERIC_LOGICERROR(fterror)                                            \
    std::logic_error                                                           \
    {                                                                          \
        "Error " + std::to_string(fterror) + " in " + __func__ + "("           \
            + std::to_string(__LINE__) + ")."                                  \
    }

using CharCode = std::uint32_t;

class FontFace;

/// \brief Wrap the FreeType library instance, which must exist while any
/// subobject is still alive.
class Freetype : public ResourceGuard<FT_Library>
{
public:
    Freetype() : ResourceGuard<FT_Library>{initialize(), &FT_Done_FreeType} {}

    FontFace load(const filesystem::path & aFontPath) const;

private:
    static FT_Library initialize()
    {
        FT_Library library;
        FT_Error error = FT_Init_FreeType(&library);
        if (error)
        {
            throw std::runtime_error{"Unable to initialize freetype, error: "
                                     + std::to_string(error)};
        }
        if(FT_Property_Set(library, "sdf", "spread", &gSdfSpread))
        {
            ADLOG(gMainLogger, error)("Unable to set the spread of FreeType sdf renderer.");
        }
        return library;
    }
};

/// \brief Wrap a font face.
/// \warning The library object must outlive the FontFace.
class FontFace : public ResourceGuard<FT_Face>
{
public:
    FontFace(const Freetype & aLibrary, const filesystem::path & aFontPath) :
        ResourceGuard<FT_Face>{initialize(aLibrary, aFontPath), &FT_Done_Face}
    {}

    const char * getName() const
    {
        return this->get()->family_name;
    }

    FontFace & inverseYAxis(bool aInverse)
    {
        static FT_Matrix inversion{1 << 16, 0, 0, -(1 << 16)};
        if (aInverse)
        {
            FT_Set_Transform(*this, &inversion, nullptr);
        }
        else
        {
            FT_Set_Transform(*this, nullptr, nullptr);
        }
        return *this;
    }

    /// \brief Define the pixel size of the GlyphBitmaps that will be obtained
    /// via `getGlyph()`.
    FontFace & setPixelSize(math::Size<2, int> aSize)
    {
        if (FT_Error error =
                FT_Set_Pixel_Sizes(*this, aSize.width(), aSize.height()))
        {
            throw GENERIC_LOGICERROR(error);
        }
        return *this;
    }

    FontFace & setPixelHeight(int aHeight)
    {
        return setPixelSize({0, aHeight});
    }

    math::Size<2, int> getPixelSize() const
    {
        // Note: this is likely not the real pixel size, but will be revisited
        // if the need arises. It returns the "Pixels per EM", which seems to be
        // set by setPixelSize() .
        return {get()->size->metrics.x_ppem, get()->size->metrics.y_ppem};
    }

    bool hasGlyph(CharCode aCharcode) const
    {
        // Explicit conversion to check it cannot lose precision.
        return FT_Get_Char_Index(*this, FT_ULong{aCharcode}) != 0;
    }

    FT_Bitmap renderGlyphSlot(FT_Render_Mode renderFlags) const
    {
        if (FT_Error error =
                FT_Render_Glyph(get()->glyph, renderFlags))
        {
            throw GENERIC_LOGICERROR(error);
        }
        return get()->glyph->bitmap;
    }

    FT_GlyphSlot loadChar(FT_ULong aCharcode, int freetypeFlags) const
    {
        if (FT_Error error =
                FT_Load_Char(*this, aCharcode, freetypeFlags))
        {
            throw GENERIC_LOGICERROR(error);
        }

        return get()->glyph;
    }

    math::Vec<2, float> kern(FT_UInt aLeftGlyphIndex,
                             FT_UInt aRightGlyphIndex) const
    {
        FT_Vector kerning;
        FT_Get_Kerning(*this, aLeftGlyphIndex, aRightGlyphIndex,
                       FT_KERNING_DEFAULT, &kerning);
        return {(float) kerning.x / (1 << 6), (float) kerning.y / (1 << 6)};
    }

private:
    static FT_Face initialize(FT_Library aLibrary,
                              const filesystem::path & aFontPath)
    {
        FT_Face face;
        FT_Error error =
            FT_New_Face(aLibrary, aFontPath.string().c_str(), 0, &face);
        if (error == FT_Err_Unknown_File_Format)
        {
            throw std::invalid_argument{
                "Font file format appears not to be supported: "
                + aFontPath.string()};
        }
        else if (error)
        {
            throw std::logic_error{"Error while accessing the font file: "
                                   + aFontPath.string()};
        }
        if (face->charmap == NULL)
        {
            throw std::logic_error{
                "Loaded font face does not have a default charmap selected."};
        }
        return face;
    }
};

//
// Implementations
//
inline FontFace Freetype::load(const filesystem::path & aFontPath) const
{
    return FontFace{*this, aFontPath};
}

#undef GENERIC_LOGICERROR

} // namespace arte
} // namespace ad
