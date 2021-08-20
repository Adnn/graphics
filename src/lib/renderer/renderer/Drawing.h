#pragma once

#include "Shading.h"
#include "Texture.h"
#include "VertexSpecification.h"

namespace ad {

struct [[nodiscard]] DrawContext
{
    DrawContext(VertexSpecification aVertexSpecification,
                Program aProgram,
                std::vector<Texture> aTextures={}) :
        mVertexSpecification{std::move(aVertexSpecification)},
        mProgram{std::move(aProgram)},
        mTextures{std::move(aTextures)}
    {}

    VertexSpecification mVertexSpecification;
    Program  mProgram;
    std::vector<Texture>  mTextures;
};

inline DrawContext makeBareContext()
{
    return DrawContext{VertexSpecification{}, Program{}};
}

inline void bindVertexArray(const DrawContext & aDrawContext)
{
    glBindVertexArray(aDrawContext.mVertexSpecification.mVertexArray);
}

inline void useProgram(const DrawContext & aDrawContext)
{
    glUseProgram(aDrawContext.mProgram);
}

inline void activate(const DrawContext & aDrawContext)
{
    bindVertexArray(aDrawContext);
    useProgram(aDrawContext);
}

inline std::vector<VertexBufferObject> & buffers(DrawContext & aDrawContext)
{
    return aDrawContext.mVertexSpecification.mVertexBuffers;
}


template <class T_index>
inline Guard scopePrimitiveRestartIndex(T_index aIndex)
{
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(aIndex);
    return Guard{ [](){glDisable(GL_PRIMITIVE_RESTART);} };
}


} // namespace ad
