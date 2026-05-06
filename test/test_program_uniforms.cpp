/**
 * @file test_program_uniforms.cpp
 * @brief @c SJH::ProgramUniforms 의 location 캐시 + setter 동작 검증.
 *
 * @details
 *  - 인라인 GLSL 셰이더를 컴파일/링크해 *실제* program 핸들로 ProgramUniforms 생성.
 *  - 활성 uniform location 조회 / 미존재 uniform 핸들링 / setter crash 없음 검증.
 */

#include <catch2/catch_test_macros.hpp>

#include "support/gl_test_fixture.h"
#include "program/program_uniforms.h"

#include <glad/glad.h>

namespace
{
    // 인라인 GLSL 컴파일 헬퍼 — Shader 클래스 우회 (테스트 의존 최소화).
    GLuint CompileShaderInline(GLenum type, const char *src)
    {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    }

    GLuint LinkProgramInline(GLuint vs, GLuint fs)
    {
        GLuint p = glCreateProgram();
        glAttachShader(p, vs);
        glAttachShader(p, fs);
        glLinkProgram(p);
        return p;
    }

    constexpr const char *kVertexSrc =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "uniform mat4 uModel;\n"
        "uniform vec3 uTranslate;\n"
        "void main() { gl_Position = uModel * vec4(aPos + uTranslate, 1.0); }\n";

    constexpr const char *kFragmentSrc =
        "#version 330 core\n"
        "uniform vec4 uColor;\n"
        "uniform float uAlpha;\n"
        "out vec4 frag;\n"
        "void main() { frag = vec4(uColor.rgb, uColor.a * uAlpha); }\n";
}

TEST_CASE("ProgramUniforms: 활성 uniform 의 location 캐싱", "[program_uniforms]")
{
    SJH::test::GLContextFixture ctx;

    GLuint vs = CompileShaderInline(GL_VERTEX_SHADER, kVertexSrc);
    GLuint fs = CompileShaderInline(GL_FRAGMENT_SHADER, kFragmentSrc);
    GLuint program = LinkProgramInline(vs, fs);

    SJH::ProgramUniforms uniforms(program);

    SECTION("선언/사용된 uniform 은 location >= 0")
    {
        // 셰이더가 *실제로 사용* 하는 uniform 만 active. 옵티마이저가 미사용은 제거.
        REQUIRE(uniforms.Get("uModel")     >= 0);
        REQUIRE(uniforms.Get("uTranslate") >= 0);
        REQUIRE(uniforms.Get("uColor")     >= 0);
        REQUIRE(uniforms.Get("uAlpha")     >= 0);
    }

    SECTION("미존재 이름은 -1 + 두 번 호출도 안전 (lazy 보강 캐시 + warn-once)")
    {
        REQUIRE(uniforms.Get("uNonExistent")  == -1);
        REQUIRE(uniforms.Get("uNonExistent")  == -1);  // 동일 호출 — silent
        REQUIRE(uniforms.Get("uAnotherMiss")  == -1);  // 다른 이름 — warn
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteProgram(program);
}

TEST_CASE("ProgramUniforms: setter family 는 crash 없이 GL 호출 위임", "[program_uniforms]")
{
    SJH::test::GLContextFixture ctx;

    GLuint vs = CompileShaderInline(GL_VERTEX_SHADER, kVertexSrc);
    GLuint fs = CompileShaderInline(GL_FRAGMENT_SHADER, kFragmentSrc);
    GLuint program = LinkProgramInline(vs, fs);

    glUseProgram(program); // setter 들이 의미 있게 동작하려면 use 상태여야 함.

    SJH::ProgramUniforms uniforms(program);

    const float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    const float v3[3] = { 1.0f, 2.0f, 3.0f };
    const float v4[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

    uniforms.SetMat4 ("uModel",     identity);
    uniforms.SetVec3 ("uTranslate", v3);
    uniforms.SetVec4 ("uColor",     v4);
    uniforms.SetFloat("uAlpha",     0.75f);

    // 미존재 이름에 set — diagnostics warn-once + GL 호출 skip (crash 없음).
    uniforms.SetMat4("uMissingMat", identity);
    uniforms.SetVec4("uMissingVec", v4);

    SUCCEED("모든 setter 호출이 crash 없이 완료");

    glUseProgram(0);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteProgram(program);
}

TEST_CASE("ProgramUniforms: 같은 이름 반복 Get 은 캐시 히트", "[program_uniforms]")
{
    SJH::test::GLContextFixture ctx;

    GLuint vs = CompileShaderInline(GL_VERTEX_SHADER, kVertexSrc);
    GLuint fs = CompileShaderInline(GL_FRAGMENT_SHADER, kFragmentSrc);
    GLuint program = LinkProgramInline(vs, fs);

    SJH::ProgramUniforms uniforms(program);

    const GLint loc1 = uniforms.Get("uModel");
    const GLint loc2 = uniforms.Get("uModel");
    const GLint loc3 = uniforms.Get("uModel");

    REQUIRE(loc1 >= 0);
    REQUIRE(loc1 == loc2);
    REQUIRE(loc2 == loc3);

    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteProgram(program);
}
