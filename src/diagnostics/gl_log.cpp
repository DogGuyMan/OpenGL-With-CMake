#include "gl_log.h"

#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    namespace detail
    {
        // 프로그램별 1회 검사 캐시 — true 면 모든 기대 uniform/attribute 존재.
        std::unordered_map<GLuint, bool> uniformChecked;
        std::unordered_map<GLuint, bool> attribChecked;
    }
}

namespace SJH::Diagnostics
{
    // 내부 헬퍼 함수들
    namespace
    {

        std::string FetchShaderInfoLog(GLuint shader)
        {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            if (length <= 0)
                return {};
            std::string log(static_cast<size_t>(length), '\0');
            glGetShaderInfoLog(shader, length, nullptr, log.data());
            if (!log.empty() && log.back() == '\0')
                log.pop_back();
            return log;
        }

        std::string FetchProgramInfoLog(GLuint program)
        {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            if (length <= 0)
                return {};
            std::string log(static_cast<size_t>(length), '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            if (!log.empty() && log.back() == '\0')
                log.pop_back();
            return log;
        }

        const char *GLErrorString(GLenum err)
        {
            switch (err)
            {
            case GL_NO_ERROR:
                return "GL_NO_ERROR";
            case GL_INVALID_ENUM:
                return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE:
                return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION:
                return "GL_INVALID_OPERATION";
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                return "GL_INVALID_FRAMEBUFFER_OPERATION";
            case GL_OUT_OF_MEMORY:
                return "GL_OUT_OF_MEMORY";
            default:
                return "GL_UNKNOWN";
            }
        }
    }

    bool GLObjectLog::CheckShaderCompile(GLuint shader, std::string_view tag)
    {
        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success)
            return true;

        const std::string log = FetchShaderInfoLog(shader);
        if (tag.empty())
            spdlog::error("shader compile failed: {}", log);
        else
            spdlog::error("shader compile failed [{}]: {}", tag, log);
        return false;
    }

    bool GLObjectLog::CheckProgramLink(GLuint program, std::string_view tag)
    {
        GLint success = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (success)
            return true;

        const std::string log = FetchProgramInfoLog(program);
        if (tag.empty())
            spdlog::error("program link failed: {}", log);
        else
            spdlog::error("program link failed [{}]: {}", tag, log);
        return false;
    }

    bool GLObjectLog::CheckProgramValidate(GLuint program, std::string_view tag)
    {
        glValidateProgram(program);
        GLint success = 0;
        glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
        if (success)
            return true;

        const std::string log = FetchProgramInfoLog(program);
        if (tag.empty())
            spdlog::warn("program validate failed: {}", log);
        else
            spdlog::warn("program validate failed [{}]: {}", tag, log);
        return false;
    }

    bool GLObjectLog::CheckExpectedUniforms(
        GLuint program,
        std::initializer_list<const char *> names,
        std::string_view tag)
    {
        const auto it = detail::uniformChecked.find(program);
        if (it != detail::uniformChecked.end())
            return it->second; // 캐시 히트 — 로그 없이 반환

        std::vector<const char *> missing;
        for (const char *name : names)
        {
            if (glGetUniformLocation(program, name) < 0)
                missing.push_back(name);
        }

        const bool allFound = missing.empty();
        if (!allFound)
        {
            if (tag.empty())
                spdlog::warn("expected uniforms missing in program {}: [{}]", program, fmt::join(missing, ", "));
            else
                spdlog::warn("expected uniforms missing in program {} [{}]: [{}]",
                             program, tag, fmt::join(missing, ", "));
        }

        detail::uniformChecked[program] = allFound;
        return allFound;
    }

    bool GLObjectLog::CheckExpectedAttributes(
        GLuint program,
        std::initializer_list<const char *> names,
        std::string_view tag)
    {
        const auto it = detail::attribChecked.find(program);
        if (it != detail::attribChecked.end())
            return it->second;

        std::vector<const char *> missing;
        for (const char *name : names)
        {
            if (glGetAttribLocation(program, name) < 0)
                missing.push_back(name);
        }

        const bool allFound = missing.empty();
        if (!allFound)
        {
            if (tag.empty())
                spdlog::warn("expected attributes missing in program {}: [{}]", program, fmt::join(missing, ", "));
            else
                spdlog::warn("expected attributes missing in program {} [{}]: [{}]",
                             program, tag, fmt::join(missing, ", "));
        }

        detail::attribChecked[program] = allFound;
        return allFound;
    }

    void GLObjectLog::InvalidateProgramCache(GLuint program)
    {
        detail::uniformChecked.erase(program);
        detail::attribChecked.erase(program);
    }

