#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <sqlite3.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "presentation/ui/rmlui/career_detail_binder.hpp"
#include "presentation/ui/rmlui/career_detail_view_model.hpp"
#include "presentation/ui/rmlui/career_ui_binder.hpp"
#include "presentation/ui/rmlui/career_ui_preview_data.hpp"
#include "presentation/ui/rmlui/career_ui_view_model.hpp"
#include "presentation/ui/rmlui/rmlui_system.hpp"
#include "presentation/ui/rmlui/screen_router.hpp"

namespace {

constexpr Uint64 kLoadingHoldMs = 700;

struct TeamChoice {
  int id = 0;
  std::string name;
  std::string league;
};

struct QuickMatchSetupState {
  std::vector<TeamChoice> teams;
  std::size_t homeIndex = 0;
  std::size_t awayIndex = 0;
  std::vector<int> halfLengths{5, 10, 15, 20, 30, 45};
  std::size_t halfLengthIndex = 0;
  int difficultyStep = 3;
  int controlSide = -1;

  const TeamChoice& Home() const { return teams.at(homeIndex); }
  const TeamChoice& Away() const { return teams.at(awayIndex); }
  int HalfLengthMinutes() const { return halfLengths.at(halfLengthIndex); }
  int MatchLengthMinutes() const { return HalfLengthMinutes() * 2; }
  float Difficulty() const { return std::clamp(difficultyStep, 0, 4) * 0.25f; }
};

struct RuntimeSettingsState {
  std::vector<std::pair<int, int>> resolutions{{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}};
  std::size_t resolutionIndex = 2;
  bool fullscreen = true;
  bool vsync = true;
  int difficultyStep = 3;
  int gameSpeedStep = 1;
  int volume = 80;

  int Width() const { return resolutions.at(resolutionIndex).first; }
  int Height() const { return resolutions.at(resolutionIndex).second; }
};

enum class PreviewExitAction {
  None,
  LaunchQuickMatch,
  LaunchCareerMatch,
};

bool EnvironmentFlagEnabled(const char* name) {
  const char* value = std::getenv(name);
  return value && value[0] != '\0' && std::string(value) != "0";
}

std::string GetExecutableBasePath() {
  char* basePath = SDL_GetBasePath();
  std::string path = basePath ? basePath : "";
  SDL_free(basePath);
  return path;
}

std::filesystem::path RuntimeSettingsPath() {
  return std::filesystem::path(GetExecutableBasePath()) / "user/fotbiler_ui.settings";
}

std::string EscapeRmlText(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char ch : value) {
    switch (ch) {
      case '&': escaped += "&amp;"; break;
      case '<': escaped += "&lt;"; break;
      case '>': escaped += "&gt;"; break;
      default: escaped += ch; break;
    }
  }
  return escaped;
}

std::string CrestLetter(const std::string& name) {
  for (char ch : name) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
      return std::string(1, static_cast<char>(ch >= 'a' && ch <= 'z' ? ch - 32 : ch));
    }
  }
  return "F";
}

const char* DifficultyName(int step) {
  switch (std::clamp(step, 0, 4)) {
    case 0: return "BEGINNER";
    case 1: return "AMATEUR";
    case 2: return "REGULAR";
    case 3: return "PROFESSIONAL";
    default: return "TOP PLAYER";
  }
}

const char* GameSpeedName(int step) {
  switch (std::clamp(step, 0, 2)) {
    case 0: return "SLOW";
    case 1: return "NORMAL";
    default: return "FAST";
  }
}

void SaveRuntimeSettings(const RuntimeSettingsState& state) {
  const std::filesystem::path path = RuntimeSettingsPath();
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  std::ofstream file(path);
  if (!file) {
    std::fprintf(stderr, "Fotbiler UI Preview: could not save %s\n", path.string().c_str());
    return;
  }
  file << "fullscreen=" << (state.fullscreen ? 1 : 0) << '\n';
  file << "width=" << state.Width() << '\n';
  file << "height=" << state.Height() << '\n';
  file << "vsync=" << (state.vsync ? 1 : 0) << '\n';
  file << "difficulty=" << state.difficultyStep << '\n';
  file << "game_speed=" << state.gameSpeedStep << '\n';
  file << "volume=" << state.volume << '\n';
}

