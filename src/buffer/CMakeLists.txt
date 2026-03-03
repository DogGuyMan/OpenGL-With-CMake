add_library(buffer STATIC
    buffer.cpp
)
target_include_directories(buffer PUBLIC ${SJH_OPENGL_SRC_DIR})
