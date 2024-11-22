#pragma once

#include "commons.h"
#include "gl_helpers.h"
#include "GL_Loader.h"
#include "MappedGL.h"
#include "ScopeGuards.h"

#include <arte/Image.h>

#include <handy/Guard.h>

#include <cassert>
#include <string>


namespace ad {
namespace graphics {


inline Guard scopePixelStorageMode(GLenum aParameter, GLint aValue)
{
    GLint previousValue;
    glGetIntegerv(aParameter, &previousValue);
    glPixelStorei(aParameter, aValue);
    return Guard{ [aParameter, previousValue](){glPixelStorei(aParameter, previousValue);} };
}

/// \brief Set the alignment of subsequent texture unpack (write to texture) operation to `aAlignment`,
/// then restore the previous alignment on returned guard destruction.
inline Guard scopeUnpackAlignment(GLint aAlignment)
{
    return scopePixelStorageMode(GL_UNPACK_ALIGNMENT, aAlignment);
}


/// \brief Set the alignment of subsequent texture pack (read from texture into client memory)
/// operation to `aAlignment`, then restore the previous alignment on returned guard destruction.
inline Guard scopePackAlignment(GLint aAlignment)
{
    return scopePixelStorageMode(GL_PACK_ALIGNMENT, aAlignment);
}


// Note: alternatively to hosting the target in a data member (and specializing Name for Texture),
//       we could do like for Buffers and have the target as a template non-type parameter.
//       Yet, this would imply that all functions taking a texture have to be templated too.
struct [[nodiscard]] Texture : public ResourceGuard<GLuint>
{
    struct NullTag{};

    /// \parameter aTarget for values see `target` parameter of
    /// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindTexture.xhtml
    Texture(GLenum aTarget) :
        ResourceGuard<GLuint>{reserve(glGenTextures),
                              [](GLuint textureId){glDeleteTextures(1, &textureId);}},
        mTarget(aTarget)
    {}

    Texture(GLenum aTarget, NullTag) :
        ResourceGuard<GLuint>{0,
                              [](GLuint textureId){}},
        mTarget(aTarget)
    {}

    GLenum mTarget;
};


template <>
class Name<Texture> : public detail::NameBase<Texture>
{
public:
    explicit Name(GLuint aResource, GLenum aTarget, UnsafeTag) :
        NameBase{aResource, UnsafeTag{}},
        mTarget{aTarget}
    {}

    /*implicit*/ Name(const Texture & aTexture) :
        NameBase{aTexture},
        mTarget{aTexture.mTarget}
    {}

    GLenum mTarget;
};


inline void activateTextureUnit(GLint aTextureUnit)
{
    glActiveTexture(GL_TEXTURE0 + aTextureUnit);
}

/// \deprecated Use the scoped version instead
inline Guard activateTextureUnitGuard(GLint aTextureUnit)
{
    activateTextureUnit(aTextureUnit);
    return Guard([]()
        {
            glActiveTexture(GL_TEXTURE0);
        });
}


inline Guard scopeTextureUnitActivation(GLint aTextureUnit)
{
    GLint previousActive;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActive);

    activateTextureUnit(aTextureUnit);
    return Guard([previousActive]()
        {
            glActiveTexture(previousActive);
        });
}


/// \brief Bind the texture to the currently active texture unit.
inline void bind(const Texture & aTexture)
{
    glBindTexture(aTexture.mTarget, aTexture);
}


inline void bind(Name<Texture> aTexture)
{
    glBindTexture(aTexture.mTarget, aTexture);
}


inline void unbind(const Texture & aTexture)
{
    glBindTexture(aTexture.mTarget, 0);
}


inline Name<Texture> getBound(const Texture & aTexture)
{
    GLint current;
    glGetIntegerv(getGLMappedTextureBinding(aTexture.mTarget), &current);
    return Name<Texture>{(GLuint)current, aTexture.mTarget, Name<Texture>::UnsafeTag{}};
}


/// \deprecated Texture unit activation is probably better explicit and outside.
inline void bind(const Texture & aTexture, GLenum aTextureUnit)
{
    glActiveTexture(aTextureUnit);
    bind(aTexture);
}


/// \deprecated Texture unit activation is probably better explicit and outside.
inline void unbind(const Texture & aTexture, GLenum aTextureUnit)
{
    glActiveTexture(aTextureUnit);
    unbind(aTexture);
}


inline void setFiltering(const Texture & aTexture, GLenum aFiltering)
{
    ScopedBind scoped{aTexture};
    glTexParameteri(aTexture.mTarget, GL_TEXTURE_MIN_FILTER, aFiltering);
    glTexParameteri(aTexture.mTarget, GL_TEXTURE_MAG_FILTER, aFiltering);
}


inline GLsizei countCompleteMipmaps(math::Size<2, int> aResolution)
{
    return static_cast<GLsizei>(std::floor(
        std::log2(
            std::max(aResolution.width(), aResolution.height())
    ))) + 1;
}


inline constexpr math::Size<2, GLsizei> getMipmapSize(math::Size<2, int> aFullResolution, unsigned int aLevel)
{
    return {
        std::max(1, aFullResolution.width() / (int)std::pow(2, aLevel)),
        std::max(1, aFullResolution.height() / (int)std::pow(2, aLevel)),
    };
}


/// @brief Returns true if the texture was successfully allocated with an immutable format.
inline bool isImmutableFormat(const Texture & aBoundTexture)
{
    GLint isSuccess;
    glGetTexParameteriv(aBoundTexture.mTarget, GL_TEXTURE_IMMUTABLE_FORMAT, &isSuccess);
    return isSuccess == GL_TRUE;
}


/// \brief Allocate texture storage.
inline void allocateStorage(const Texture & aTexture, const GLenum aInternalFormat,
                            const GLsizei aWidth, const GLsizei aHeight,
                            const GLsizei aMipmapLevelsCount = 1)
{
    ScopedBind bound(aTexture);

    if (GL_ARB_texture_storage)
    {
        glTexStorage2D(aTexture.mTarget, aMipmapLevelsCount, aInternalFormat, aWidth, aHeight);
        if(!isImmutableFormat(aTexture))
        {
            const std::string message{"Error calling 'glTexStorage2D'"};
            std::cerr << message << std::endl;
            throw std::runtime_error(message);
        }
    }
    else
    {
        throw std::runtime_error("Not implemented" + std::to_string(__LINE__));
    }
}


inline void allocateStorage(const Texture & aTexture, const GLenum aInternalFormat,
                            math::Size<2, GLsizei> aResolution,
                            const GLsizei aMipmapLevelsCount = 1)
{
    return allocateStorage(aTexture, aInternalFormat,
                           aResolution.width(), aResolution.height(),
                           aMipmapLevelsCount);
}


void clear(const Texture & aTexture, math::hdr::Rgba_f aClearValue);


} // namespace graphics
} // namespace ad
