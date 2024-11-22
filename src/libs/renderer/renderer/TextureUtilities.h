#pragma once


#include "Texture.h"

#include "MappedGL.h"

namespace ad::graphics {


template <class T_Pixel>
void serializeTexture(const graphics::Texture & aTexture,
                      GLint aLevel,
                      arte::ImageFormat aFormat,
                      std::ostream & aOut)
{
    graphics::ScopedBind boundTexture{aTexture};

    math::Size<2, GLint> size;
    glGetTexLevelParameteriv(aTexture.mTarget,
                             aLevel,
                             GL_TEXTURE_WIDTH,
                             &size.width());
    glGetTexLevelParameteriv(aTexture.mTarget,
                             aLevel,
                             GL_TEXTURE_HEIGHT,
                             &size.height());

    // TODO: retrieve the texture internal format, and assert T_Pixel compatibility
    //GLenum internalFormat;
    //glGetTexLevelParameteriv(aTexture.mTarget,
    //                         aLevel,
    //                         GL_TEXTURE_INTERNAL_FORMAT,
    //                         static_cast<GLint *>(&internalFormat));

    // Note: All image format we can write to accept 1-byte alignment for rows,
    // and STBI_writer only allow to control the stride for PNG.
    // Default OpenGL value is 4-bytes alignment for row start, which can be problematic
    // for < 4 components image with a width that is not a multiple of 4.
    // The easy solution is to always require 1-byte alignment 
    // (even when it gives the same results than 4-bytes alignment)
    auto packAlignmentGuard = scopePackAlignment(1);

    std::unique_ptr<unsigned char[]> raster = 
        std::make_unique<unsigned char[]>(sizeof(T_Pixel) * size.area());

    glGetTexImage(aTexture.mTarget,
                  aLevel,
                  graphics::MappedPixel_v<T_Pixel>, 
                  graphics::MappedPixelComponentType_v<T_Pixel>,
                  raster.get());

    arte::Image<T_Pixel> result{size, std::move(raster)};
    result.write(aFormat, aOut);
}


/// \brief Parameter for `writeTo()` below.
struct InputImageParameters
{
    template <class T_pixel>
    static InputImageParameters From(const arte::Image<T_pixel> & aImage);

    math::Size<2, GLsizei> resolution;
    GLenum format;
    GLenum type;
    GLint alignment; // maps to GL_UNPACK_ALIGNMENT
};


template <class T_pixel>
InputImageParameters InputImageParameters::From(const arte::Image<T_pixel> & aImage)
{
    return {
        aImage.dimensions(),
        MappedPixel_v<T_pixel>,
        MappedPixelComponentType_v<T_pixel>,
        (GLint)aImage.rowAlignment()
    };
}


/// \brief OpenGL pixel unpack operation, writing to a texture whose storage is already allocated.
inline void writeTo(const Texture & aTexture,
                    const std::byte * aRawData,
                    const InputImageParameters & aInput,
                    math::Position<2, GLint> aTextureOffset = {0, 0},
                    GLint aMipmapLevelId = 0)
{
    // Cubemaps individual faces muste be accessed explicitly in glTexSubImage2D.
    assert(aTexture.mTarget != GL_TEXTURE_CUBE_MAP);

    // TODO assert that texture target is of correct dimension.

    // TODO replace with DSA
    ScopedBind bound(aTexture);

    // Handle alignment
    Guard scopedAlignemnt = scopeUnpackAlignment(aInput.alignment);

    glTexSubImage2D(aTexture.mTarget, aMipmapLevelId,
                    aTextureOffset.x(), aTextureOffset.y(),
                    aInput.resolution.width(), aInput.resolution.height(),
                    aInput.format, aInput.type,
                    aRawData);
}


inline void writeTo(const Texture & aTexture,
                    const std::byte * aRawData,
                    const InputImageParameters & aInput,
                    math::Position<3, GLint> aTextureOffset,
                    GLint aMipmapLevelId = 0)
{
    // TODO assert that texture target is of correct dimension.

    // TODO replace with DSA
    ScopedBind bound(aTexture);

    // Handle alignment
    Guard scopedAlignemnt = scopeUnpackAlignment(aInput.alignment);

    glTexSubImage3D(aTexture.mTarget, aMipmapLevelId,
                    aTextureOffset.x(), aTextureOffset.y(), aTextureOffset.z(),
                    aInput.resolution.width(), aInput.resolution.height(), 1,
                    aInput.format, aInput.type,
                    aRawData);
}


/// \brief Allocate storage and read `aImage` into `aTexture`.
/// \note The number of mipmap levels allocated for the texture can be specified,
/// but the provided image is always written to mipmal level #0.
template <class T_pixel>
void loadImage(const Texture & aTexture,
               const arte::Image<T_pixel> & aImage,
               GLint aMipmapLevelsCount = 1)
{
    // Probably too restrictive
    assert(aTexture.mTarget == GL_TEXTURE_2D
        || aTexture.mTarget == GL_TEXTURE_RECTANGLE);

    allocateStorage(
        aTexture,
        MappedSizedPixel_v<T_pixel>,
        aImage.dimensions(),
        aMipmapLevelsCount);
    writeTo(
        aTexture, 
        static_cast<const std::byte *>(aImage),
        InputImageParameters::From(aImage));
}

template <class T_pixel>
void loadImageCompleteMipmaps(const Texture & aTexture,
                              const arte::Image<T_pixel> & aImage)
{
    loadImage(aTexture, aImage, countCompleteMipmaps(aImage.dimensions()));
    ScopedBind bound(aTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
}

/// \brief Load an animation from an image containing a (column) array of frames.
template <class T_pixel>
inline void loadAnimationAsArray(const Texture & aTexture,
                                 const arte::Image<T_pixel> & aImage,
                                 const Size2<int> & aFrame,
                                 size_t aSteps)
{
    // Implementers note:
    // This implemention is kept with the "old-approach" to illustrate
    // how it can be done pre GL_ARB_texture_storage

    assert(aTexture.mTarget == GL_TEXTURE_2D_ARRAY);

    ScopedBind bound(aTexture);

    Guard scopedAlignemnt = scopeUnpackAlignment(aImage.rowAlignment());

    glTexImage3D(aTexture.mTarget, 0, GL_RGBA,
                 aFrame.width(), aFrame.height(), static_cast<GLsizei>(aSteps),
                 0, MappedPixel_v<T_pixel>, GL_UNSIGNED_BYTE, static_cast<const unsigned char *>(aImage));

    // Texture parameters
    glTexParameteri(aTexture.mTarget, GL_TEXTURE_MAX_LEVEL, 0);
    // Sampler parameters
    glTexParameteri(aTexture.mTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(aTexture.mTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}


} // namespace ad::graphics