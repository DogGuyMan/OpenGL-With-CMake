#ifndef __SJH_DIAGNOSTICS_GL_STATE_FIELDS_H__
#define __SJH_DIAGNOSTICS_GL_STATE_FIELDS_H__

#pragma once

#include <glad/glad.h>
#include <array>
#include <string>

namespace SJH::Diagnostics
{
    /// 한 시점의 GL 상태 스냅샷. log + snapshot 양쪽이 공유.
    struct GLStateFields
    {
        // 바인딩
        GLuint vao{0};
        GLuint program{0};
        GLuint array_buffer{0};
        GLuint element_buffer{0};
        GLuint draw_fbo{0};
        GLuint read_fbo{0};

        // 텍스처 (macOS GL 3.3 spec 상한 16; 0이 아닌 unit만 ToString 출력)
        GLenum active_texture{GL_TEXTURE0};
        std::array<GLuint, 16> texture_2d_per_unit{};

        // viewport (x, y, w, h)
        std::array<GLint, 4> viewport{};

        // 픽셀 파이프라인
        bool depth_test_enabled{false};
        GLenum depth_func{GL_LESS};
        bool depth_write_mask{true};

        bool blend_enabled{false};
        GLenum blend_src_rgb{GL_ONE};
        GLenum blend_dst_rgb{GL_ZERO};

        bool cull_face_enabled{false};
        GLenum cull_face_mode{GL_BACK};
        GLenum front_face{GL_CCW};

        std::array<bool, 4> color_write_mask{true, true, true, true};
        std::array<GLfloat, 4> clear_color{0, 0, 0, 0};
    };

    /// 현재 GL 상태 캡처. 부수효과 0 (active_texture 저장→유닛 순회→복원).
    /// @pre  GL context active (caller 책임)
    /// @post 17 필드 모두 채워 반환. glGetError가 non-zero 였으면 spdlog::warn (값 정확성 의심)
    GLStateFields CaptureGLState();

    /// GLenum → 사람이 읽는 이름. ~28 사전 + GL_TEXTUREn 동적. 미적중 시 "0xXXXX".
    /// @note SymbolicName(0) == "GL_ZERO" — blend factor 컨텍스트 가정. 자세한 근거는
    ///       spec 2.1 / test_gl_state_fields.cpp "GL_ZERO 정책" 케이스 참조.
    const char* SymbolicName(GLenum e);

    /// GLStateFields → 사람이 읽는 다중라인 문자열.
    /// VAO=0인 경우 element_buffer 라인에 주석 자동 포함.
    /// enum 필드는 SymbolicName, GLuint 핸들은 raw 정수 (의도된 비대칭).
    std::string FieldsToString(const GLStateFields& fields);
}

#endif // __SJH_DIAGNOSTICS_GL_STATE_FIELDS_H__
