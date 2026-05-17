#ifndef __SJH_COMMON_H__
#define __SJH_COMMON_H__

#pragma once

#include <memory>
#include <optional>
#include <string>

/**
 * @def CLASS_PTR
 * @brief 클래스 forward declaration + std 스마트 포인터 별칭을 일괄 생성하는 매크로.
 * @details 다음 3가지 typedef 를 한 번에 정의:
 *  - `<klassName>UPtr` — @c std::unique_ptr<klassName> (단독 소유)
 *  - `<klassName>Ptr`  — @c std::shared_ptr<klassName> (공유 소유)
 *  - `<klassName>WPtr` — @c std::weak_ptr<klassName>   (약한 참조)
 * @par 예시
 * @code
 * CLASS_PTR(Shader)  // ShaderUPtr / ShaderPtr / ShaderWPtr 자동 생성
 * @endcode
 * @note 모든 @c SJH:: 네임스페이스 클래스 헤더 상단에서 사용.
 */
#define CLASS_PTR(klassName)                            \
    class klassName;                                    \
    using klassName##UPtr = std::unique_ptr<klassName>; \
    using klassName##Ptr = std::shared_ptr<klassName>;  \
    using klassName##WPtr = std::weak_ptr<klassName>;

namespace SJH
{
    /**
     * @brief 텍스트 파일을 한 번에 읽어 @c std::string 으로 반환.
     * @param filename 읽을 파일의 경로 (실행 디렉토리 기준 상대 경로 또는 절대 경로).
     * @return 성공 시 파일 내용 문자열, 실패 시 @c std::nullopt.
     * @details 실패 사유(파일 없음/권한 등)는 @c spdlog::error 로 출력.
     *          GLSL 셰이더 소스 등 텍스트 리소스 로딩에 사용.
     */
    std::optional<std::string> LoadTextFile(const std::string &filename);

    // depth test 비교 연산자 선택 — 라벨 배열 순서는 아래 DEPTH_FUNC[] 와 동일해야 함.
    // Depth Test를 꺼야하는 상황은? -> ImGUI를 사용할때 이다.
    // Depth 독립적으로 항상 앞으로 그려야 한다. 혹은 항상 뒤로 그려야 한다 할때.
    // glClearDepth(1.0f)
    //      제일 가까운애가 0, 제일 멀리있는게 1
    //      GL_LESS : 1보다 더 작은애를 먼저 그리게 한다.
    // ┌───────┬─────────────┬──────────────────────────────────┐
    // │ 인덱스 │ 값          │ 의미                             │
    // ├───────┼─────────────┼──────────────────────────────────┤
    // │ 0     │ GL_ALWAYS   │ 항상 통과 (depth test 무력화 효과) │
    // │ 1     │ GL_NEVER    │ 항상 실패 (아무것도 안 그려짐)    │
    // │ 2     │ GL_LESS     │ 더 가까우면 통과 (기본값)        │
    // │ 3     │ GL_LEQUAL   │ 같거나 가까우면 통과             │
    // │ 4     │ GL_GREATER  │ 더 멀면 통과                     │
    // │ 5     │ GL_GEQUAL   │ 같거나 멀면 통과                 │
    // │ 6     │ GL_EQUAL    │ 깊이 같을 때만                   │
    // │ 7     │ GL_NOTEQUAL │ 깊이 다를 때만                   │
    // └───────┴─────────────┴──────────────────────────────────┘
    static const char *DEPTH_FUNC_LABELS[] = {
        "GL_ALWAYS", "GL_NEVER",
        "GL_LESS", "GL_LEQUAL",
        "GL_GREATER", "GL_GEQUAL",
        "GL_EQUAL", "GL_NOTEQUAL"};

    static const GLuint DEPTH_FUNC[] = {
        GL_ALWAYS, GL_NEVER,
        GL_LESS, GL_LEQUAL,
        GL_GREATER, GL_GEQUAL,
        GL_EQUAL, GL_NOTEQUAL};
}

#endif //__SJH_COMMON_H__
