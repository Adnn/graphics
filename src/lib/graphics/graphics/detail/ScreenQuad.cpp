#include "ScreenQuad.h"

#include "../shaders.h"


namespace ad {
namespace graphics {
namespace detail {


VertexSpecification make_ScreenQuad()
{
    std::array<VertexScreenQuad, 4> gVerticesScreen = {
        VertexScreenQuad{
            {-1.0f, -1.0f},
            {0.0f, 0.0f},
        },
        VertexScreenQuad{
            {-1.0f,  1.0f},
            {0.0f,  1.0f},
        },
        VertexScreenQuad{
            { 1.0f, -1.0f},
            { 1.0f, 0.0f},
        },
        VertexScreenQuad{
            { 1.0f,  1.0f},
            { 1.0f,  1.0f},
        },
    };

    VertexSpecification result;
    appendToVertexSpecification(result, gVertexScreenDescription, gsl::make_span(gVerticesScreen));
    return result;
}


Program make_PassthroughProgram()
{
    Program passthrough = makeLinkedProgram({
        {GL_VERTEX_SHADER, gPassthroughVertexShader},
        {GL_FRAGMENT_SHADER, gTexturingFragmentShader}
    });

    glProgramUniform1i(passthrough,
                       glGetUniformLocation(passthrough, "inputTexture"),
                       0);

    return passthrough;
}



} // namespace detail
} // namespace graphics
} // namespace ad

