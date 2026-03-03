add_library("shader" STATIC
    "shader.cpp"
)
target_include_directories("shader" PUBLIC ${SJH_OPENGL_SRC_DIR})