RuntimeSettingsState LoadRuntimeSettings() {
  RuntimeSettingsState state;
  std::ifstream file(RuntimeSettingsPath());
  if (!file) {
    return state;
  }

  int savedWidth = state.Width();
  int savedHeight = state.Height();
  std::string line;
  while (std::getline(file, line)) {
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = line.substr(0, eq);
    const std::string value = line.substr(eq + 1);
    const int number = std::atoi(value.c_str());
    if (key == "fullscreen") state.fullscreen = number != 0;
    else if (key == "width") savedWidth = number;
    else if (key == "height") savedHeight = number;
    else if (key == "vsync") state.vsync = number != 0;
    else if (key == "difficulty") state.difficultyStep = std::clamp(number, 0, 4);
    else if (key == "game_speed") state.gameSpeedStep = std::clamp(number, 0, 2);
    else if (key == "volume") state.volume = std::clamp(number, 0, 100);
  }

  for (std::size_t i = 0; i < state.resolutions.size(); ++i) {
    if (state.resolutions[i].first == savedWidth && state.resolutions[i].second == savedHeight) {
      state.resolutionIndex = i;
      break;
    }
  }
  return state;
}

std::vector<TeamChoice> LoadTeamChoices() {
  std::vector<TeamChoice> teams;
  const std::filesystem::path dbPath =
      std::filesystem::path(GetExecutableBasePath()) / "databases/default/database.sqlite";

  sqlite3* db = nullptr;
  if (sqlite3_open_v2(dbPath.string().c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK) {
    const char* sql =
        "select teams.id, teams.name, coalesce(leagues.name, '') "
        "from teams left join leagues on teams.league_id = leagues.id "
        "order by leagues.name, teams.name";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) == SQLITE_OK) {
      while (sqlite3_step(statement) == SQLITE_ROW) {
        TeamChoice team;
        team.id = sqlite3_column_int(statement, 0);
        const unsigned char* name = sqlite3_column_text(statement, 1);
        const unsigned char* league = sqlite3_column_text(statement, 2);
        team.name = name ? reinterpret_cast<const char*>(name) : "TEAM";
        team.league = league ? reinterpret_cast<const char*>(league) : "";
        if (team.id > 0 && !team.name.empty()) teams.push_back(std::move(team));
      }
    } else {
      std::fprintf(stderr, "Fotbiler UI Preview: could not query team catalog: %s\n", sqlite3_errmsg(db));
    }
    if (statement) sqlite3_finalize(statement);
  } else {
    std::fprintf(stderr, "Fotbiler UI Preview: could not open team database at %s\n", dbPath.string().c_str());
  }
  if (db) sqlite3_close(db);

  if (teams.size() < 2) {
    teams = {{3, "BARCELONA", "DEFAULT DATABASE"}, {8, "REAL MADRID", "DEFAULT DATABASE"}};
  }
  return teams;
}

QuickMatchSetupState BuildQuickMatchSetupState() {
  QuickMatchSetupState state;
  state.teams = LoadTeamChoices();
  auto findTeam = [&state](int id) {
    for (std::size_t index = 0; index < state.teams.size(); ++index) {
      if (state.teams[index].id == id) return index;
    }
    return std::size_t{0};
  };
  state.homeIndex = findTeam(3);
  state.awayIndex = findTeam(8);
  if (state.awayIndex == state.homeIndex && state.teams.size() > 1) {
    state.awayIndex = (state.homeIndex + 1) % state.teams.size();
  }
  return state;
}

