// Fotbiler SDL/window bridge and modern match-overlay host.
//
// The production renderer still owns SDL_Window and the OpenGL context. Rather
// than create a second UI window, modern match overlays are injected at the two
// stable SDL boundaries already used by OpenGLRenderer3D: event polling and
// buffer swap. This lets RmlUi render in the exact same window/context as the
// 3D match while the broader frontend migration continues.

#ifdef SDL_CreateWindow
#undef SDL_CreateWindow
#endif
#ifdef SDL_DestroyWindow
#undef SDL_DestroyWindow
#endif
#ifdef SDL_PollEvent
#undef SDL_PollEvent
#endif
#ifdef SDL_GL_SwapWindow
#undef SDL_GL_SwapWindow
#endif
#ifdef SDL_GL_DeleteContext
#undef SDL_GL_DeleteContext
#endif

#include <SDL2/SDL.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "presentation/ui/rmlui/rmlui_system.hpp"
#include "presentation/ui/rmlui/runtime_ui_bridge.hpp"

namespace {

constexpr const char* kDisplayIndexEnv = "FOTBILER_UI_DISPLAY_INDEX";
constexpr const char* kWindowXEnv = "FOTBILER_UI_WINDOW_X";
constexpr const char* kWindowYEnv = "FOTBILER_UI_WINDOW_Y";
constexpr const char* kLoadingHomeNameEnv = "FOTBILER_UI_LOADING_HOME_NAME";
constexpr const char* kLoadingAwayNameEnv = "FOTBILER_UI_LOADING_AWAY_NAME";

SDL_Window* g_runtimeWindow = nullptr;
std::unique_ptr<blunted::ui::RmlUiSystem> g_runtimeUi;
blunted::ui::runtime::Screen g_loadedScreen = blunted::ui::runtime::Screen::None;
bool g_exitMatchConfirmOpen = false;
bool g_runtimeWindowPresented = false;

bool EnvironmentInt(const char* name, int& value) {
  const char* text = SDL_getenv(name);
  if (!text || text[0] == '\0') return false;
  char* end = nullptr;
  const long parsed = std::strtol(text, &end, 10);
  if (!end || *end != '\0') return false;
  value = static_cast<int>(parsed);
  return true;
}

std::string EnvironmentString(const char* name, const char* fallback) {
  const char* value = SDL_getenv(name);
  return value && value[0] != '\0' ? std::string(value) : std::string(fallback);
}

std::string CrestLetter(const std::string& name) {
  for (char ch : name) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
      if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - ('a' - 'A'));
      return std::string(1, ch);
    }
  }
  return "F";
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
  return static_cast<int>(SDL_WINDOWPOS_CENTERED_MASK | static_cast<Uint32>(displayIndex));
}

const char* DocumentForScreen(blunted::ui::runtime::Screen screen) {
  using blunted::ui::runtime::Screen;
  switch (screen) {
    case Screen::Loading: return "media/ui/fotbiler/loading_match.rml";
    case Screen::Pause: return "media/ui/fotbiler/pause_menu.rml";
    case Screen::MatchStats: return "media/ui/fotbiler/match_stats.rml";
    case Screen::TeamManagement: return "media/ui/fotbiler/team_management.rml";
    case Screen::Replay: return "media/ui/fotbiler/replay_modern.rml";
    case Screen::Settings: return "media/ui/fotbiler/runtime_settings.rml";
    case Screen::CameraSettings: return "media/ui/fotbiler/camera_settings.rml";
    case Screen::ControllerSelect: return "media/ui/fotbiler/controller_select_modern.rml";
    case Screen::VisualSettings: return "media/ui/fotbiler/visual_settings.rml";
    case Screen::None: break;
  }
  return nullptr;
}

void InitializeRuntimeUiIfNeeded(SDL_Window* window) {
  if (!ModernUiSessionActive() || !window || g_runtimeUi) return;

  int width = 0;
  int height = 0;
  SDL_GL_GetDrawableSize(window, &width, &height);
  if (width <= 0 || height <= 0) return;

  auto ui = std::make_unique<blunted::ui::RmlUiSystem>();
  if (!ui->Initialize(window, width, height)) return;

  g_runtimeWindow = window;
  g_runtimeUi = std::move(ui);
  g_loadedScreen = blunted::ui::runtime::Screen::None;
  g_exitMatchConfirmOpen = false;

  // The frontend owns the visible loading presentation. The gameplay window
  // is created hidden and is only revealed once GamePage confirms a live match.
  // This prevents KDE/Wayland from compositing a second near-identical loading
  // window for one or two frames during process handoff.
  if (blunted::ui::runtime::GetScreen() == blunted::ui::runtime::Screen::None) {
    blunted::ui::runtime::SetScreen(blunted::ui::runtime::Screen::Loading);
  }
}

