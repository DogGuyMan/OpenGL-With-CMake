# #!/bin/bash
# # 1-4. Hello CMake 

# CurPath=$(pwd)
# ProjectName="$1"
# FILE_PATH="$CurPath/$ProjectName"

# display_usage() { 
#     echo "Type ProjectName by argument"
#     echo "Like bash ~/CMakeInitTemplate [ProjectName]"
# }

# make_dir_with_message() {
#     mkdir -v $1
#     echo "$2 디렉토리 생성"
# }

# # 파일 쓰기
# # https://stackoverflow.com/questions/11162406/open-and-write-data-to-text-file-using-bash
# create_CMakeLists_text() {
#     touch CMakeLists.txt
#     echo "# 최소 CMkae버젼 요구사항 명시
# cmake_minimum_required(VERSION 3.0.0)

# # 프로젝트 이름 지정
# project(${ProjectName})

# # 타겟 프로그램 정의
# add_executable(${ProjectName} src/main.cpp)

# # 조건문
# if(BUILD_TESTING)
#     # 메세지 출력
#     message('Hello Test')
# endif()" >> CMakeLists.txt
# }

# # GNU 폴더 구조
# set_cmake_folderStructure() {
#     make_dir_with_message bin "동적 라이브러리와 실행 파일" # 1. 동적 라이브러리와 실행파일   : bin
#     make_dir_with_message build "빌드 파일"             # 2. "빌드 파일"
#     make_dir_with_message data "데이터와 에셋"          # 3. 데이터와 에셋    : data
#     make_dir_with_message demo "데모 파일"              # 4. 데모 : demo
#     make_dir_with_message doc "문서 파일"               # 5. 문서 : doc
#     make_dir_with_message include "헤더 파일"           # 6. 헤더파일 : include 
#     make_dir_with_message lib "라이브러리 파일"           # 7. 라이브러리   : lib
#     make_dir_with_message src "소스 파일"               # 8. 소스파일 : src
#     make_dir_with_message test "테스트 파일"            # 9. 테스트   : test
#     touch src/main.cpp
# }

# if [ -z "$ProjectName" ]; then
#     display_usage
#     exit 0
# fi

# # 폴더 구조 제작
# # 파일이 있는지 없는지 확인하기 https://co-no.tistory.com/109
# if [ -e "$FILE_PATH" ]; then
#     echo "$ProjectName File Exists"
#     cd "${ProjectName}"
# else
#     mkdir "${ProjectName}"
#     cd "${ProjectName}"
#     set_cmake_folderStructure || exit
# fi

# # CMake 만들기
# if [ -e "CMakeLists.txt" ]; then
#     echo "CMakeLists.txt File Exists"
#     exit
# else
#     create_CMakeLists_text || exit
# fi

# cd "$CurPath"