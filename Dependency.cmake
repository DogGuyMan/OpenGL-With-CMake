# 외부 라이브러리를 다운로드하고 빌드에 포함하기 위한 FetchContent 모듈 로드
include(FetchContent)

# ./build/_deps 에 추가되고, 해당 디렉토리의 CMakeLists.txt도 서브 디렉토리에 추가됨.
# dep의 유래는... dependency의 줄임말인가?
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG "v1.x"
    GIT_SHALLOW 1 # 1==true 로 하면 깃 변경 내역 최신것만 받아온다는 의미.
    # UPDATE_COMMAND "<_command_>" 업데이트가 있을떄 자동으로 CLI에서 수행할 커맨드
)

FetchContent_MakeAvailable(spdlog)
