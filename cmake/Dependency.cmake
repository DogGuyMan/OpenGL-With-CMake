# vcpkg manifest mode (vcpkg.json) 로 관리되는 외부 라이브러리 탐색.
# vcpkg install은 cmake --preset 시점(configure phase)에 자동 실행됨.
# cmake --build 시점(build phase)에는 vcpkg가 실행되지 않음.
#
# 명시적 의존성 설치 타겟:
#   cmake --build build_Darwin --target install-deps
#
# vcpkg 자동 설치를 끄고 싶을 때 (빠른 재구성):
#   cmake --preset debug -DVCPKG_MANIFEST_INSTALL=OFF

#  install-deps 커스텀 타겟
# vcpkg.json 변경 후 의존성만 별도로 설치할 때 사용
add_custom_target(install-deps
    COMMAND $ENV{VCPKG_ROOT}/vcpkg install --triplet ${VCPKG_TARGET_TRIPLET}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "vcpkg install --triplet ${VCPKG_TARGET_TRIPLET}"
    VERBATIM
)

#  find_package
# fmt: fast formatting library
find_package(fmt CONFIG REQUIRED)

# spdlog: fast C++ logging library
find_package(spdlog CONFIG REQUIRED)

# glfw3: window / OpenGL context / input
find_package(glfw3 CONFIG REQUIRED)

# glad: OpenGL function loader
find_package(glad CONFIG REQUIRED)

find_package(glm CONFIG REQUIRED)

# stb: header-only image / utility library
# vcpkg는 stb를 imported target이 아닌 변수(${Stb_INCLUDE_DIR})로만 노출.
# 다른 의존성과의 사용 일관성을 위해 INTERFACE 타겟 stb::stb 로 래핑.
#   소비 측: target_link_libraries(<tgt> PRIVATE stb::stb)
find_package(Stb REQUIRED)
