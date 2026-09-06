#ifdef SDL_SetWindowTitle
#undef SDL_SetWindowTitle
#endif

#include <SDL2/SDL.h>

#include <cstdlib>
#include <string>

namespace {

bool EnvironmentFlagEnabled(const char* name) {
  const char* value = std::getenv(name);
  return value && value[0] != '\0' && std::string(value) != "0";
}

bool ModernUiActive() {
  return EnvironmentFlagEnabled("FOTBILER_UI_MODERN_APP") ||
         EnvironmentFlagEnabled("FOTBILER_UI_MODERN_SESSION");
}

}  // namespace

extern "C" void SDLCALL FotbilerSDLSetWindowTitle(SDL_Window* window, const char* title) {
  SDL_SetWindowTitle(window, ModernUiActive() ? "Fotbiler Football" : title);
}
