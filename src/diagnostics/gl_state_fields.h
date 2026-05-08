#ifndef __SJH_DIAGNOSTICS_GL_STATE_FIELDS_H__
#define __SJH_DIAGNOSTICS_GL_STATE_FIELDS_H__

#pragma once

#include <glad/glad.h>
#include <array>
#include <string>

namespace SJH::Diagnostics
{
    /// 한 vertex attribute slot의 layout 상태. (audit 트랙 A 추가 — 카테고리 C 커버)
    /// @details 현재 바인딩된 VAO의 slot당 설정. VAO=0일 때는 모두 default.
    ///          @c glGetVertexAttribiv 로 조회하므로 부수효과 0.
    /// @note    잡는 회귀: vec3 attribute에 size=2 설정(C2), stride 잘못 계산(C1),
    ///          glEnableVertexAttribArray 누락(C4), wrong VBO binding(C3 등).
    struct VertexAttribInfo
    {
        bool    enabled{false};         ///< GL_VERTEX_ATTRIB_ARRAY_ENABLED
        GLint   size{4};                ///< GL_VERTEX_ATTRIB_ARRAY_SIZE (1, 2, 3, 4) — default 4
        GLenum  type{GL_FLOAT};         ///< GL_VERTEX_ATTRIB_ARRAY_TYPE — default GL_FLOAT
        bool    normalized{false};      ///< GL_VERTEX_ATTRIB_ARRAY_NORMALIZED
        GLsizei stride{0};              ///< GL_VERTEX_ATTRIB_ARRAY_STRIDE
        GLuint  buffer_binding{0};      ///< GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING (어느 VBO에서 오는지)

        bool operator==(const VertexAttribInfo& o) const noexcept
        {
            return enabled == o.enabled && size == o.size && type == o.type
                && normalized == o.normalized && stride == o.stride
                && buffer_binding == o.buffer_binding;
        }
        bool operator!=(const VertexAttribInfo& o) const noexcept { return !(*this == o); }
    };

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

        /// Vertex Attribute layout per slot — GL 3.3 spec 상한 16.
        /// (audit 트랙 A 추가 — bug-coverage-audit.md 카테고리 C 대응)
        std::array<VertexAttribInfo, 16> attribute_layouts{};
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