void ShutdownRuntimeUi() {
  if (g_runtimeUi) {
    g_runtimeUi->Shutdown();
    g_runtimeUi.reset();
  }
  g_runtimeWindow = nullptr;
  g_loadedScreen = blunted::ui::runtime::Screen::None;
  g_exitMatchConfirmOpen = false;
  g_runtimeWindowPresented = false;
  blunted::ui::runtime::Reset();
}

void BindLoadingContext() {
  if (!g_runtimeUi || g_loadedScreen != blunted::ui::runtime::Screen::Loading) return;

  const std::string home = EnvironmentString(kLoadingHomeNameEnv, "HOME TEAM");
  const std::string away = EnvironmentString(kLoadingAwayNameEnv, "AWAY TEAM");
  const std::string session = EnvironmentString("FOTBILER_UI_MODERN_SESSION", "quick");
  const bool career = session == "career";

  g_runtimeUi->SetElementText("loading-home-name", home);
  g_runtimeUi->SetElementText("loading-away-name", away);
  g_runtimeUi->SetElementText("loading-home-crest", CrestLetter(home));
  g_runtimeUi->SetElementText("loading-away-crest", CrestLetter(away));
  g_runtimeUi->SetElementText("loading-mode",
                              career ? "CAREER MODE · MATCHDAY" : "QUICK MATCH · MATCH ENGINE");
  g_runtimeUi->SetElementText("loading-kicker", career ? "CAREER FIXTURE" : "KICK OFF");
  g_runtimeUi->SetElementText("loading-title",
                              career ? "PREPARING CAREER MATCH" : "PREPARING MATCH");
}

void ShowExitMatchConfirmation() {
  if (!g_runtimeUi || g_loadedScreen != blunted::ui::runtime::Screen::Pause) return;
  if (!g_runtimeUi->SetElementProperty("exit-match-confirm-overlay", "display", "block")) return;
  g_exitMatchConfirmOpen = true;
  g_runtimeUi->FocusElement("exit-match-cancel");
}

void HideExitMatchConfirmation() {
  if (!g_runtimeUi) return;
  g_runtimeUi->SetElementProperty("exit-match-confirm-overlay", "display", "none");
  g_exitMatchConfirmOpen = false;
  if (!g_runtimeUi->FocusElement("pause-exit-match")) {
    g_runtimeUi->FocusDefaultElement();
  }
}

void HandleRoute(const std::string& route) {
  using blunted::ui::runtime::Screen;
  if (g_exitMatchConfirmOpen) return;
  if (route == "pause-menu") blunted::ui::runtime::SetScreen(Screen::Pause);
  else if (route == "team-management") blunted::ui::runtime::SetScreen(Screen::TeamManagement);
  else if (route == "match-stats") blunted::ui::runtime::SetScreen(Screen::MatchStats);
  else if (route == "replay-modern") blunted::ui::runtime::SetScreen(Screen::Replay);
  else if (route == "runtime-settings") blunted::ui::runtime::SetScreen(Screen::Settings);
  else if (route == "camera-settings") blunted::ui::runtime::SetScreen(Screen::CameraSettings);
  else if (route == "controller-select-modern") blunted::ui::runtime::SetScreen(Screen::ControllerSelect);
  else if (route == "visual-settings") blunted::ui::runtime::SetScreen(Screen::VisualSettings);
}

void HandleAction(const std::string& action) {
  using blunted::ui::runtime::Command;

  if (action == "cancel-exit-match") {
    HideExitMatchConfirmation();
    return;
  }
  if (action == "confirm-exit-match") {
    g_exitMatchConfirmOpen = false;
    blunted::ui::runtime::SendCommand(Command::ExitMatch);
    return;
  }
  if (g_exitMatchConfirmOpen) return;

  if (action == "resume-match") {
    blunted::ui::runtime::SendCommand(Command::ResumeMatch);
  } else if (action == "exit-match" || action == "forfeit-match") {
    ShowExitMatchConfirmation();
  }
}

