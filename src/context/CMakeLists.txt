add_library(context STATIC
    context.cpp
)
target_include_directories(context PUBLIC ${SJH_OPENGL_SRC_DIR})
