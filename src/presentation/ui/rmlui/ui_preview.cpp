#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <cstdio>

#include "presentation/ui/rmlui/rmlui_system.hpp"

namespace {

constexpr int kInitialWidth = 1600;
constexpr int kInitialHeight = 900;
constexpr const char* kMainMenuDocument = "media/ui/fotbiler/main_menu.rml";
constexpr const char* kCareerCentralDocument = "media/ui/fotbiler/career_central.rml";
constexpr const char* kSquadDocument = "media/ui/fotbiler/squad.rml";
constexpr const char* kTransfersDocument = "media/ui/fotbiler/transfers.rml";
constexpr const char* kOfficeDocument = "media/ui/fotbiler/office.rml";
constexpr const char* kSeasonDocument = "media/ui/fotbiler/season.rml";
constexpr const char* kTacticsDocument = "media/ui/fotbiler/tactics.rml";

void UpdateDrawableSize(SDL_Window* window, blunted::ui::RmlUiSystem& ui) {
  int width = 0;
  int height = 0;
  SDL_GL_GetDrawableSize(window, &width, &height);
  if (width > 0 && height > 0) {
    glViewport(0, 0, width, height);
    ui.SetDimensions(width, height);
  }
}

bool LoadPreviewDocument(blunted::ui::RmlUiSystem& ui, const char* path) {
  if (ui.LoadDocument(path)) {
    return true;
  }
  std::fprintf(stderr, "Fotbiler UI Preview: could not load %s\n", path);
  return false;
}

}  // namespace

int main() {
  SDL_SetMainReady();

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
    std::fprintf(stderr, "Fotbiler UI Preview: SDL init failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_Window* window = SDL_CreateWindow(
      "Fotbiler Football - FIFA Era UI Preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      kInitialWidth, kInitialHeight,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
  if (!window) {
    std::fprintf(stderr, "Fotbiler UI Preview: window creation failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    std::fprintf(stderr, "Fotbiler UI Preview: GL context creation failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GL_MakeCurrent(window, glContext);
  SDL_GL_SetSwapInterval(1);

  int drawableWidth = 0;
  int drawableHeight = 0;
  SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

  blunted::ui::RmlUiSystem ui;
  if (!ui.Initialize(window, drawableWidth, drawableHeight)) {
    std::fprintf(stderr, "Fotbiler UI Preview: RmlUi initialization failed.\n");
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  if (!LoadPreviewDocument(ui, kMainMenuDocument)) {
    ui.Shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  UpdateDrawableSize(window, ui);

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        running = false;
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
        LoadPreviewDocument(ui, kMainMenuDocument);
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F2) {
        LoadPreviewDocument(ui, kCareerCentralDocument);
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F3) {
        LoadPreviewDocument(ui, kSquadDocument);
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F4) {
        LoadPreviewDocument(ui, kTransfersDocument);
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F5) {
        LoadPreviewDocument(ui, kOfficeDocument);
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F6) {
        LoadPreviewDocument(ui, kSeasonDocument);
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F7) {
        LoadPreviewDocument(ui, kTacticsDocument);
      } else if (event.type == SDL_WINDOWEVENT &&
                 (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                  event.window.event == SDL_WINDOWEVENT_RESIZED)) {
        UpdateDrawableSize(window, ui);
      }

      ui.HandleEvent(event);
    }

    glClearColor(0.035f, 0.055f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ui.Update();
    ui.Render();
    SDL_GL_SwapWindow(window);
  }

  ui.Shutdown();
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
