#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <cstdio>
#include <string>

#include "presentation/ui/rmlui/rmlui_system.hpp"
#include "presentation/ui/rmlui/screen_router.hpp"

namespace {

constexpr int kInitialWidth = 1600;
constexpr int kInitialHeight = 900;

void UpdateDrawableSize(SDL_Window* window, blunted::ui::RmlUiSystem& ui) {
  int width = 0;
  int height = 0;
  SDL_GL_GetDrawableSize(window, &width, &height);
  if (width > 0 && height > 0) {
    glViewport(0, 0, width, height);
    ui.SetDimensions(width, height);
  }
}

bool LoadPreviewDocument(blunted::ui::RmlUiSystem& ui, const std::string& path) {
  if (ui.LoadDocument(path)) {
    return true;
  }
  std::fprintf(stderr, "Fotbiler UI Preview: could not load %s\n", path.c_str());
  return false;
}

void NavigatePendingRoute(blunted::ui::RmlUiSystem& ui, blunted::ui::ScreenRouter& router) {
  const std::string route = ui.ConsumeRouteRequest();
  if (!route.empty() && !router.NavigateByName(route)) {
    std::fprintf(stderr, "Fotbiler UI Preview: unknown or failed route %s\n", route.c_str());
  }
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

  blunted::ui::ScreenRouter router(
      [&ui](const std::string& path) { return LoadPreviewDocument(ui, path); });
  if (!router.Navigate(blunted::ui::ScreenId::MainMenu)) {
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
      bool forwardToUi = true;
      bool activateFocusedElement = false;

      if (event.type == SDL_QUIT) {
        running = false;
        forwardToUi = false;
      } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            if (!router.Back()) {
              running = false;
            }
            forwardToUi = false;
            break;
          case SDLK_RETURN:
          case SDLK_KP_ENTER:
            activateFocusedElement = true;
            forwardToUi = false;
            break;
          case SDLK_F1:
            router.Navigate(blunted::ui::ScreenId::MainMenu);
            forwardToUi = false;
            break;
          case SDLK_F2:
            router.Navigate(blunted::ui::ScreenId::CareerCentral);
            forwardToUi = false;
            break;
          case SDLK_F3:
            router.Navigate(blunted::ui::ScreenId::Squad);
            forwardToUi = false;
            break;
          case SDLK_F4:
            router.Navigate(blunted::ui::ScreenId::Transfers);
            forwardToUi = false;
            break;
          case SDLK_F5:
            router.Navigate(blunted::ui::ScreenId::Office);
            forwardToUi = false;
            break;
          case SDLK_F6:
            router.Navigate(blunted::ui::ScreenId::Season);
            forwardToUi = false;
            break;
          case SDLK_F7:
            router.Navigate(blunted::ui::ScreenId::Tactics);
            forwardToUi = false;
            break;
          default:
            break;
        }
      } else if (event.type == SDL_WINDOWEVENT &&
                 (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                  event.window.event == SDL_WINDOWEVENT_RESIZED)) {
        UpdateDrawableSize(window, ui);
      }

      if (forwardToUi) {
        ui.HandleEvent(event);
      }
      if (activateFocusedElement) {
        ui.ActivateFocusedElement();
      }
      NavigatePendingRoute(ui, router);
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
