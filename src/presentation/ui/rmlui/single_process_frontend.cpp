#include "single_process_frontend.hpp"

#include <SDL2/SDL.h>
#include <sqlite3.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "gamedefines.hpp"
#include "hid/keyboard.hpp"
#include "main.hpp"
#include "presentation/ui/rmlui/career_detail_binder.hpp"
#include "presentation/ui/rmlui/career_detail_view_model.hpp"
#include "presentation/ui/rmlui/career_ui_binder.hpp"
#include "presentation/ui/rmlui/career_ui_preview_data.hpp"
#include "presentation/ui/rmlui/career_ui_view_model.hpp"
#include "presentation/ui/rmlui/input_settings.hpp"
#include "presentation/ui/rmlui/rmlui_system.hpp"
#include "presentation/ui/rmlui/runtime_settings.hpp"
#include "utils/localization.hpp"

namespace blunted::ui {
namespace {

struct TeamChoice {
  int id = 0;
  std::string name;
  std::string league;
};

struct QuickMatchState {
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

std::string CrestLetter(const std::string& name) {
  for (char ch : name) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
      if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - ('a' - 'A'));
      return std::string(1, ch);
    }
  }
  return "F";
}

std::vector<TeamChoice> LoadTeams() {
  std::vector<TeamChoice> teams;
  sqlite3* db = nullptr;
  if (sqlite3_open_v2("databases/default/database.sqlite", &db, SQLITE_OPEN_READONLY, nullptr) ==
      SQLITE_OK) {
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
    }
    if (statement) sqlite3_finalize(statement);
  }
  if (db) sqlite3_close(db);
  if (teams.size() < 2) {
    teams = {{3, "BARCELONA", "DEFAULT DATABASE"}, {8, "REAL MADRID", "DEFAULT DATABASE"}};
  }
  return teams;
}

