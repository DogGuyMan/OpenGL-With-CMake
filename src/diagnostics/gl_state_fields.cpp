#include "diagnostics/gl_state_fields.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace SJH::Diagnostics
{
    const char* SymbolicName(GLenum e)
    {
        // GL_TEXTURE0..GL_TEXTURE15 동적 영역
        if (e >= GL_TEXTURE0 && e <= GL_TEXTURE0 + 15) {
            // thread_local 정적 버퍼 — caller가 즉시 출력하면 안전, 보관 시엔 std::string 권장
            static thread_local char buf[16];
            snprintf(buf, sizeof(buf), "GL_TEXTURE%d", static_cast<int>(e - GL_TEXTURE0));
            return buf;
        }

        switch (e) {
            // depth_func / stencil_func 공용 8개
            case GL_NEVER:    return "GL_NEVER";
            case GL_LESS:     return "GL_LESS";
            case GL_EQUAL:    return "GL_EQUAL";
            case GL_LEQUAL:   return "GL_LEQUAL";
            case GL_GREATER:  return "GL_GREATER";
            case GL_NOTEQUAL: return "GL_NOTEQUAL";
            case GL_GEQUAL:   return "GL_GEQUAL";
            case GL_ALWAYS:   return "GL_ALWAYS";

            // blend factor — GL 3.3 core 한정 (SRC1 family는 4.4+ 제외)
            // 0은 GL_ZERO — blend factor 컨텍스트 가정 (spec 2.1)
            case 0:                              return "GL_ZERO";
            case GL_ONE:                         return "GL_ONE";
            case GL_SRC_COLOR:                   return "GL_SRC_COLOR";
            case GL_ONE_MINUS_SRC_COLOR:         return "GL_ONE_MINUS_SRC_COLOR";
            case GL_DST_COLOR:                   return "GL_DST_COLOR";
            case GL_ONE_MINUS_DST_COLOR:         return "GL_ONE_MINUS_DST_COLOR";
            case GL_SRC_ALPHA:                   return "GL_SRC_ALPHA";
            case GL_ONE_MINUS_SRC_ALPHA:         return "GL_ONE_MINUS_SRC_ALPHA";
            case GL_DST_ALPHA:                   return "GL_DST_ALPHA";
            case GL_ONE_MINUS_DST_ALPHA:         return "GL_ONE_MINUS_DST_ALPHA";
            case GL_CONSTANT_COLOR:              return "GL_CONSTANT_COLOR";
            case GL_ONE_MINUS_CONSTANT_COLOR:    return "GL_ONE_MINUS_CONSTANT_COLOR";
            case GL_CONSTANT_ALPHA:              return "GL_CONSTANT_ALPHA";
            case GL_ONE_MINUS_CONSTANT_ALPHA:    return "GL_ONE_MINUS_CONSTANT_ALPHA";
            case GL_SRC_ALPHA_SATURATE:          return "GL_SRC_ALPHA_SATURATE";

            // cull/front face
            case GL_FRONT:           return "GL_FRONT";
            case GL_BACK:            return "GL_BACK";
            case GL_FRONT_AND_BACK:  return "GL_FRONT_AND_BACK";
            case GL_CCW:             return "GL_CCW";
            case GL_CW:              return "GL_CW";
        }

        // 미적중 — hex fallback. 4자리 대문자 (예: "0xDEAD")
        // thread_local 버퍼: caller가 즉시 출력 가정. 영구 보관 시 fmt::format 사용 권장.
        static thread_local char buf[16];
        snprintf(buf, sizeof(buf), "0x%04X", static_cast<unsigned>(e));
        return buf;
    }

    // FieldsToString은 Task 5 (Log)에서 채움 — 이 task엔 stub
    std::string FieldsToString(const GLStateFields&) { return ""; }

    // CaptureGLState는 Task 2에서 채움 — 이 task엔 stub
    GLStateFields CaptureGLState() { return {}; }
}