void ConsumeUiRequests() {
  if (!g_runtimeUi) return;
  const std::string route = g_runtimeUi->ConsumeRouteRequest();
  if (!route.empty()) HandleRoute(route);
  const std::string action = g_runtimeUi->ConsumeActionRequest();
  if (!action.empty()) HandleAction(action);
}

void SyncDocument() {
  if (!g_runtimeUi) return;

  const blunted::ui::runtime::Screen wanted = blunted::ui::runtime::GetScreen();
  if (wanted == g_loadedScreen) return;

  const blunted::ui::runtime::Screen previous = g_loadedScreen;
  g_exitMatchConfirmOpen = false;
  g_runtimeUi->UnloadDocument();
  g_loadedScreen = blunted::ui::runtime::Screen::None;

  if (wanted == blunted::ui::runtime::Screen::None) {
    // Loading -> None is the exact point at which GamePage has a live Match.
    // Reveal the gameplay window only now, so the parent loading screen remains
    // visually continuous until the first real 3D frame is ready.
    if (previous == blunted::ui::runtime::Screen::Loading && g_runtimeWindow &&
        !g_runtimeWindowPresented) {
      SDL_ShowWindow(g_runtimeWindow);
      SDL_RaiseWindow(g_runtimeWindow);
      g_runtimeWindowPresented = true;
    }
    return;
  }

  const char* path = DocumentForScreen(wanted);
  if (path && g_runtimeUi->LoadDocument(path)) {
    g_loadedScreen = wanted;
    if (wanted == blunted::ui::runtime::Screen::Loading) BindLoadingContext();
    g_runtimeUi->FocusDefaultElement();
  }
}

void BindMatchSnapshot() {
  if (!g_runtimeUi || g_loadedScreen == blunted::ui::runtime::Screen::None ||
      g_loadedScreen == blunted::ui::runtime::Screen::Loading) {
    return;
  }

  const blunted::ui::runtime::MatchSnapshot snapshot = blunted::ui::runtime::ReadMatchSnapshot();
  const std::string score = std::to_string(snapshot.homeScore) + " - " +
                            std::to_string(snapshot.awayScore);
  const std::string matchup = snapshot.homeName + "  " + score + "  " + snapshot.awayName;

  g_runtimeUi->SetElementText("runtime-match-minute", "MATCH PAUSED · " + std::to_string(snapshot.minute) + "'");
  g_runtimeUi->SetElementText("runtime-matchup", matchup);
  g_runtimeUi->SetElementText("runtime-home-team", snapshot.homeShortName);
  g_runtimeUi->SetElementText("runtime-away-team", snapshot.awayShortName);
  g_runtimeUi->SetElementText("runtime-score", score);
  g_runtimeUi->SetElementText("runtime-home-possession", std::to_string(snapshot.homePossession) + "%");
  g_runtimeUi->SetElementText("runtime-away-possession", std::to_string(snapshot.awayPossession) + "%");
  g_runtimeUi->SetElementText("runtime-home-shots", std::to_string(snapshot.homeShots));
  g_runtimeUi->SetElementText("runtime-away-shots", std::to_string(snapshot.awayShots));

  g_runtimeUi->SetElementText("stats-matchup", matchup);
  g_runtimeUi->SetElementText("stats-home-possession", std::to_string(snapshot.homePossession) + "%");
  g_runtimeUi->SetElementText("stats-away-possession", std::to_string(snapshot.awayPossession) + "%");
  g_runtimeUi->SetElementText("stats-home-shots", std::to_string(snapshot.homeShots));
  g_runtimeUi->SetElementText("stats-away-shots", std::to_string(snapshot.awayShots));
  g_runtimeUi->SetElementText("stats-home-on-target", std::to_string(snapshot.homeShotsOnTarget));
  g_runtimeUi->SetElementText("stats-away-on-target", std::to_string(snapshot.awayShotsOnTarget));
  g_runtimeUi->SetElementText("stats-home-pass-accuracy", std::to_string(snapshot.homePassAccuracy) + "%");
  g_runtimeUi->SetElementText("stats-away-pass-accuracy", std::to_string(snapshot.awayPassAccuracy) + "%");
  g_runtimeUi->SetElementText("stats-home-fouls", std::to_string(snapshot.homeFouls));
  g_runtimeUi->SetElementText("stats-away-fouls", std::to_string(snapshot.awayFouls));
}

