# 타겟 실행파일 정의
add_executable(${PROJECT_NAME} main.cpp)

# 타겟을 링킹할 때 필요한 타겟이나 라이브러러리를 지정
# 라이브러리 지정시 ::은 namespace까지도 의미하는것으로 보임?
target_link_libraries(${PROJECT_NAME} PUBLIC ${DEP_LIBS})