void BindQuickMatchSetup(blunted::ui::RmlUiSystem& ui, const QuickMatchSetupState& state) {
  if (state.teams.size() < 2) return;
  const TeamChoice& home = state.Home();
  const TeamChoice& away = state.Away();
  ui.SetElementText("quick-home-crest", EscapeRmlText(CrestLetter(home.name)));
  ui.SetElementText("quick-home-name", EscapeRmlText(home.name));
  ui.SetElementText("quick-home-meta", EscapeRmlText("HOME" + (home.league.empty() ? std::string() : " · " + home.league)));
  ui.SetElementText("quick-away-crest", EscapeRmlText(CrestLetter(away.name)));
  ui.SetElementText("quick-away-name", EscapeRmlText(away.name));
  ui.SetElementText("quick-away-meta", EscapeRmlText("AWAY" + (away.league.empty() ? std::string() : " · " + away.league)));
  ui.SetElementText("quick-half-length", std::to_string(state.HalfLengthMinutes()) + " MIN");
  ui.SetElementText("quick-difficulty", DifficultyName(state.difficultyStep));
  ui.SetElementText("quick-control-side", state.controlSide < 0 ? "KEYBOARD · HOME" : "KEYBOARD · AWAY");
  ui.SetElementText("loading-home-name", EscapeRmlText(home.name));
  ui.SetElementText("loading-away-name", EscapeRmlText(away.name));
  ui.SetElementText("loading-home-crest", EscapeRmlText(CrestLetter(home.name)));
  ui.SetElementText("loading-away-crest", EscapeRmlText(CrestLetter(away.name)));
}

void BindRuntimeSettings(blunted::ui::RmlUiSystem& ui, const RuntimeSettingsState& state) {
  ui.SetElementText("settings-window-mode", state.fullscreen ? "FULLSCREEN" : "WINDOWED");
  ui.SetElementText("settings-resolution", std::to_string(state.Width()) + " × " + std::to_string(state.Height()));
  ui.SetElementText("settings-vsync", state.vsync ? "ON" : "OFF");
  ui.SetElementText("settings-difficulty", DifficultyName(state.difficultyStep));
  ui.SetElementText("settings-game-speed", GameSpeedName(state.gameSpeedStep));
  ui.SetElementText("settings-volume", std::to_string(state.volume) + "%");
}

void BindCareerLoading(blunted::ui::RmlUiSystem& ui, const blunted::ui::CareerUiViewModel& careerView) {
  const std::string clubName = careerView.header.clubName.empty() ? "CAREER CLUB" : careerView.header.clubName;
  ui.SetElementText("loading-mode", "CAREER MODE · MATCHDAY");
  ui.SetElementText("loading-kicker", "CAREER FIXTURE");
  ui.SetElementText("loading-title", "PREPARING CAREER MATCH");
  ui.SetElementText("loading-home-name", EscapeRmlText(clubName));
  ui.SetElementText("loading-home-crest", EscapeRmlText(CrestLetter(clubName)));
  ui.SetElementText("loading-away-name", "NEXT LEAGUE OPPONENT");
  ui.SetElementText("loading-away-crest", "A");
  ui.SetElementText("loading-copy", "Loading the active career fixture. The final score will be autosaved after full time.");
}

void AdvanceTeam(QuickMatchSetupState& state, bool home) {
  if (state.teams.size() < 2) return;
  std::size_t& index = home ? state.homeIndex : state.awayIndex;
  const std::size_t other = home ? state.awayIndex : state.homeIndex;
  do { index = (index + 1) % state.teams.size(); } while (index == other && state.teams.size() > 1);
}

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
  if (ui.LoadDocument(path)) return true;
  std::fprintf(stderr, "Fotbiler UI Preview: could not load %s\n", path.c_str());
  return false;
}

void NavigatePendingRoute(blunted::ui::RmlUiSystem& ui, blunted::ui::ScreenRouter& router) {
  const std::string route = ui.ConsumeRouteRequest();
  if (!route.empty() && !router.NavigateByName(route)) {
    std::fprintf(stderr, "Fotbiler UI Preview: unknown or failed route %s\n", route.c_str());
  }
}

