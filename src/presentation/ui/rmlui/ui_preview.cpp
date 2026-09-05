#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <sqlite3.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <cstdio>
#include <string>
#include <vector>

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
  int controlSide = -1;  // -1 = home, 1 = away, matching the legacy side convention.

  const TeamChoice& Home() const { return teams.at(homeIndex); }
  const TeamChoice& Away() const { return teams.at(awayIndex); }
  int HalfLengthMinutes() const { return halfLengths.at(halfLengthIndex); }
  int MatchLengthMinutes() const { return HalfLengthMinutes() * 2; }
  float Difficulty() const { return std::clamp(difficultyStep, 0, 4) * 0.25f; }
};

enum class PreviewExitAction {
  None,
  LaunchQuickMatch,
  LaunchCareerMatch,
};

std::string GetExecutableBasePath() {
  char* basePath = SDL_GetBasePath();
  std::string path = basePath ? basePath : "";
  SDL_free(basePath);
  return path;
}

std::string EscapeRmlText(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char ch : value) {
    switch (ch) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      default:
        escaped += ch;
        break;
    }
  }
  return escaped;
}

std::string TeamCrestLetter(const TeamChoice& team) {
  for (char ch : team.name) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
      return std::string(1, static_cast<char>(ch >= 'a' && ch <= 'z' ? ch - 32 : ch));
    }
  }
  return "F";
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
        if (team.id > 0 && !team.name.empty()) {
          teams.push_back(std::move(team));
        }
      }
    } else {
      std::fprintf(stderr, "Fotbiler UI Preview: could not query team catalog: %s\n",
                   sqlite3_errmsg(db));
    }
    if (statement) {
      sqlite3_finalize(statement);
    }
  } else {
    std::fprintf(stderr, "Fotbiler UI Preview: could not open team database at %s\n",
                 dbPath.string().c_str());
  }
  if (db) {
    sqlite3_close(db);
  }

  if (teams.size() < 2) {
    teams.clear();
    teams.push_back({3, "BARCELONA", "DEFAULT DATABASE"});
    teams.push_back({8, "REAL MADRID", "DEFAULT DATABASE"});
  }
  return teams;
}

