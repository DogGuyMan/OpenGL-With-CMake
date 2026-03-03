add_library(layout STATIC
    vertex_layout.cpp
)
target_include_directories(layout PUBLIC ${SJH_OPENGL_SRC_DIR})
