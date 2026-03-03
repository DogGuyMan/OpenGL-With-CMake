# ── Source Modules ────────────────────────────────────────────
# Common include root: all modules use src/ as the base include path.
# Headers are included as: #include "common/types.h", etc.
set(SJH_OPENGL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "SJH_OPENGL source include root")

# # Phase 1 — Foundation
# add_subdirectory(common)
# add_subdirectory(rom)

# # Phase 2 — Hardware Abstraction
# add_subdirectory(mapper)
# add_subdirectory(bus)
# add_subdirectory(ppu_bus)

# # Phase 3 — CPU
# add_subdirectory(cpu)

# # Phase 4 — PPU
# add_subdirectory(ppu)

# # Phase 5 — Integration
# add_subdirectory(input)
# add_subdirectory(emulator)

# # Phase 5/6 — SDL2 & Audio
# add_subdirectory(platform)
# add_subdirectory(apu)
