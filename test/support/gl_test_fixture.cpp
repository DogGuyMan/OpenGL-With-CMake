/**
 * @file gl_test_fixture.cpp
 * @brief @c GLContextFixture 구현 — GLFW 1회 init 가드 + 인스턴스별 hidden window.
 */

#include "gl_test_fixture.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <stdexcept>

namespace
{
    namespace detail
    {
        bool glfwInitialized = false;
        bool gladLoaded      = false;
    }
}

namespace SJH::test
{
    namespace
    {
        void EnsureGLFWInit()
        {
            if (detail::glfwInitialized) return;
            if (!glfwInit())
                throw std::runtime_error("GLContextFixture: glfwInit failed");
            detail::glfwInitialized = true;
            std::atexit([] {
                if (detail::glfwInitialized)
                {
                    glfwTerminate();
                    detail::glfwInitialized = false;
                }
            });
        }
    }

    GLContextFixture::GLContextFixture(int width, int height)
        : mWidth(width), mHeight(height)
    {
        EnsureGLFWInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        mWindow = glfwCreateWindow(mWidth, mHeight, "test-gl", nullptr, nullptr);
        if (!mWindow)
            throw std::runtime_error("GLContextFixture: glfwCreateWindow failed");

        glfwMakeContextCurrent(mWindow);

        // glad 는 첫 fixture 에서만 실 로드. 같은 GL 버전(3.3 core) 컨텍스트 간엔
        // 함수 포인터 재사용 가능 — 동일 드라이버라는 가정.
        if (!detail::gladLoaded)
        {
            if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            {
                glfwDestroyWindow(mWindow);
                mWindow = nullptr;
                throw std::runtime_error("GLContextFixture: gladLoadGLLoader failed");
            }
            detail::gladLoaded = true;
        }
    }

    GLContextFixture::~GLContextFixture()
    {
        if (mWindow)
        {
            glfwDestroyWindow(mWindow);
            mWindow = nullptr;
        }
    }
}