void ShowQuitConfirmation(blunted::ui::RmlUiSystem& ui, bool& open) {
  if (!ui.SetElementProperty("quit-confirm-overlay", "display", "block")) return;
  open = true;
  ui.FocusElement("quit-confirm-cancel");
}

void HideQuitConfirmation(blunted::ui::RmlUiSystem& ui, bool& open) {
  ui.SetElementProperty("quit-confirm-overlay", "display", "none");
  open = false;
  if (!ui.FocusElement("tile-career")) ui.FocusDefaultElement();
}

PreviewExitAction ConsumePendingAction(blunted::ui::RmlUiSystem& ui, QuickMatchSetupState& quickMatch,
                                       RuntimeSettingsState& settings, bool& quitConfirmOpen,
                                       bool& quitRequested) {
  const std::string action = ui.ConsumeActionRequest();
  if (action.empty()) return PreviewExitAction::None;

  if (action == "cancel-quit-game") {
    HideQuitConfirmation(ui, quitConfirmOpen);
    return PreviewExitAction::None;
  }
  if (action == "confirm-quit-game") {
    quitRequested = true;
    return PreviewExitAction::None;
  }
  if (quitConfirmOpen) return PreviewExitAction::None;

  if (action == "start-quick-match") {
    std::fprintf(stdout, "Fotbiler UI Preview: handing START MATCH to gameplayfootball runtime.\n");
    return PreviewExitAction::LaunchQuickMatch;
  }
  if (action == "start-career-match") {
    std::fprintf(stdout, "Fotbiler UI Preview: handing CAREER MATCH to gameplayfootball runtime.\n");
    return PreviewExitAction::LaunchCareerMatch;
  }
  if (action == "change-home-team") AdvanceTeam(quickMatch, true);
  else if (action == "change-away-team") AdvanceTeam(quickMatch, false);
  else if (action == "cycle-half-length") quickMatch.halfLengthIndex = (quickMatch.halfLengthIndex + 1) % quickMatch.halfLengths.size();
  else if (action == "cycle-difficulty") {
    quickMatch.difficultyStep = (quickMatch.difficultyStep + 1) % 5;
    settings.difficultyStep = quickMatch.difficultyStep;
  } else if (action == "toggle-control-side") quickMatch.controlSide = quickMatch.controlSide < 0 ? 1 : -1;
  else if (action == "toggle-fullscreen") settings.fullscreen = !settings.fullscreen;
  else if (action == "cycle-resolution") settings.resolutionIndex = (settings.resolutionIndex + 1) % settings.resolutions.size();
  else if (action == "toggle-vsync") settings.vsync = !settings.vsync;
  else if (action == "cycle-game-speed") settings.gameSpeedStep = (settings.gameSpeedStep + 1) % 3;
  else if (action == "cycle-audio-volume") settings.volume = settings.volume >= 100 ? 0 : settings.volume + 10;
  else if (action == "apply-settings") {
    SaveRuntimeSettings(settings);
    std::fprintf(stdout, "Fotbiler UI Preview: runtime settings saved.\n");
  } else {
    std::fprintf(stdout, "Fotbiler UI Preview: runtime action '%s' is staged for the in-game host.\n", action.c_str());
  }

  BindQuickMatchSetup(ui, quickMatch);
  BindRuntimeSettings(ui, settings);
  return PreviewExitAction::None;
}

