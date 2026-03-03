add_library("program" STATIC
    "program.cpp"
)
target_include_directories("program" PUBLIC ${SJH_OPENGL_SRC_DIR})
