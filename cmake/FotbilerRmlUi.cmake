include(FetchContent)

# RmlUi is the foundation for the new Fotbiler player-facing UI. Keep the
# dependency pinned so UI rendering/layout behavior cannot change underneath
# the game between builds.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static third-party libraries" FORCE)
set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
set(RMLUI_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
set(RMLUI_LOTTIE_PLUGIN OFF CACHE BOOL "" FORCE)
set(RMLUI_SVG_PLUGIN OFF CACHE BOOL "" FORCE)
set(RMLUI_HARFBUZZ_SAMPLE OFF CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE freetype CACHE STRING "" FORCE)

FetchContent_Declare(
  RmlUi
  GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
  GIT_TAG 6.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(RmlUi)

add_library(fotbiler_rmlui STATIC
  ${rmlui_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.cpp
  ${rmlui_SOURCE_DIR}/Backends/RmlUi_Renderer_GL3.cpp
  ${PROJECT_SOURCE_DIR}/src/presentation/ui/rmlui/rmlui_system.cpp
  ${PROJECT_SOURCE_DIR}/src/presentation/ui/rmlui/rmlui_system.hpp
)

target_compile_features(fotbiler_rmlui PRIVATE cxx_std_17)
target_compile_definitions(fotbiler_rmlui PRIVATE
  RMLUI_SDL_VERSION_MAJOR=2
  RMLUI_NUM_MSAA_SAMPLES=0
)

target_include_directories(fotbiler_rmlui
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
  PRIVATE
    ${rmlui_SOURCE_DIR}/Backends
    ${SDL2_INCLUDE_DIR}
)

target_link_libraries(fotbiler_rmlui PUBLIC RmlUi::RmlUi)

if(GF_SDL2_TARGET)
  target_link_libraries(fotbiler_rmlui PRIVATE ${GF_SDL2_TARGET})
elseif(SDL2_LIBRARIES)
  target_link_libraries(fotbiler_rmlui PRIVATE ${SDL2_LIBRARIES})
endif()

if(GF_OPENGL_TARGET)
  target_link_libraries(fotbiler_rmlui PRIVATE ${GF_OPENGL_TARGET})
elseif(OPENGL_LIBRARIES)
  target_link_libraries(fotbiler_rmlui PRIVATE ${OPENGL_LIBRARIES})
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(fotbiler_rmlui PRIVATE dl m)
endif()

# Preserve the display/window placement as Fotbiler transitions between the
# modern frontend and the 3D runtime during the migration. The wrapper is
# deliberately target-scoped so third-party SDL/RmlUi targets and test-only
# binaries are not affected.
function(fotbiler_enable_sdl_window_bridge target)
  target_sources(${target} PRIVATE
    ${PROJECT_SOURCE_DIR}/src/platform/fotbiler_sdl_window_bridge.cpp
  )
  target_compile_definitions(${target} PRIVATE
    SDL_CreateWindow=FotbilerSDLCreateWindow
    SDL_DestroyWindow=FotbilerSDLDestroyWindow
  )
endfunction()

# Standalone 2D UI lab. It intentionally uses the same RmlUiSystem and assets
# as the game, but creates its own SDL/OpenGL window so FIFA-era menu design can
# be iterated without destabilising the legacy Gui2 runtime during migration.
# Keep the career persistence pieces deliberately small here: the preview now
# opens the same save file as gameplayfootball, but it still does not link the
# legacy menu/game libraries.
add_executable(fotbiler_ui_preview
  ${PROJECT_SOURCE_DIR}/src/presentation/ui/rmlui/ui_preview.cpp
  ${PROJECT_SOURCE_DIR}/src/core/career/career_common.cpp
  ${PROJECT_SOURCE_DIR}/src/menu/career/career_persistence.cpp
)
fotbiler_enable_sdl_window_bridge(fotbiler_ui_preview)
target_compile_features(fotbiler_ui_preview PRIVATE cxx_std_17)
target_include_directories(fotbiler_ui_preview PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/src/core/career
  ${SDL2_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(fotbiler_ui_preview PRIVATE fotbiler_rmlui SQLite::SQLite3)

if(GF_SDL2_TARGET)
  target_link_libraries(fotbiler_ui_preview PRIVATE ${GF_SDL2_TARGET})
elseif(SDL2_LIBRARIES)
  target_link_libraries(fotbiler_ui_preview PRIVATE ${SDL2_LIBRARIES})
endif()

if(GF_OPENGL_TARGET)
  target_link_libraries(fotbiler_ui_preview PRIVATE ${GF_OPENGL_TARGET})
elseif(OPENGL_LIBRARIES)
  target_link_libraries(fotbiler_ui_preview PRIVATE ${OPENGL_LIBRARIES})
endif()

# RML/RCSS/font files are runtime data, not compiler inputs. Refresh them on
# every build so a style-only edit is immediately visible in the preview.
# Use PROJECT_BINARY_DIR directly here; referring to TARGET_FILE_DIR from this
# utility target would make CMake infer a reverse dependency on the executable
# and create a dependency cycle when the executable depends on this target.
set(FOTBILER_UI_PREVIEW_RUNTIME_DIR "${PROJECT_BINARY_DIR}")
add_custom_target(fotbiler_ui_preview_assets ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "${FOTBILER_UI_PREVIEW_RUNTIME_DIR}/media/ui"
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "${FOTBILER_UI_PREVIEW_RUNTIME_DIR}/media/fonts"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/data/media/ui"
    "${FOTBILER_UI_PREVIEW_RUNTIME_DIR}/media/ui"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/data/media/fonts"
    "${FOTBILER_UI_PREVIEW_RUNTIME_DIR}/media/fonts"
  COMMENT "Refreshing Fotbiler UI preview assets"
  VERBATIM
)
add_dependencies(fotbiler_ui_preview fotbiler_ui_preview_assets)