bool BeginRuntimeHandoff(PreviewExitAction action, blunted::ui::RmlUiSystem& ui,
                         blunted::ui::ScreenRouter& router, const QuickMatchSetupState& quickMatch,
                         const blunted::ui::CareerUiViewModel& careerView) {
  if (!router.Navigate(blunted::ui::ScreenId::MatchLoading)) {
    std::fprintf(stderr, "Fotbiler UI Preview: could not open modern loading screen.\n");
    return false;
  }
  if (action == PreviewExitAction::LaunchCareerMatch) {
    BindCareerLoading(ui, careerView);
  } else {
    BindQuickMatchSetup(ui, quickMatch);
    ui.SetElementText("loading-mode", "QUICK MATCH · MATCH ENGINE");
    ui.SetElementText("loading-kicker", "KICK OFF");
    ui.SetElementText("loading-title", "PREPARING MATCH");
  }
  return true;
}

std::string GetGameplayExecutablePath() {
  std::string path = GetExecutableBasePath();
#ifdef _WIN32
  path += "gameplayfootball.exe";
#else
  path += "gameplayfootball";
#endif
  return path;
}

void SetEnvInt(const char* name, int value) {
  const std::string text = std::to_string(value);
  SDL_setenv(name, text.c_str(), 1);
}

void SetEnvFloat(const char* name, float value) {
  char text[32];
  std::snprintf(text, sizeof(text), "%.2f", value);
  SDL_setenv(name, text, 1);
}

void SetEnvString(const char* name, const std::string& value) {
  SDL_setenv(name, value.c_str(), 1);
}

void PublishWindowPlacement(SDL_Window* window) {
  if (!window) return;
  const int displayIndex = SDL_GetWindowDisplayIndex(window);
  if (displayIndex >= 0) SetEnvInt("FOTBILER_UI_DISPLAY_INDEX", displayIndex);
  int x = 0;
  int y = 0;
  SDL_GetWindowPosition(window, &x, &y);
  SetEnvInt("FOTBILER_UI_WINDOW_X", x);
  SetEnvInt("FOTBILER_UI_WINDOW_Y", y);
}

int LaunchGameplayFootball(const std::string& executablePath, PreviewExitAction action,
                           const QuickMatchSetupState& quickMatch,
                           const RuntimeSettingsState& settings,
                           const blunted::ui::CareerUiViewModel& careerView,
                           SDL_Window* frontendWindow) {
  if (executablePath.empty() || !std::filesystem::exists(executablePath)) {
    std::fprintf(stderr, "Fotbiler UI Preview: gameplay executable not found at %s\n", executablePath.c_str());
    return 1;
  }

  PublishWindowPlacement(frontendWindow);
  SaveRuntimeSettings(settings);

  int contextWidth = settings.Width();
  int contextHeight = settings.Height();
  if (frontendWindow) {
    int drawableWidth = 0;
    int drawableHeight = 0;
    SDL_GL_GetDrawableSize(frontendWindow, &drawableWidth, &drawableHeight);
    if (drawableWidth > 0 && drawableHeight > 0) {
      contextWidth = drawableWidth;
      contextHeight = drawableHeight;
    }
  }

  // Use the exact current drawable geometry, not just the saved logical
  // resolution. This keeps the frontend loading document and gameplay loading
  // document pixel-identical across fullscreen/HiDPI monitor handoff.
  SetEnvInt("FOTBILER_UI_CONTEXT_FULLSCREEN", settings.fullscreen ? 1 : 0);
  SetEnvInt("FOTBILER_UI_CONTEXT_X", contextWidth);
  SetEnvInt("FOTBILER_UI_CONTEXT_Y", contextHeight);

  if (action == PreviewExitAction::LaunchCareerMatch) {
    SDL_setenv("FOTBILER_UI_CAREER_MATCH", "1", 1);
    SDL_setenv("FOTBILER_UI_QUICK_MATCH", "0", 1);
    SDL_setenv("FOTBILER_UI_MODERN_SESSION", "career", 1);
    const std::string clubName =
        careerView.header.clubName.empty() ? "CAREER CLUB" : careerView.header.clubName;
    SetEnvString("FOTBILER_UI_LOADING_HOME_NAME", clubName);
    SetEnvString("FOTBILER_UI_LOADING_AWAY_NAME", "NEXT LEAGUE OPPONENT");
  } else {
    SDL_setenv("FOTBILER_UI_QUICK_MATCH", "1", 1);
    SDL_setenv("FOTBILER_UI_CAREER_MATCH", "0", 1);
    SDL_setenv("FOTBILER_UI_MODERN_SESSION", "quick", 1);
    SetEnvInt("FOTBILER_UI_HOME_TEAM_ID", quickMatch.Home().id);
    SetEnvInt("FOTBILER_UI_AWAY_TEAM_ID", quickMatch.Away().id);
    SetEnvInt("FOTBILER_UI_MATCH_DURATION_MINUTES", quickMatch.MatchLengthMinutes());
    SetEnvFloat("FOTBILER_UI_MATCH_DIFFICULTY", quickMatch.Difficulty());
    SetEnvInt("FOTBILER_UI_CONTROL_SIDE", quickMatch.controlSide);
    SetEnvString("FOTBILER_UI_LOADING_HOME_NAME", quickMatch.Home().name);
    SetEnvString("FOTBILER_UI_LOADING_AWAY_NAME", quickMatch.Away().name);
  }

  const std::string command = "\"" + executablePath + "\"";
  std::fprintf(stdout, "Fotbiler UI Preview: launching %s while keeping frontend alive.\n",
               executablePath.c_str());
  const int result = std::system(command.c_str());
  if (result == -1) {
    std::fprintf(stderr, "Fotbiler UI Preview: failed to launch gameplayfootball.\n");
  }

  // The same frontend process resumes after the match. Do not recursively
  // launch a second fotbiler_ui_preview process.
  SDL_setenv("FOTBILER_UI_CAREER_MATCH", "0", 1);
  SDL_setenv("FOTBILER_UI_QUICK_MATCH", "0", 1);
  SDL_setenv("FOTBILER_UI_MODERN_SESSION", "0", 1);
  return result == -1 ? 1 : result;
}