QuickMatchSetupState BuildQuickMatchSetupState() {
  QuickMatchSetupState state;
  state.teams = LoadTeamChoices();

  auto findTeam = [&state](int id) {
    for (std::size_t index = 0; index < state.teams.size(); ++index) {
      if (state.teams[index].id == id) {
        return index;
      }
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

const char* DifficultyName(int step) {
  switch (std::clamp(step, 0, 4)) {
    case 0:
      return "BEGINNER";
    case 1:
      return "AMATEUR";
    case 2:
      return "REGULAR";
    case 3:
      return "PROFESSIONAL";
    case 4:
    default:
      return "TOP PLAYER";
  }
}

void BindQuickMatchSetup(blunted::ui::RmlUiSystem& ui, const QuickMatchSetupState& state) {
  if (state.teams.size() < 2) {
    return;
  }

  const TeamChoice& home = state.Home();
  const TeamChoice& away = state.Away();
  ui.SetElementText("quick-home-crest", EscapeRmlText(TeamCrestLetter(home)));
  ui.SetElementText("quick-home-name", EscapeRmlText(home.name));
  ui.SetElementText("quick-home-meta",
                    EscapeRmlText("HOME" + (home.league.empty() ? std::string() : " · " + home.league)));
  ui.SetElementText("quick-away-crest", EscapeRmlText(TeamCrestLetter(away)));
  ui.SetElementText("quick-away-name", EscapeRmlText(away.name));
  ui.SetElementText("quick-away-meta",
                    EscapeRmlText("AWAY" + (away.league.empty() ? std::string() : " · " + away.league)));
  ui.SetElementText("quick-half-length", std::to_string(state.HalfLengthMinutes()) + " MIN");
  ui.SetElementText("quick-difficulty", DifficultyName(state.difficultyStep));
  ui.SetElementText("quick-control-side",
                    state.controlSide < 0 ? "KEYBOARD · HOME" : "KEYBOARD · AWAY");

  // The modern loading document reuses these ids so a final binding pass can
  // display the exact fixture selected on Match Setup.
  ui.SetElementText("loading-home-name", EscapeRmlText(home.name));
  ui.SetElementText("loading-away-name", EscapeRmlText(away.name));
  ui.SetElementText("loading-home-crest", EscapeRmlText(TeamCrestLetter(home)));
  ui.SetElementText("loading-away-crest", EscapeRmlText(TeamCrestLetter(away)));
}

void AdvanceTeam(QuickMatchSetupState& state, bool home) {
  if (state.teams.size() < 2) {
    return;
  }
  std::size_t& index = home ? state.homeIndex : state.awayIndex;
  const std::size_t other = home ? state.awayIndex : state.homeIndex;
  do {
    index = (index + 1) % state.teams.size();
  } while (index == other && state.teams.size() > 1);
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

PreviewExitAction ConsumePendingAction(blunted::ui::RmlUiSystem& ui,
                                       QuickMatchSetupState& quickMatch) {
  const std::string action = ui.ConsumeActionRequest();
  if (action.empty()) {
    return PreviewExitAction::None;
  }
  if (action == "start-quick-match") {
    std::fprintf(stdout,
                 "Fotbiler UI Preview: handing START MATCH to gameplayfootball runtime.\n");
    return PreviewExitAction::LaunchQuickMatch;
  }
  if (action == "start-career-match") {
    std::fprintf(stdout,
                 "Fotbiler UI Preview: handing CAREER MATCH to gameplayfootball runtime.\n");
    return PreviewExitAction::LaunchCareerMatch;
  }
  if (action == "change-home-team") {
    AdvanceTeam(quickMatch, true);
    BindQuickMatchSetup(ui, quickMatch);
    return PreviewExitAction::None;
  }
  if (action == "change-away-team") {
    AdvanceTeam(quickMatch, false);
    BindQuickMatchSetup(ui, quickMatch);
    return PreviewExitAction::None;
  }
  if (action == "cycle-half-length") {
    quickMatch.halfLengthIndex =
        (quickMatch.halfLengthIndex + 1) % quickMatch.halfLengths.size();
    BindQuickMatchSetup(ui, quickMatch);
    return PreviewExitAction::None;
  }
  if (action == "cycle-difficulty") {
    quickMatch.difficultyStep = (quickMatch.difficultyStep + 1) % 5;
    BindQuickMatchSetup(ui, quickMatch);
    return PreviewExitAction::None;
  }
  if (action == "toggle-control-side") {
    quickMatch.controlSide = quickMatch.controlSide < 0 ? 1 : -1;
    BindQuickMatchSetup(ui, quickMatch);
    return PreviewExitAction::None;
  }

  std::fprintf(stdout, "Fotbiler UI Preview: runtime action '%s' is not wired.\n",
               action.c_str());
  return PreviewExitAction::None;
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

int LaunchGameplayFootball(const std::string& executablePath, PreviewExitAction action,
                           const QuickMatchSetupState& quickMatch) {
  if (executablePath.empty() || !std::filesystem::exists(executablePath)) {
    std::fprintf(stderr, "Fotbiler UI Preview: gameplay executable not found at %s\n",
                 executablePath.c_str());
    return 1;
  }

  if (action == PreviewExitAction::LaunchCareerMatch) {
    SDL_setenv("FOTBILER_UI_CAREER_MATCH", "1", 1);
    SDL_setenv("FOTBILER_UI_QUICK_MATCH", "0", 1);
  } else {
    SDL_setenv("FOTBILER_UI_QUICK_MATCH", "1", 1);
    SDL_setenv("FOTBILER_UI_CAREER_MATCH", "0", 1);
    SetEnvInt("FOTBILER_UI_HOME_TEAM_ID", quickMatch.Home().id);
    SetEnvInt("FOTBILER_UI_AWAY_TEAM_ID", quickMatch.Away().id);
    SetEnvInt("FOTBILER_UI_MATCH_DURATION_MINUTES", quickMatch.MatchLengthMinutes());
    SetEnvFloat("FOTBILER_UI_MATCH_DIFFICULTY", quickMatch.Difficulty());
    SetEnvInt("FOTBILER_UI_CONTROL_SIDE", quickMatch.controlSide);
  }

  const std::string command = "\"" + executablePath + "\"";
  std::fprintf(stdout, "Fotbiler UI Preview: launching %s\n", executablePath.c_str());
  const int result = std::system(command.c_str());
  if (result == -1) {
    std::fprintf(stderr, "Fotbiler UI Preview: failed to launch gameplayfootball.\n");
    return 1;
  }
  return result;
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
  QuickMatchSetupState quickMatch = BuildQuickMatchSetupState();

  blunted::ui::ScreenRouter router(
      [&ui, &careerView, &detailView, &quickMatch](const std::string& path) {
        if (!LoadPreviewDocument(ui, path)) {
          return false;
        }
        blunted::ui::BindCareerUiViewModel(ui, careerView);
        blunted::ui::BindCareerDetailViewModel(ui, detailView);
        BindQuickMatchSetup(ui, quickMatch);
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
  PreviewExitAction exitAction = PreviewExitAction::None;
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

      const PreviewExitAction action = ConsumePendingAction(ui, quickMatch);
      if (action != PreviewExitAction::None) {
        exitAction = action;
        running = false;
        break;
      }
    }

    if (!running) {
      break;
    }

    glClearColor(0.035f, 0.055f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ui.Update();
    ui.Render();
    SDL_GL_SwapWindow(window);
  }

  const std::string gameplayExecutable =
      exitAction != PreviewExitAction::None ? GetGameplayExecutablePath() : std::string();

  if (controller) {
    SDL_GameControllerClose(controller);
  }
  ui.Shutdown();
  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();

  if (exitAction != PreviewExitAction::None) {
    return LaunchGameplayFootball(gameplayExecutable, exitAction, quickMatch);
  }
  return 0;
}