bool IsInputEvent(Uint32 type) {
  return type == SDL_KEYDOWN || type == SDL_KEYUP || type == SDL_TEXTINPUT ||
         type == SDL_MOUSEMOTION || type == SDL_MOUSEBUTTONDOWN || type == SDL_MOUSEBUTTONUP ||
         type == SDL_MOUSEWHEEL || type == SDL_CONTROLLERBUTTONDOWN ||
         type == SDL_CONTROLLERBUTTONUP || type == SDL_JOYBUTTONDOWN || type == SDL_JOYBUTTONUP ||
         type == SDL_JOYAXISMOTION;
}

void SendNavigationKey(SDL_Keycode key) {
  if (!g_runtimeUi) return;
  SDL_Event synthetic{};
  synthetic.type = SDL_KEYDOWN;
  synthetic.key.type = SDL_KEYDOWN;
  synthetic.key.state = SDL_PRESSED;
  synthetic.key.repeat = 0;
  synthetic.key.keysym.sym = key;
  g_runtimeUi->HandleEvent(synthetic);
}

bool HandleRuntimeInput(SDL_Event& event) {
  if (!g_runtimeUi || blunted::ui::runtime::GetScreen() == blunted::ui::runtime::Screen::None) {
    return false;
  }

  using blunted::ui::runtime::Command;
  using blunted::ui::runtime::Screen;

  // Loading is a non-interactive handoff state. Swallow player input until the
  // live match page explicitly clears it, while still allowing SDL_QUIT to pass
  // through because it is not classified as an input event here.
  if (blunted::ui::runtime::GetScreen() == Screen::Loading) {
    return IsInputEvent(event.type);
  }

  if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
    if (g_exitMatchConfirmOpen) {
      switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          HideExitMatchConfirmation();
          return true;
        case SDLK_LEFT:
        case SDLK_UP:
          g_runtimeUi->FocusElement("exit-match-cancel");
          return true;
        case SDLK_RIGHT:
        case SDLK_DOWN:
          g_runtimeUi->FocusElement("exit-match-accept");
          return true;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
          g_runtimeUi->ActivateFocusedElement();
          ConsumeUiRequests();
          return true;
        default:
          return IsInputEvent(event.type);
      }
    }

    if (event.key.keysym.sym == SDLK_ESCAPE) {
      if (blunted::ui::runtime::GetScreen() == Screen::Pause) {
        blunted::ui::runtime::SendCommand(Command::ResumeMatch);
      } else {
        blunted::ui::runtime::SetScreen(Screen::Pause);
      }
      return true;
    }
    if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
      g_runtimeUi->ActivateFocusedElement();
      ConsumeUiRequests();
      return true;
    }
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN) {
    if (g_exitMatchConfirmOpen) {
      switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
          g_runtimeUi->FocusElement("exit-match-cancel");
          return true;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
          g_runtimeUi->FocusElement("exit-match-accept");
          return true;
        case SDL_CONTROLLER_BUTTON_A:
          g_runtimeUi->ActivateFocusedElement();
          ConsumeUiRequests();
          return true;
        case SDL_CONTROLLER_BUTTON_B:
          HideExitMatchConfirmation();
          return true;
        default:
          return true;
      }
    }

    switch (event.cbutton.button) {
      case SDL_CONTROLLER_BUTTON_DPAD_UP: SendNavigationKey(SDLK_UP); return true;
      case SDL_CONTROLLER_BUTTON_DPAD_DOWN: SendNavigationKey(SDLK_DOWN); return true;
      case SDL_CONTROLLER_BUTTON_DPAD_LEFT: SendNavigationKey(SDLK_LEFT); return true;
      case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: SendNavigationKey(SDLK_RIGHT); return true;
      case SDL_CONTROLLER_BUTTON_A:
        g_runtimeUi->ActivateFocusedElement();
        ConsumeUiRequests();
        return true;
      case SDL_CONTROLLER_BUTTON_B:
        if (blunted::ui::runtime::GetScreen() == Screen::Pause)
          blunted::ui::runtime::SendCommand(Command::ResumeMatch);
        else
          blunted::ui::runtime::SetScreen(Screen::Pause);
        return true;
      default: return true;
    }
  }

  g_runtimeUi->HandleEvent(event);
  ConsumeUiRequests();
  return IsInputEvent(event.type);
}

}  // namespace