SDL_GameController* OpenController(int deviceIndex) {
  if (deviceIndex < 0 || !SDL_IsGameController(deviceIndex)) return nullptr;
  SDL_GameController* controller = SDL_GameControllerOpen(deviceIndex);
  if (controller) {
    std::fprintf(stdout, "Fotbiler UI Preview: controller connected: %s\n", SDL_GameControllerName(controller));
  }
  return controller;
}

SDL_GameController* OpenFirstAvailableController() {
  for (int index = 0; index < SDL_NumJoysticks(); ++index) {
    if (SDL_GameController* controller = OpenController(index)) return controller;
  }
  return nullptr;
}

SDL_JoystickID ControllerInstanceId(SDL_GameController* controller) {
  return controller ? SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)) : -1;
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

  RuntimeSettingsState settings = LoadRuntimeSettings();
  Uint32 windowFlags =
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN;
  if (settings.fullscreen) windowFlags |= SDL_WINDOW_FULLSCREEN;

  SDL_Window* window = SDL_CreateWindow(
      "Fotbiler Football", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      settings.Width(), settings.Height(), windowFlags);
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
  SDL_GL_SetSwapInterval(settings.vsync ? 1 : 0);
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

  CareerSave previewSave = blunted::ui::BuildCareerPreviewSave();
  blunted::ui::CareerUiViewModel careerView = blunted::ui::BuildCareerUiViewModel(previewSave);
  blunted::ui::CareerDetailViewModel detailView = blunted::ui::BuildCareerDetailViewModel(previewSave);
  QuickMatchSetupState quickMatch = BuildQuickMatchSetupState();
  quickMatch.difficultyStep = settings.difficultyStep;

  blunted::ui::ScreenRouter router([&](const std::string& path) {
    if (!LoadPreviewDocument(ui, path)) return false;
    blunted::ui::BindCareerUiViewModel(ui, careerView);
    blunted::ui::BindCareerDetailViewModel(ui, detailView);
    BindQuickMatchSetup(ui, quickMatch);
    BindRuntimeSettings(ui, settings);
    return true;
  });

  const blunted::ui::ScreenId initialScreen =
      EnvironmentFlagEnabled("FOTBILER_UI_RESUME_CAREER") ? blunted::ui::ScreenId::CareerCentral
                                                          : blunted::ui::ScreenId::MainMenu;
  SDL_setenv("FOTBILER_UI_RESUME_CAREER", "0", 1);
  if (!router.Reset(initialScreen)) {
    ui.Shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GameController* controller = OpenFirstAvailableController();
  UpdateDrawableSize(window, ui);

  bool running = true;
  bool handoffPending = false;
  bool quitConfirmOpen = false;
  bool quitRequested = false;
  Uint64 handoffDeadlineMs = 0;
  PreviewExitAction exitAction = PreviewExitAction::None;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (handoffPending) {
        if (event.type == SDL_QUIT) {
          exitAction = PreviewExitAction::None;
          running = false;
        } else if (event.type == SDL_WINDOWEVENT &&
                   (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                    event.window.event == SDL_WINDOWEVENT_RESIZED)) {
          UpdateDrawableSize(window, ui);
        }
        continue;
      }

      bool forwardToUi = true;
      bool activateFocusedElement = false;

      if (event.type == SDL_QUIT) {
        running = false;
        forwardToUi = false;
      } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        if (quitConfirmOpen) {
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
              HideQuitConfirmation(ui, quitConfirmOpen);
              break;
            case SDLK_LEFT:
            case SDLK_UP:
              ui.FocusElement("quit-confirm-cancel");
              break;
            case SDLK_RIGHT:
            case SDLK_DOWN:
              ui.FocusElement("quit-confirm-accept");
              break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
              activateFocusedElement = true;
              break;
            default:
              break;
          }
          forwardToUi = false;
        } else {
          switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
              if (router.Current() && *router.Current() == blunted::ui::ScreenId::MainMenu) {
                ShowQuitConfirmation(ui, quitConfirmOpen);
              } else if (!router.Back()) {
                router.Reset(blunted::ui::ScreenId::MainMenu);
              }
              forwardToUi = false;
              break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
              activateFocusedElement = true;
              forwardToUi = false;
              break;
            case SDLK_F1: router.Navigate(blunted::ui::ScreenId::MainMenu); forwardToUi = false; break;
            case SDLK_F2: router.Navigate(blunted::ui::ScreenId::CareerCentral); forwardToUi = false; break;
            case SDLK_F3: router.Navigate(blunted::ui::ScreenId::Squad); forwardToUi = false; break;
            case SDLK_F4: router.Navigate(blunted::ui::ScreenId::Transfers); forwardToUi = false; break;
            case SDLK_F5: router.Navigate(blunted::ui::ScreenId::Office); forwardToUi = false; break;
            case SDLK_F6: router.Navigate(blunted::ui::ScreenId::Season); forwardToUi = false; break;
            case SDLK_F7: router.Navigate(blunted::ui::ScreenId::Tactics); forwardToUi = false; break;
            case SDLK_F8: router.Navigate(blunted::ui::ScreenId::CareerModeSelect); forwardToUi = false; break;
            case SDLK_F9: router.Navigate(blunted::ui::ScreenId::MatchSetup); forwardToUi = false; break;
            case SDLK_F10: router.Navigate(blunted::ui::ScreenId::RuntimeSettings); forwardToUi = false; break;
            case SDLK_F11: router.Navigate(blunted::ui::ScreenId::PauseMenu); forwardToUi = false; break;
            case SDLK_F12: router.Navigate(blunted::ui::ScreenId::MatchHud); forwardToUi = false; break;
            case SDLK_h: router.Navigate(blunted::ui::ScreenId::Halftime); forwardToUi = false; break;
            case SDLK_j: router.Navigate(blunted::ui::ScreenId::Fulltime); forwardToUi = false; break;
            case SDLK_m: router.Navigate(blunted::ui::ScreenId::MatchStats); forwardToUi = false; break;
            default: break;
          }
        }
      } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        forwardToUi = false;
        if (quitConfirmOpen) {
          switch (event.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
              ui.FocusElement("quit-confirm-cancel");
              break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
              ui.FocusElement("quit-confirm-accept");
              break;
            case SDL_CONTROLLER_BUTTON_A:
              activateFocusedElement = true;
              break;
            case SDL_CONTROLLER_BUTTON_B:
              HideQuitConfirmation(ui, quitConfirmOpen);
              break;
            default:
              break;
          }
        } else {
          switch (event.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP: SendNavigationKey(ui, SDLK_UP); break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: SendNavigationKey(ui, SDLK_DOWN); break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: SendNavigationKey(ui, SDLK_LEFT); break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: SendNavigationKey(ui, SDLK_RIGHT); break;
            case SDL_CONTROLLER_BUTTON_A: activateFocusedElement = true; break;
            case SDL_CONTROLLER_BUTTON_B:
              if (router.Current() && *router.Current() == blunted::ui::ScreenId::MainMenu) {
                ShowQuitConfirmation(ui, quitConfirmOpen);
              } else if (!router.Back()) {
                router.Reset(blunted::ui::ScreenId::MainMenu);
              }
              break;
            default: break;
          }
        }
      } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
        forwardToUi = false;
        if (!controller) controller = OpenController(event.cdevice.which);
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

      if (forwardToUi) ui.HandleEvent(event);
      if (activateFocusedElement) ui.ActivateFocusedElement();
      if (!quitConfirmOpen) NavigatePendingRoute(ui, router);

      const PreviewExitAction action =
          ConsumePendingAction(ui, quickMatch, settings, quitConfirmOpen, quitRequested);
      if (quitRequested) {
        running = false;
        break;
      }
      if (action != PreviewExitAction::None) {
        exitAction = action;
        if (BeginRuntimeHandoff(action, ui, router, quickMatch, careerView)) {
          handoffPending = true;
          handoffDeadlineMs = SDL_GetTicks64() + kLoadingHoldMs;
        } else {
          running = false;
        }
        break;
      }
    }

    if (!running) break;

    glClearColor(0.035f, 0.055f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    ui.Update();
    ui.Render();
    SDL_GL_SwapWindow(window);

    if (handoffPending && SDL_GetTicks64() >= handoffDeadlineMs) {
      const PreviewExitAction completedAction = exitAction;
      handoffPending = false;
      exitAction = PreviewExitAction::None;

      const int runtimeResult = LaunchGameplayFootball(
          GetGameplayExecutablePath(), completedAction, quickMatch, settings, careerView, window);
      if (runtimeResult != 0) {
        std::fprintf(stderr, "Fotbiler UI Preview: gameplay runtime returned status %d.\n",
                     runtimeResult);
      }

      // Child process focus/context changes must not force a second frontend
      // process. Resume this exact SDL window and RmlUi context in place.
      SDL_GL_MakeCurrent(window, glContext);
      SDL_ShowWindow(window);
      SDL_RaiseWindow(window);
      UpdateDrawableSize(window, ui);

      const blunted::ui::ScreenId returnScreen =
          completedAction == PreviewExitAction::LaunchCareerMatch
              ? blunted::ui::ScreenId::CareerCentral
              : blunted::ui::ScreenId::MatchSetup;

      if (completedAction == PreviewExitAction::LaunchCareerMatch) {
        previewSave = blunted::ui::BuildCareerPreviewSave();
        careerView = blunted::ui::BuildCareerUiViewModel(previewSave);
        detailView = blunted::ui::BuildCareerDetailViewModel(previewSave);
      }

      if (!router.Reset(returnScreen)) {
        std::fprintf(stderr, "Fotbiler UI Preview: could not restore frontend after match.\n");
        running = false;
      }
    }
  }

  if (controller) SDL_GameControllerClose(controller);
  ui.Shutdown();
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
