add_library(common STATIC
    common.cpp
)
target_include_directories(common PUBLIC ${SJH_OPENGL_SRC_DIR})
