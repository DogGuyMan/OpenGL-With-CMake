#!/usr/bin/env bash
# Install vcpkg global (classic-mode) packages mirrored from this project's vcpkg.json.
#
# 용도:
#   - 다른 프로젝트에서도 동일 라이브러리를 쓰거나, IDE/툴체인이 global 위치를 보는 경우
#   - vcpkg.json 의 manifest install 과 별개로 ${VCPKG_ROOT}/installed/<triplet>/ 에 설치됨
#
# 사용법:
#   ./scripts/install-global-vcpkg.sh                 # triplet 자동 감지
#   ./scripts/install-global-vcpkg.sh arm64-osx       # triplet 명시
#   ./scripts/install-global-vcpkg.sh x64-windows     # (WSL/bash 환경에서)
#
# 전제:
#   - 환경변수 VCPKG_ROOT 설정 (예: export VCPKG_ROOT=$HOME/vcpkg)
#   - ${VCPKG_ROOT}/vcpkg 실행 가능

set -euo pipefail

# ── 사전 검사 ─────────────────────────────────────────
if [[ -z "${VCPKG_ROOT:-}" ]]; then
    echo "Error: VCPKG_ROOT 가 설정되지 않았습니다." >&2
    echo "  export VCPKG_ROOT=\$HOME/vcpkg   # 또는 본인 경로" >&2
    exit 1
fi

if [[ ! -x "${VCPKG_ROOT}/vcpkg" ]]; then
    echo "Error: ${VCPKG_ROOT}/vcpkg 가 없거나 실행 권한이 없습니다." >&2
    echo "  cd \"\$VCPKG_ROOT\" && ./bootstrap-vcpkg.sh   # 부트스트랩 필요" >&2
    exit 1
fi

# ── triplet 결정 ─────────────────────────────────────
TRIPLET="${1:-}"
if [[ -z "$TRIPLET" ]]; then
    case "$(uname -s)-$(uname -m)" in
        Darwin-arm64)   TRIPLET="arm64-osx" ;;
        Darwin-x86_64)  TRIPLET="x64-osx" ;;
        Linux-x86_64)   TRIPLET="x64-linux" ;;
        Linux-aarch64)  TRIPLET="arm64-linux" ;;
        *)
            echo "Error: 자동 감지 실패 — triplet 을 1번째 인자로 명시하세요." >&2
            echo "  예: $0 arm64-osx" >&2
            exit 1
            ;;
    esac
fi

# ── 패키지 목록 (vcpkg.json#5-20 미러) ────────────────
# 새 의존성을 vcpkg.json 에 추가하면 여기에도 동기화.
PACKAGES=(
    fmt
    spdlog
    glfw3
    glad
    glm
    stb
    catch2
    assimp
    'imgui[glfw-binding,opengl3-binding]'
    imguizmo
)

# ── 요약 출력 + 확인 ──────────────────────────────────
echo "VCPKG_ROOT  = ${VCPKG_ROOT}"
echo "Triplet     = ${TRIPLET}"
echo "Install dir = ${VCPKG_ROOT}/installed/${TRIPLET}"
echo "Packages    :"
for pkg in "${PACKAGES[@]}"; do
    echo "  - ${pkg}"
done
echo

read -r -p "위 패키지를 global 로 설치합니다. 진행? [y/N] " reply
case "$reply" in
    [Yy]*) ;;
    *) echo "취소."; exit 0 ;;
esac

# ── 설치 ──────────────────────────────────────────────
# IMPORTANT: vcpkg.json 이 있는 디렉토리에서 호출하면 manifest mode 로 빠져서
# 명령행 패키지 인자가 무시됨. ${VCPKG_ROOT} 에는 vcpkg.json 이 없으므로
# 거기로 이동해서 classic mode 를 강제한다.
cd "${VCPKG_ROOT}"

./vcpkg install --triplet="${TRIPLET}" "${PACKAGES[@]}"

echo
echo "✓ 완료. ${VCPKG_ROOT}/installed/${TRIPLET}/ 에 설치됨."
echo "  목록 확인: ${VCPKG_ROOT}/vcpkg list"
