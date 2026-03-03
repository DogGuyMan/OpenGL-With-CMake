# ── Source Modules ────────────────────────────────────────────
# Common include root: all modules use src/ as the base include path.
# Headers are included as: #include "common/types.h", etc.
set(SJH_OPENGL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "SJH_OPENGL source include root")

add_subdirectory("buffer")
add_subdirectory("common")
add_subdirectory("context")
add_subdirectory("layout")
add_subdirectory("program")
add_subdirectory("shader")
