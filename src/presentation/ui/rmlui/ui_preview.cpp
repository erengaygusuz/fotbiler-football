#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <cstdio>
#include <string>

#include "presentation/ui/rmlui/career_detail_binder.hpp"
#include "presentation/ui/rmlui/career_detail_view_model.hpp"
#include "presentation/ui/rmlui/career_ui_binder.hpp"
#include "presentation/ui/rmlui/career_ui_preview_data.hpp"
#include "presentation/ui/rmlui/career_ui_view_model.hpp"
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

SDL_GameController* OpenController(int deviceIndex) {
  if (deviceIndex < 0 || !SDL_IsGameController(deviceIndex)) {
    return nullptr;
  }

  SDL_GameController* controller = SDL_GameControllerOpen(deviceIndex);
  if (controller) {
    std::fprintf(stdout, "Fotbiler UI Preview: controller connected: %s\n",
                 SDL_GameControllerName(controller));
  }
  return controller;
}

SDL_GameController* OpenFirstAvailableController() {
  const int joystickCount = SDL_NumJoysticks();
  for (int index = 0; index < joystickCount; ++index) {
    if (SDL_GameController* controller = OpenController(index)) {
      return controller;
    }
  }
  return nullptr;
}

SDL_JoystickID ControllerInstanceId(SDL_GameController* controller) {
  if (!controller) {
    return -1;
  }
  return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
}

void SendNavigationKey(blunted::ui::RmlUiSystem& ui, SDL_Keycode key) {
  SDL_Event keyDown{};
  keyDown.type = SDL_KEYDOWN;
  keyDown.key.type = SDL_KEYDOWN;
  keyDown.key.state = SDL_PRESSED;
  keyDown.key.repeat = 0;
  keyDown.key.keysym.scancode = SDL_GetScancodeFromKey(key);
  keyDown.key.keysym.sym = key;
  keyDown.key.keysym.mod = KMOD_NONE;
  ui.HandleEvent(keyDown);

  SDL_Event keyUp = keyDown;
  keyUp.type = SDL_KEYUP;
  keyUp.key.type = SDL_KEYUP;
  keyUp.key.state = SDL_RELEASED;
  ui.HandleEvent(keyUp);
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

  const CareerSave previewSave = blunted::ui::BuildCareerPreviewSave();
  const blunted::ui::CareerUiViewModel careerView =
      blunted::ui::BuildCareerUiViewModel(previewSave);
  const blunted::ui::CareerDetailViewModel detailView =
      blunted::ui::BuildCareerDetailViewModel(previewSave);

  blunted::ui::ScreenRouter router([&ui, &careerView, &detailView](const std::string& path) {
    if (!LoadPreviewDocument(ui, path)) {
      return false;
    }
    blunted::ui::BindCareerUiViewModel(ui, careerView);
    blunted::ui::BindCareerDetailViewModel(ui, detailView);
    return true;
  });
  if (!router.Navigate(blunted::ui::ScreenId::MainMenu)) {
    ui.Shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GameController* controller = OpenFirstAvailableController();
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
          case SDLK_F8:
            router.Navigate(blunted::ui::ScreenId::CareerModeSelect);
            forwardToUi = false;
            break;
          case SDLK_F9:
            router.Navigate(blunted::ui::ScreenId::MatchSetup);
            forwardToUi = false;
            break;
          default:
            break;
        }
      } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        forwardToUi = false;
        switch (event.cbutton.button) {
          case SDL_CONTROLLER_BUTTON_DPAD_UP:
            SendNavigationKey(ui, SDLK_UP);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            SendNavigationKey(ui, SDLK_DOWN);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            SendNavigationKey(ui, SDLK_LEFT);
            break;
          case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            SendNavigationKey(ui, SDLK_RIGHT);
            break;
          case SDL_CONTROLLER_BUTTON_A:
            activateFocusedElement = true;
            break;
          case SDL_CONTROLLER_BUTTON_B:
            if (!router.Back()) {
              running = false;
            }
            break;
          default:
            break;
        }
      } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
        forwardToUi = false;
        if (!controller) {
          controller = OpenController(event.cdevice.which);
        }
      } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        forwardToUi = false;
        if (controller && ControllerInstanceId(controller) == event.cdevice.which) {
          SDL_GameControllerClose(controller);
          controller = OpenFirstAvailableController();
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

  if (controller) {
    SDL_GameControllerClose(controller);
  }
  ui.Shutdown();
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
