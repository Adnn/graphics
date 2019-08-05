#pragma once

#include <glad/glad.h>

#include <iostream>


namespace ad
{

#if defined(GL_VERSION_4_3)
    // Only starting in 4.3 apparently
void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
    std::cerr << "GL CALLBACK: " << ((type == GL_DEBUG_TYPE_ERROR) ? "** GL ERROR **" 
                                                                   : "")
              << "type = 0x" << type
              << ", severity = 0x" << severity
              << ", message = " << message
              << std::endl;
}

// During init, can be used to enable debug output
void enableDebugOutput()
{
    glEnable              ( GL_DEBUG_OUTPUT );
    glDebugMessageCallback( MessageCallback, 0 );
}
#endif

struct [[nodiscard]] ErrorCheck
{
    ErrorCheck()
    {
        while (glGetError() != GL_NO_ERROR)
        {
            std::cerr << "An error was waiting in the stack";
        }
    }

    ~ErrorCheck()
    {
        while (GLenum status = glGetError())
        {
            std::cerr << "The call generated error: " << std::hex << "0x" <<  status << std::endl;
        }
    }
};

} // namespace ad;
