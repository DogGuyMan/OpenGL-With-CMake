#include "gl_log.h"

#include <spdlog/spdlog.h>
#include <string>

namespace SJH::diagnostics
{
    // 내부 헬퍼들 — public API(GLObjectLog/GLDebug) 가 사용. 외부 노출 없음.
    namespace
    {
        std::string FetchShaderInfoLog(GLuint shader)
        {
            GLint length = 0;
            //shader에 대한 정수형 정보를 얻어옴
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            if (length <= 0) return {};
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
            if (length <= 0) return {};
            std::string log(static_cast<size_t>(length), '\0');
            // glGetProgramInfoLog(): program에 대한 로그를 얻어옴.
            // 링크 에러 얻어내는 용도로 사용
            glGetProgramInfoLog(program, length, nullptr, log.data());
            if (!log.empty() && log.back() == '\0')
                log.pop_back();
            return log;
        }

        const char *GLErrorString(GLenum err)
        {
            switch (err)
            {
                case GL_NO_ERROR:                      return "GL_NO_ERROR";
                case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
                case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
                case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
                case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
                case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
                default:                               return "GL_UNKNOWN";
            }
        }

#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
        void GLAPIENTRY OnGLDebugMessage(
            GLenum source, GLenum type, GLuint id, GLenum severity,
            GLsizei /*length*/, const GLchar *message, const void * /*userParam*/)
        {
            spdlog::level::level_enum level = spdlog::level::info;
            switch (severity)
            {
                case GL_DEBUG_SEVERITY_HIGH:         level = spdlog::level::err;   break;
                case GL_DEBUG_SEVERITY_MEDIUM:       level = spdlog::level::warn;  break;
                case GL_DEBUG_SEVERITY_LOW:          level = spdlog::level::info;  break;
                case GL_DEBUG_SEVERITY_NOTIFICATION: level = spdlog::level::trace; break;
            }
            spdlog::log(level, "[GL] src=0x{:x} type=0x{:x} id={} : {}",
                        source, type, id, message);
        }
#endif
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

    void GLDebug::Init()
    {
#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
        if (glDebugMessageCallback != nullptr)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(&OnGLDebugMessage, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            spdlog::info("[GL] debug callback enabled (KHR_debug)");
            return;
        }
#endif
        spdlog::info("[GL] debug callback unavailable; using polling fallback (SJH_GL_CHECK)");
    }

    bool GLDebug::DrainErrors(const char *expr, const char *file, int line)
    {
        bool clean = true;
        while (true)
        {
            const GLenum err = glGetError();
            if (err == GL_NO_ERROR)
                break;
            clean = false;
            spdlog::error("[GL] {} at {}:{} -> {}", expr, file, line, GLErrorString(err));
        }
        return clean;
    }
}