extern "C" SDL_Window* SDLCALL FotbilerSDLCreateWindow(const char* title, int x, int y, int w,
                                                        int h, Uint32 flags) {
  int displayIndex = -1;
  const int displayCount = SDL_GetNumVideoDisplays();
  if (EnvironmentInt(kDisplayIndexEnv, displayIndex) && displayIndex >= 0 && displayIndex < displayCount) {
    const bool fullscreen = (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
    if (fullscreen || SDL_WINDOWPOS_ISCENTERED(x) || SDL_WINDOWPOS_ISUNDEFINED(x)) x = CenteredOnDisplay(displayIndex);
    else { int savedX = 0; if (EnvironmentInt(kWindowXEnv, savedX)) x = savedX; }
    if (fullscreen || SDL_WINDOWPOS_ISCENTERED(y) || SDL_WINDOWPOS_ISUNDEFINED(y)) y = CenteredOnDisplay(displayIndex);
    else { int savedY = 0; if (EnvironmentInt(kWindowYEnv, savedY)) y = savedY; }
  }

  const bool modernSession = ModernUiSessionActive();
  if (modernSession) {
    // Keep the parent Fotbiler loading window visible while the child renderer
    // creates its GL context, stadium and players. Do not let the compositor
    // reveal this second SDL window until the match is actually live.
    flags &= ~SDL_WINDOW_SHOWN;
    flags |= SDL_WINDOW_HIDDEN;
  }

  const char* resolvedTitle = modernSession ? "Fotbiler Football" : title;
  SDL_Window* window = SDL_CreateWindow(resolvedTitle, x, y, w, h, flags);
  if (modernSession) {
    g_runtimeWindow = window;
    g_runtimeWindowPresented = false;
  }
  return window;
}

extern "C" void SDLCALL FotbilerSDLDestroyWindow(SDL_Window* window) {
  if (window) {
    const int displayIndex = SDL_GetWindowDisplayIndex(window);
    if (displayIndex >= 0) SetEnvironmentInt(kDisplayIndexEnv, displayIndex);
    int x = 0, y = 0;
    SDL_GetWindowPosition(window, &x, &y);
    SetEnvironmentInt(kWindowXEnv, x);
    SetEnvironmentInt(kWindowYEnv, y);
  }
  SDL_DestroyWindow(window);
}

extern "C" int SDLCALL FotbilerSDLPollEvent(SDL_Event* event) {
  while (SDL_PollEvent(event)) {
    if (ModernUiSessionActive() && g_runtimeWindow) {
      InitializeRuntimeUiIfNeeded(g_runtimeWindow);
      if (event->type == SDL_WINDOWEVENT &&
          (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
           event->window.event == SDL_WINDOWEVENT_RESIZED) && g_runtimeUi) {
        int width = 0, height = 0;
        SDL_GL_GetDrawableSize(g_runtimeWindow, &width, &height);
        if (width > 0 && height > 0) g_runtimeUi->SetDimensions(width, height);
      }
      if (HandleRuntimeInput(*event)) continue;
    }
    return 1;
  }
  return 0;
}

extern "C" void SDLCALL FotbilerSDLGLSwapWindow(SDL_Window* window) {
  if (ModernUiSessionActive()) {
    InitializeRuntimeUiIfNeeded(window);
    SyncDocument();
    if (g_runtimeUi && blunted::ui::runtime::GetScreen() != blunted::ui::runtime::Screen::None) {
      if (g_loadedScreen == blunted::ui::runtime::Screen::Loading) BindLoadingContext();
      BindMatchSnapshot();
      g_runtimeUi->Update();
      g_runtimeUi->Render();
      ConsumeUiRequests();
    }
  }
  SDL_GL_SwapWindow(window);
}

extern "C" void SDLCALL FotbilerSDLGLDeleteContext(SDL_GLContext context) {
  // RmlUi's GL renderer owns buffers/textures tied to this context. Release
  // those resources while the context is still current and valid.
  ShutdownRuntimeUi();
  SDL_GL_DeleteContext(context);
}
