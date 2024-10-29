#include "Shading.h"

#include "ShaderSource.h"

#include <optional>

#include <cassert>


namespace ad {
namespace graphics {


ShaderSourceView::ShaderSourceView(const ShaderSource & aShaderSource) :
    mSource{aShaderSource.getSource()},
    mMap{&aShaderSource.getSourceMap()}
{}


Shader::Shader(GLenum aStage, ShaderSourceView aSource) : Shader(aStage)
{
    compileShader(*this, aSource);
}


std::optional<std::string> extractGlslError(GLuint objectId,
                                            GLenum aStatusEnumerator,
                                            std::function<void(GLuint, GLenum, GLint*)> statusGetter,
                                            std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> infoLogGetter)
{
    GLint status;
    statusGetter(objectId, aStatusEnumerator, &status);
    if(status == GL_FALSE)
    {
        GLint maxLength = 0;
        statusGetter(objectId, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        infoLogGetter(objectId, maxLength, &maxLength, infoLog.data());

        // -1, because the returned log is null terminated.
        std::string errorLog(infoLog.begin(), infoLog.end() - 1);
        return {errorLog};
    }
    else
    {
        return std::nullopt;
    }
}


void handleCompilationError(GLuint aObjectId, ShaderSourceView aSource)
{
    // Diagnostic will receive the processed output.
    std::ostringstream diagnostic;
    if(auto errorLog =
            extractGlslError(aObjectId, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog))
    {
        try
        {
            // Extract each line from the shader source code.
            // Disclaimer: this is stupid, so many string copies \o/
            // Some might argue that it somehow reflects the standard library (why can't you getLine on a string_view?)
            // TODO Replace with a vector of indices range for each line in the view.
            std::istringstream source{std::string{aSource}};
            std::string sourceLine;
            std::vector<std::string> sourceLines;
            while(std::getline(source, sourceLine))
            {
                sourceLines.push_back(sourceLine);
            }

            // Prepare a diagnostic for each error, notably showing the offending line.
            std::istringstream log{*errorLog};
            std::string errorLine;

            std::getline(log, errorLine); // following getline calls are in the inner while
            while(!errorLine.empty())
            {
#ifdef _MSVC_LANG
                auto left = errorLine.find("(");
                auto right = errorLine.find(")");
                int column = std::stoi(errorLine.substr(0, left));    
                int line = std::stoi(errorLine.substr(left + 1, right - left));    
#else
                auto left = errorLine.find(":");
                auto right = errorLine.find("(");
                auto endCol = errorLine.find(")");
                int column = std::stoi(errorLine.substr(right + 1, endCol - right));    
                int line = std::stoi(errorLine.substr(left + 1, right - left));    
#endif
                assert(line > 0);

                // Assemble error message with the current body, 
                // plus all following lines that are indented.
                std::string errorMessage = errorLine.substr(right + 4) + "\n";
                while(std::getline(log, errorLine) && errorLine.starts_with("    "))
                {
                    errorMessage += errorLine + "\n";
                }

                // Once a non-indented line has been found, the error has been treated entirely.
                auto mapping = [&]() -> SourceMap::Mapping
                {
                     if (aSource.mMap != nullptr)
                     {
                        return aSource.mMap->getLine(line);
                     }
                     else
                     {
                        return {aSource.mIdentifier, (std::size_t)line};
                     }
                }();

                // Using a format allowing VSCode to follow the link, 3rd format in this:
                // https://github.com/microsoft/vscode/issues/140780#issuecomment-1015630638
                diagnostic << mapping.mIdentifier << " on line " << mapping.mLine << ", column "  << column << " : "
                    << sourceLines[line - 1] << "\n"
                    << "- " << errorMessage << "\n";
            }
        }
        catch(std::exception &aException)
        {
            diagnostic << *errorLog
                       << "Error log analysis failed with exception: " << aException.what();
        }
        throw ShaderCompilationError("GLSL compilation error", "\n" + diagnostic.str());
    }
}


void handleLinkError(GLuint aObjectId)
{
    if (auto errorLog = 
            extractGlslError(aObjectId, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog))
    {
        throw ShaderCompilationError("GLSL link error", *errorLog);
    }
}


void compileShader(const Shader & aShader, ShaderSourceView aSource)
{
    const char * const data = aSource.data();
    const GLint size = static_cast<GLint>(aSource.size());
    glShaderSource(aShader, 1, &data, &size);
    glCompileShader(aShader);

    handleCompilationError(aShader, aSource);
}


} // namespace graphics
} // namespace ad