QuickMatchState BuildQuickMatchState() {
  QuickMatchState state;
  state.teams = LoadTeams();
  auto findTeam = [&state](int id) {
    for (std::size_t i = 0; i < state.teams.size(); ++i) {
      if (state.teams[i].id == id) return i;
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

void AdvanceTeam(QuickMatchState& state, bool home) {
  if (state.teams.size() < 2) return;
  std::size_t& selected = home ? state.homeIndex : state.awayIndex;
  const std::size_t other = home ? state.awayIndex : state.homeIndex;
  do {
    selected = (selected + 1) % state.teams.size();
  } while (selected == other && state.teams.size() > 1);
}

void SendNavigationKey(RmlUiSystem& ui, SDL_Keycode key) {
  SDL_Event event{};
  event.type = SDL_KEYDOWN;
  event.key.type = SDL_KEYDOWN;
  event.key.state = SDL_PRESSED;
  event.key.repeat = 0;
  event.key.keysym.sym = key;
  event.key.keysym.scancode = SDL_GetScancodeFromKey(key);
  ui.HandleEvent(event);
}

HIDKeyboard* FindKeyboardController() {
  for (IHIDevice* controller : GetControllers()) {
    if (controller && controller->GetDeviceType() == e_HIDeviceType_Keyboard) {
      return static_cast<HIDKeyboard*>(controller);
    }
  }
  return nullptr;
}

int ConnectedGamepadCount() {
  int count = 0;
  for (IHIDevice* controller : GetControllers()) {
    if (controller && controller->GetDeviceType() == e_HIDeviceType_Gamepad) ++count;
  }
  return count;
}

std::string KeyName(SDL_Keycode key) {
  const char* name = SDL_GetKeyName(key);
  return name && name[0] != '\0' ? std::string(name) : "UNBOUND";
}

}  // namespace

struct SingleProcessFrontend::Impl {
  explicit Impl(RmlUiSystem& system)
      : ui(system),
        quickMatch(BuildQuickMatchState()),
        settings(LoadRuntimeSettings()),
        careerSave(BuildCareerPreviewSave()),
        careerView(BuildCareerUiViewModel(careerSave)),
        detailView(BuildCareerDetailViewModel(careerSave)),
        router([this](const std::string& path) { return LoadDocument(path); }) {
    quickMatch.difficultyStep = settings.difficultyStep;
  }

  bool LoadDocument(const std::string& path) {
    if (!ui.LoadDocument(path)) {
      std::fprintf(stderr, "[fotbiler-ui] could not load frontend document %s\n", path.c_str());
      return false;
    }
    BindAll();
    ui.FocusDefaultElement();
    return true;
  }

  void BindRuntimeSettings() {
    ui.SetElementText("settings-window-mode", settings.fullscreen ? "FULLSCREEN" : "WINDOWED");
    ui.SetElementText("settings-resolution",
                      std::to_string(settings.Width()) + " × " + std::to_string(settings.Height()));
    ui.SetElementText("settings-vsync", settings.vsync ? "ON" : "OFF");
    ui.SetElementText("settings-difficulty", RuntimeDifficultyName(settings.difficultyStep));
    ui.SetElementText("settings-game-speed", RuntimeGameSpeedName(settings.gameSpeedStep));
    ui.SetElementText("settings-volume", std::to_string(settings.volume) + "%");
    const std::string language = Localization::GetInstance().GetCurrentLanguage();
    ui.SetElementText("settings-language", Localization::GetLanguageDisplayName(language));
  }

  void BindControlsSettings() {
    HIDKeyboard* keyboard = FindKeyboardController();
    for (std::size_t i = 0; i < kKeyboardBindingLabels.size(); ++i) {
      const std::string elementId = "control-key-" + std::to_string(i);
      ui.SetElementText(elementId,
                        keyboard ? KeyName(keyboard->GetFunctionMapping(
                                       static_cast<e_ButtonFunction>(i)))
                                 : "NOT FOUND");
    }

    const int gamepads = ConnectedGamepadCount();
    ui.SetElementText("controls-gamepad-status",
                      gamepads == 0 ? "GAMEPADS · NONE DETECTED"
                                    : "GAMEPADS · " + std::to_string(gamepads) + " DETECTED");

    if (!capturingKeyIndex) {
      ui.SetElementText("controls-capture-status",
                        keyboard ? "Select an action, then press the new key. Changes are saved immediately."
                                 : "No keyboard input device is available.");
    }
  }

  void BindAll() {
    BindCareerUiViewModel(ui, careerView);
    BindCareerDetailViewModel(ui, detailView);
    BindQuickMatch();
    BindRuntimeSettings();
    BindControlsSettings();
  }

  void BindQuickMatch() {
    if (quickMatch.teams.size() < 2) return;
    const TeamChoice& home = quickMatch.Home();
    const TeamChoice& away = quickMatch.Away();
    ui.SetElementText("quick-home-crest", CrestLetter(home.name));
    ui.SetElementText("quick-home-name", home.name);
    ui.SetElementText("quick-home-meta",
                      "HOME" + (home.league.empty() ? std::string() : " · " + home.league));
    ui.SetElementText("quick-away-crest", CrestLetter(away.name));
    ui.SetElementText("quick-away-name", away.name);
    ui.SetElementText("quick-away-meta",
                      "AWAY" + (away.league.empty() ? std::string() : " · " + away.league));
    ui.SetElementText("quick-half-length", std::to_string(quickMatch.HalfLengthMinutes()) + " MIN");
    ui.SetElementText("quick-difficulty", RuntimeDifficultyName(quickMatch.difficultyStep));
    ui.SetElementText("quick-control-side",
                      quickMatch.controlSide < 0 ? "KEYBOARD · HOME" : "KEYBOARD · AWAY");
    ui.SetElementText("loading-home-name", home.name);
    ui.SetElementText("loading-away-name", away.name);
    ui.SetElementText("loading-home-crest", CrestLetter(home.name));
    ui.SetElementText("loading-away-crest", CrestLetter(away.name));
  }

  void BindCareerLoading() {
    const std::string clubName =
        careerView.header.clubName.empty() ? "CAREER CLUB" : careerView.header.clubName;
    ui.SetElementText("loading-mode", "CAREER MODE · MATCHDAY");
    ui.SetElementText("loading-kicker", "CAREER FIXTURE");
    ui.SetElementText("loading-title", "PREPARING CAREER MATCH");
    ui.SetElementText("loading-home-name", clubName);
    ui.SetElementText("loading-home-crest", CrestLetter(clubName));
    ui.SetElementText("loading-away-name", "NEXT LEAGUE OPPONENT");
    ui.SetElementText("loading-away-crest", "A");
  }

  void ShowQuitModal() {
    if (ui.SetElementProperty("quit-confirm-overlay", "display", "block")) {
      quitConfirmOpen = true;
      ui.FocusElement("quit-confirm-cancel");
    }
  }

  void HideQuitModal() {
    ui.SetElementProperty("quit-confirm-overlay", "display", "none");
    quitConfirmOpen = false;
    if (!ui.FocusElement("tile-career")) ui.FocusDefaultElement();
  }

  void ApplyRuntimeSettings() {
    SaveRuntimeSettings(settings);

    frontend::DisplaySettingsRequest request;
    request.width = settings.Width();
    request.height = settings.Height();
    request.fullscreen = settings.fullscreen;
    request.vsync = settings.vsync;
    request.difficultyStep = settings.difficultyStep;
    request.gameSpeedStep = settings.gameSpeedStep;
    request.volume = settings.volume;
    frontend::PublishDisplaySettings(request);

    std::fprintf(stdout,
                 "[fotbiler-ui] display settings requested: %dx%d %s, vsync=%d\n",
                 request.width, request.height, request.fullscreen ? "fullscreen" : "windowed",
                 request.vsync ? 1 : 0);
  }

  void BeginKeyboardCapture(std::size_t index) {
    if (!FindKeyboardController() || index >= kKeyboardBindingLabels.size()) return;
    capturingKeyIndex = index;
    ui.SetElementText("controls-capture-status",
                      "PRESS A KEY FOR " + std::string(kKeyboardBindingLabels[index]) +
                          " · ESC CANCELS");
  }

  void CancelKeyboardCapture() {
    capturingKeyIndex.reset();
    BindControlsSettings();
  }

  void CommitKeyboardCapture(SDL_Keycode key) {
    if (!capturingKeyIndex) return;
    HIDKeyboard* keyboard = FindKeyboardController();
    if (!keyboard) {
      CancelKeyboardCapture();
      return;
    }
    keyboard->SetFunctionMapping(static_cast<int>(*capturingKeyIndex), key);
    keyboard->SaveConfig();
    capturingKeyIndex.reset();
    BindControlsSettings();
  }

  void ResetKeyboardBindings() {
    HIDKeyboard* keyboard = FindKeyboardController();
    if (!keyboard) return;
    for (std::size_t i = 0; i < kKeyboardBindingLabels.size(); ++i) {
      keyboard->SetFunctionMapping(static_cast<int>(i), defaultKeyIDs[i]);
    }
    keyboard->SaveConfig();
    capturingKeyIndex.reset();
    BindControlsSettings();
  }

  void CycleLanguage() {
    const std::vector<std::string> languages = Localization::GetAvailableLanguages();
    if (languages.empty()) return;

    const std::string current = Localization::GetInstance().GetCurrentLanguage();
    auto it = std::find(languages.begin(), languages.end(), current);
    const std::size_t currentIndex =
        it == languages.end() ? 0 : static_cast<std::size_t>(std::distance(languages.begin(), it));
    const std::string& next = languages[(currentIndex + 1) % languages.size()];
    if (!Localization::GetInstance().Load(next)) return;

    GetConfiguration()->Set("locale_language", next);
    GetConfiguration()->SaveFile(GetConfigFilename());
    BindRuntimeSettings();
  }

  void ProcessUiRequests() {
    if (!quitConfirmOpen) {
      const std::string route = ui.ConsumeRouteRequest();
      if (!route.empty() && !router.NavigateByName(route)) {
        std::fprintf(stderr, "[fotbiler-ui] unknown route %s\n", route.c_str());
      }
    } else {
      (void)ui.ConsumeRouteRequest();
    }

    const std::string action = ui.ConsumeActionRequest();
    if (action.empty()) return;

    if (action == "cancel-quit-game") {
      HideQuitModal();
      return;
    }
    if (action == "confirm-quit-game") {
      frontend::RequestQuit();
      return;
    }
    if (quitConfirmOpen) return;

    if (const auto keyIndex = ParseKeyboardBindingAction(action)) {
      BeginKeyboardCapture(*keyIndex);
      return;
    }

    if (action == "reset-keyboard-bindings") {
      ResetKeyboardBindings();
    } else if (action == "cycle-language") {
      CycleLanguage();
    } else if (action == "change-home-team") {
      AdvanceTeam(quickMatch, true);
      BindQuickMatch();
    } else if (action == "change-away-team") {
      AdvanceTeam(quickMatch, false);
      BindQuickMatch();
    } else if (action == "cycle-half-length") {
      quickMatch.halfLengthIndex = (quickMatch.halfLengthIndex + 1) % quickMatch.halfLengths.size();
      BindQuickMatch();
    } else if (action == "cycle-difficulty") {
      quickMatch.difficultyStep = (quickMatch.difficultyStep + 1) % 5;
      settings.difficultyStep = quickMatch.difficultyStep;
      BindQuickMatch();
      BindRuntimeSettings();
    } else if (action == "toggle-control-side") {
      quickMatch.controlSide = quickMatch.controlSide < 0 ? 1 : -1;
      BindQuickMatch();
    } else if (action == "toggle-fullscreen") {
      settings.fullscreen = !settings.fullscreen;
      BindRuntimeSettings();
    } else if (action == "cycle-resolution") {
      settings.resolutionIndex = (settings.resolutionIndex + 1) % settings.resolutions.size();
      BindRuntimeSettings();
    } else if (action == "toggle-vsync") {
      settings.vsync = !settings.vsync;
      BindRuntimeSettings();
    } else if (action == "cycle-game-speed") {
      settings.gameSpeedStep = (settings.gameSpeedStep + 1) % 3;
      BindRuntimeSettings();
    } else if (action == "cycle-audio-volume") {
      settings.volume = settings.volume >= 100 ? 0 : settings.volume + 10;
      BindRuntimeSettings();
    } else if (action == "apply-settings") {
      ApplyRuntimeSettings();
    } else if (action == "start-quick-match") {
      if (router.Navigate(ScreenId::MatchLoading)) {
        BindQuickMatch();
        ui.SetElementText("loading-mode", "QUICK MATCH · MATCH ENGINE");
        ui.SetElementText("loading-kicker", "KICK OFF");
        ui.SetElementText("loading-title", "PREPARING MATCH");
        frontend::PublishQuickMatchLaunch(quickMatch.Home().id, quickMatch.Away().id,
                                          quickMatch.MatchLengthMinutes(), quickMatch.Difficulty(),
                                          quickMatch.controlSide);
      }
    } else if (action == "start-career-match") {
      if (router.Navigate(ScreenId::MatchLoading)) {
        BindCareerLoading();
        frontend::PublishCareerLaunch();
      }
    }
  }

  bool Initialize() {
    if (initialized) return true;
    frontend::Reset();
    if (!router.Reset(ScreenId::MainMenu)) return false;
    initialized = true;
    suspended = false;
    return true;
  }

  void RefreshCareer() {
    careerSave = BuildCareerPreviewSave();
    careerView = BuildCareerUiViewModel(careerSave);
    detailView = BuildCareerDetailViewModel(careerSave);
  }

  bool Resume(frontend::ReturnTarget target) {
    RefreshCareer();
    suspended = false;
    switch (target) {
      case frontend::ReturnTarget::MatchSetup: return router.Reset(ScreenId::MatchSetup);
      case frontend::ReturnTarget::CareerCentral: return router.Reset(ScreenId::CareerCentral);
      case frontend::ReturnTarget::MainMenu: default: return router.Reset(ScreenId::MainMenu);
    }
  }

  bool HandleEvent(SDL_Event& event) {
    if (!initialized || suspended) return false;
    if (frontend::GetAppMode() == frontend::AppMode::Loading) {
      return event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ||
             event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP ||
             event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
             event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEWHEEL;
    }

    if (capturingKeyIndex) {
      if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        if (event.key.keysym.sym == SDLK_ESCAPE)
          CancelKeyboardCapture();
        else
          CommitKeyboardCapture(event.key.keysym.sym);
        return true;
      }
      if (event.type == SDL_CONTROLLERBUTTONDOWN && event.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
        CancelKeyboardCapture();
        return true;
      }
      if (event.type == SDL_KEYUP || event.type == SDL_CONTROLLERBUTTONUP) return true;
    }

    bool consumed = false;
    bool activate = false;

    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
      if (quitConfirmOpen) {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE: HideQuitModal(); consumed = true; break;
          case SDLK_LEFT:
          case SDLK_UP: ui.FocusElement("quit-confirm-cancel"); consumed = true; break;
          case SDLK_RIGHT:
          case SDLK_DOWN: ui.FocusElement("quit-confirm-accept"); consumed = true; break;
          case SDLK_RETURN:
          case SDLK_KP_ENTER: activate = true; consumed = true; break;
          default: break;
        }
      } else if (event.key.keysym.sym == SDLK_ESCAPE) {
        if (router.Current() && *router.Current() == ScreenId::MainMenu) ShowQuitModal();
        else if (!router.Back()) router.Reset(ScreenId::MainMenu);
        consumed = true;
      } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
        activate = true;
        consumed = true;
      }
    } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
      consumed = true;
      if (quitConfirmOpen) {
        switch (event.cbutton.button) {
          case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
          case SDL_CONTROLLER_BUTTON_DPAD_UP: ui.FocusElement("quit-confirm-cancel"); break;
          case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
          case SDL_CONTROLLER_BUTTON_DPAD_DOWN: ui.FocusElement("quit-confirm-accept"); break;
          case SDL_CONTROLLER_BUTTON_A: activate = true; break;
          case SDL_CONTROLLER_BUTTON_B: HideQuitModal(); break;
          default: break;
        }
      } else {
        switch (event.cbutton.button) {
          case SDL_CONTROLLER_BUTTON_DPAD_UP: SendNavigationKey(ui, SDLK_UP); break;
          case SDL_CONTROLLER_BUTTON_DPAD_DOWN: SendNavigationKey(ui, SDLK_DOWN); break;
          case SDL_CONTROLLER_BUTTON_DPAD_LEFT: SendNavigationKey(ui, SDLK_LEFT); break;
          case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: SendNavigationKey(ui, SDLK_RIGHT); break;
          case SDL_CONTROLLER_BUTTON_A: activate = true; break;
          case SDL_CONTROLLER_BUTTON_B:
            if (router.Current() && *router.Current() == ScreenId::MainMenu) ShowQuitModal();
            else if (!router.Back()) router.Reset(ScreenId::MainMenu);
            break;
          default: break;
        }
      }
    }

    if (!consumed) consumed = ui.HandleEvent(event);
    if (activate) ui.ActivateFocusedElement();
    ProcessUiRequests();
    return consumed;
  }

  RmlUiSystem& ui;
  QuickMatchState quickMatch;
  RuntimeSettings settings;
  CareerSave careerSave;
  CareerUiViewModel careerView;
  CareerDetailViewModel detailView;
  ScreenRouter router;
  bool initialized = false;
  bool suspended = false;
  bool quitConfirmOpen = false;
  std::optional<std::size_t> capturingKeyIndex;
};

SingleProcessFrontend::SingleProcessFrontend(RmlUiSystem& ui)
    : impl(std::make_unique<Impl>(ui)) {}

SingleProcessFrontend::~SingleProcessFrontend() = default;

bool SingleProcessFrontend::Initialize() { return impl->Initialize(); }

void SingleProcessFrontend::Shutdown() {
  if (!impl->initialized) return;
  impl->ui.UnloadDocument();
  impl->initialized = false;
  impl->suspended = false;
}

bool SingleProcessFrontend::HandleEvent(SDL_Event& event) { return impl->HandleEvent(event); }

bool SingleProcessFrontend::UpdateAndRender() {
  if (!impl->initialized || impl->suspended) return false;
  impl->ProcessUiRequests();
  return impl->ui.Update() && impl->ui.Render();
}

void SingleProcessFrontend::SuspendForMatch() {
  if (!impl->initialized || impl->suspended) return;
  impl->ui.UnloadDocument();
  impl->suspended = true;
}

bool SingleProcessFrontend::Resume(frontend::ReturnTarget target) {
  return impl->initialized && impl->Resume(target);
}

bool SingleProcessFrontend::IsInitialized() const { return impl->initialized; }

}  // namespace blunted::ui
