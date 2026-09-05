// Fotbiler SDL window lifecycle bridge.
//
// During the RmlUi migration the modern frontend and the legacy match runtime
// still create separate SDL windows. Preserve the originating window's display
// and position in environment state so every subsequent Fotbiler window opens
// on the same monitor instead of SDL_WINDOWPOS_CENTERED silently selecting the
// primary display.

#ifdef SDL_CreateWindow
#undef SDL_CreateWindow
#endif
#ifdef SDL_DestroyWindow
#undef SDL_DestroyWindow
#endif

#include <SDL2/SDL.h>

#include <cstdlib>
#include <string>

namespace {

constexpr const char* kDisplayIndexEnv = "FOTBILER_UI_DISPLAY_INDEX";
constexpr const char* kWindowXEnv = "FOTBILER_UI_WINDOW_X";
constexpr const char* kWindowYEnv = "FOTBILER_UI_WINDOW_Y";

bool EnvironmentInt(const char* name, int& value) {
  const char* text = SDL_getenv(name);
  if (!text || text[0] == '\0') {
    return false;
  }

  char* end = nullptr;
  const long parsed = std::strtol(text, &end, 10);
  if (!end || *end != '\0') {
    return false;
  }

  value = static_cast<int>(parsed);
  return true;
}

void SetEnvironmentInt(const char* name, int value) {
  const std::string text = std::to_string(value);
  SDL_setenv(name, text.c_str(), 1);
}

bool ModernUiSessionActive() {
  const char* session = SDL_getenv("FOTBILER_UI_MODERN_SESSION");
  return session && session[0] != '\0' && std::string(session) != "0";
}

int CenteredOnDisplay(int displayIndex) {
  // SDL2 encodes a display index in the low bits of the centered-position
  // sentinel. Use the public mask instead of hard-coding a screen origin.
  return static_cast<int>(SDL_WINDOWPOS_CENTERED_MASK | static_cast<Uint32>(displayIndex));
}

}  // namespace

extern "C" SDL_Window* SDLCALL FotbilerSDLCreateWindow(const char* title, int x, int y, int w,
                                                        int h, Uint32 flags) {
  int displayIndex = -1;
  const int displayCount = SDL_GetNumVideoDisplays();
  if (EnvironmentInt(kDisplayIndexEnv, displayIndex) && displayIndex >= 0 &&
      displayIndex < displayCount) {
    const bool fullscreen = (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;

    if (fullscreen || SDL_WINDOWPOS_ISCENTERED(x) || SDL_WINDOWPOS_ISUNDEFINED(x)) {
      x = CenteredOnDisplay(displayIndex);
    } else {
      int savedX = 0;
      if (EnvironmentInt(kWindowXEnv, savedX)) {
        x = savedX;
      }
    }

    if (fullscreen || SDL_WINDOWPOS_ISCENTERED(y) || SDL_WINDOWPOS_ISUNDEFINED(y)) {
      y = CenteredOnDisplay(displayIndex);
    } else {
      int savedY = 0;
      if (EnvironmentInt(kWindowYEnv, savedY)) {
        y = savedY;
      }
    }
  }

  const char* resolvedTitle = ModernUiSessionActive() ? "Fotbiler Football" : title;
  return SDL_CreateWindow(resolvedTitle, x, y, w, h, flags);
}

extern "C" void SDLCALL FotbilerSDLDestroyWindow(SDL_Window* window) {
  if (window) {
    const int displayIndex = SDL_GetWindowDisplayIndex(window);
    if (displayIndex >= 0) {
      SetEnvironmentInt(kDisplayIndexEnv, displayIndex);
    }

    int x = 0;
    int y = 0;
    SDL_GetWindowPosition(window, &x, &y);
    SetEnvironmentInt(kWindowXEnv, x);
    SetEnvironmentInt(kWindowYEnv, y);
  }

  SDL_DestroyWindow(window);
}
