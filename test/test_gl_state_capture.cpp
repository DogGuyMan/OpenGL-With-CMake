/**
 * @file test_gl_state_capture.cpp
 * @brief CaptureGLState 회귀 — 결정성, 부수효과 0, GL_NO_ERROR, fresh default, bind 반영.
 */

#include <catch2/catch_test_macros.hpp>

#include "diagnostics/gl_state_fields.h"
#include "support/gl_test_fixture.h"
#include <glad/glad.h>

using SJH::Diagnostics::CaptureGLState;

namespace
{
    void DrainGLErrors()
    {
        while (glGetError() != GL_NO_ERROR)
        {
        }
    }
}

TEST_CASE("CaptureGLState 결정성 — 두 번 연속 호출 byte-equal", "[diagnostics][capture]")
{
    SJH::test::GLContextFixture ctx;
    DrainGLErrors();

    auto a = CaptureGLState();
    auto b = CaptureGLState();

    // struct 비교 — std::array는 == 지원
    REQUIRE(a.vao == b.vao);
    REQUIRE(a.program == b.program);
    REQUIRE(a.active_texture == b.active_texture);
    REQUIRE(a.texture_2d_per_unit == b.texture_2d_per_unit);
    REQUIRE(a.viewport == b.viewport);
    REQUIRE(a.depth_test_enabled == b.depth_test_enabled);
    REQUIRE(a.blend_enabled == b.blend_enabled);
    REQUIRE(a.clear_color == b.clear_color);
}

TEST_CASE("CaptureGLState 부수효과 0 — active_texture 변하지 않음", "[diagnostics][capture]")
{
    SJH::test::GLContextFixture ctx;
    DrainGLErrors();

    glActiveTexture(GL_TEXTURE5); // 의도적으로 unit 5로 변경
    GLint before = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &before);

    CaptureGLState(); // 내부에서 16 unit 순회 후 복원해야 함

    GLint after = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &after);
    REQUIRE(before == after);
    REQUIRE(after == GL_TEXTURE5);
}

TEST_CASE("CaptureGLState 후 GL_NO_ERROR", "[diagnostics][capture]")
{
    SJH::test::GLContextFixture ctx;
    DrainGLErrors();

    auto fields = CaptureGLState();

    GLenum err = glGetError();
    REQUIRE(err == GL_NO_ERROR);
}

TEST_CASE("fresh fixture default — VAO=0, program=0, viewport=(0,0,256,256)",
          "[diagnostics][capture]")
{
    SJH::test::GLContextFixture ctx(256, 256);
    DrainGLErrors();

    auto f = CaptureGLState();
    REQUIRE(f.vao == 0u);
    REQUIRE(f.program == 0u);
    REQUIRE(f.viewport[0] == 0);
    REQUIRE(f.viewport[1] == 0);
    REQUIRE(f.viewport[2] == 256);
    REQUIRE(f.viewport[3] == 256);
}

TEST_CASE("CaptureGLState — VAO 바인딩 후 fields.vao 반영", "[diagnostics][capture]")
{
    SJH::test::GLContextFixture ctx;
    DrainGLErrors();

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    auto f = CaptureGLState();
    REQUIRE(f.vao == vao);

    glDeleteVertexArrays(1, &vao);
}