    bool GLDebug::CheckGLGenVertexArrays()
    {
        const GLenum err = glGetError();
        switch (err)
        {
        case GL_NO_ERROR:
            return true;
        case GL_INVALID_VALUE:
            spdlog::error("glGenVertexArrays: n<0 (passed 1)");
            return false;
        default:
            spdlog::error("glGenVertexArrays: unexpected 0x{:x}", err);
            return false;
        }
    }
    bool GLDebug::CheckGLBindVertexArray(const GLuint vao)
    {
        const GLenum err = glGetError();
        switch (err)
        {
        case GL_NO_ERROR:
            return true;
        case GL_INVALID_OPERATION:
            spdlog::error("glBindVertexArray: invalid VAO handle ({}) — "
                          "not from glGenVertexArrays or already deleted",
                          vao);
            return false;
        default:
            spdlog::error("glBindVertexArray: unexpected 0x{:x}", err);
            return false;
        }
    }

    bool GLDebug::CheckGLGenBuffers(const GLuint vbo)
    {
        const GLenum err = glGetError();
        switch (err)
        {
        case GL_NO_ERROR:
            return true;
        case GL_INVALID_ENUM:
            spdlog::error("glBindBuffer: target enum not accepted "
                          "(must be GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, ...)");
            return false;
        case GL_INVALID_VALUE:
            spdlog::error("glBindBuffer: buffer ({}) not from glGenBuffers", vbo);
            return false;
        default:
            spdlog::error("glBindBuffer: unexpected 0x{:x}", err);
            return false;
        }
    }

    bool GLDebug::CheckGLBindBuffer(const GLuint vbo)
    {
        const GLenum err = glGetError();
        switch (err)
        {
        case GL_NO_ERROR:
            return true;
        case GL_INVALID_ENUM:
            spdlog::error("glBindBuffer: target enum not accepted "
                          "(must be GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, ...)");
            return false;
        case GL_INVALID_VALUE:
            spdlog::error("glBindBuffer: buffer ({}) not from glGenBuffers", vbo);
            return false;
        default:
            spdlog::error("glBindBuffer: unexpected 0x{:x}", err);
            return false;
        }
    }

    bool GLDebug::CheckGLBufferData(const GLint data_size)
    {
        const GLenum err = glGetError();
        switch (err)
        {
        case GL_NO_ERROR:
            return true;
        case GL_INVALID_ENUM:
            spdlog::error("glBufferData: target or usage not accepted");
            return false;
        case GL_INVALID_VALUE:
            spdlog::error("glBufferData: size<0 ({})", data_size);
            return false;
        case GL_INVALID_OPERATION:
            spdlog::error("glBufferData: buffer 0 bound (= no glBindBuffer), "
                          "or buffer is currently mapped");
            return false;
        case GL_OUT_OF_MEMORY:
            spdlog::error("glBufferData: GPU OOM ({} bytes)", data_size);
            return false;
        default:
            spdlog::error("glBufferData: unexpected 0x{:x}", err);
            return false;
        }
    }

    bool GLDebug::CheckGLEnableVertexAttribArray(GLuint layout_location)
    {
        const GLenum err = glGetError();
        switch (err)
        {
        case GL_NO_ERROR:
            return true;
        case GL_INVALID_OPERATION:
            spdlog::error("glEnableVertexAttribArray: VAO not bound (3.3 core 강제)");
            return false;
        case GL_INVALID_VALUE:
            spdlog::error("glEnableVertexAttribArray: index 0 >= GL_MAX_VERTEX_ATTRIBS");
            return false;
        default:
            spdlog::error("glEnableVertexAttribArray: unexpected 0x{:x}", err);
            return false;
        }
    }
    bool GLDebug::CheckGLVertexAttribPointer(const std::vector<GLuint> &&strides)
    {
        const GLenum err = glGetError();
        switch (err)
        {
        case GL_NO_ERROR:
            return true;
        case GL_INVALID_VALUE:
            spdlog::error("glVertexAttribPointer: index>=max, size not in {{1,2,3,4,GL_BGRA}}, "
                          "or stride<0 ({})",
                          fmt::join(strides, ","));
            return false;
        case GL_INVALID_ENUM:
            spdlog::error("glVertexAttribPointer: type not accepted "
                          "(must be GL_FLOAT, GL_INT, GL_HALF_FLOAT, ...)");
            return false;
        case GL_INVALID_OPERATION:
            spdlog::error("glVertexAttribPointer: VAO not bound, or "
                          "non-zero offset without VBO bound");
            return false;
        default:
            spdlog::error("glVertexAttribPointer: unexpected 0x{:x}", err);
            return false;
        }
    }
}
