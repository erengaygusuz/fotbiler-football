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

# SDL bridge shared by the frontend handoff and the production match renderer.
# On modern match sessions it also owns the RmlUi overlay host that renders in
# the existing gameplay OpenGL context instead of creating another window.
add_library(fotbiler_sdl_window_bridge STATIC
  ${PROJECT_SOURCE_DIR}/src/platform/fotbiler_sdl_window_bridge.cpp
)
target_compile_features(fotbiler_sdl_window_bridge PRIVATE cxx_std_17)
target_include_directories(fotbiler_sdl_window_bridge PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${SDL2_INCLUDE_DIR}
)
target_link_libraries(fotbiler_sdl_window_bridge PUBLIC fotbiler_rmlui)
if(GF_SDL2_TARGET)
  target_link_libraries(fotbiler_sdl_window_bridge PRIVATE ${GF_SDL2_TARGET})
elseif(SDL2_LIBRARIES)
  target_link_libraries(fotbiler_sdl_window_bridge PRIVATE ${SDL2_LIBRARIES})
endif()

# The standalone preview only needs placement persistence. Its own event/render
# loop already hosts RmlUi directly, so do not intercept PollEvent/SwapWindow.
function(fotbiler_enable_sdl_window_bridge target)
  target_compile_definitions(${target} PRIVATE
    SDL_CreateWindow=FotbilerSDLCreateWindow
    SDL_DestroyWindow=FotbilerSDLDestroyWindow
  )
  target_link_libraries(${target} PRIVATE fotbiler_sdl_window_bridge)
endfunction()

# OpenGLRenderer3D owns the production SDL window, event pump, final buffer swap
# and GL context teardown. Intercept only those stable SDL boundaries. Legacy
# sessions pass through unchanged; FOTBILER_UI_MODERN_SESSION activates RmlUi.
set_property(SOURCE
  ${PROJECT_SOURCE_DIR}/src/systems/graphics/rendering/opengl_renderer3d.cpp
  APPEND PROPERTY COMPILE_DEFINITIONS
    SDL_CreateWindow=FotbilerSDLCreateWindow
    SDL_DestroyWindow=FotbilerSDLDestroyWindow
    SDL_PollEvent=FotbilerSDLPollEvent
    SDL_GL_SwapWindow=FotbilerSDLGLSwapWindow
    SDL_GL_DeleteContext=FotbilerSDLGLDeleteContext
)

# Do not use directory-wide link_libraries() here. This file is included before
# the project's own targets and before GoogleTest, so a directory-wide bridge
# dependency leaks into gtest/gmock and breaks their install(EXPORT ...) graph.
# Instead wrap the SDL target that the parent CMakeLists already adds only to
# gameplayfootball's final LIBRARIES list. The bridge and RmlUi therefore stay
# scoped to the production executable while tests keep their original graph.
add_library(fotbiler_runtime_sdl INTERFACE)
if(GF_SDL2_TARGET)
  target_link_libraries(fotbiler_runtime_sdl INTERFACE ${GF_SDL2_TARGET})
elseif(SDL2_LIBRARIES)
  target_link_libraries(fotbiler_runtime_sdl INTERFACE ${SDL2_LIBRARIES})
endif()
target_link_libraries(fotbiler_runtime_sdl INTERFACE fotbiler_sdl_window_bridge)
set(GF_SDL2_TARGET fotbiler_runtime_sdl)

# Standalone 2D UI lab. It intentionally uses the same RmlUiSystem and assets
# as the game, but creates its own SDL/OpenGL window for fast design iteration.
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
# every build so a style-only edit is immediately visible in preview and match.
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
  COMMENT "Refreshing Fotbiler UI runtime assets"
  VERBATIM
)
add_dependencies(fotbiler_ui_preview fotbiler_ui_preview_assets)