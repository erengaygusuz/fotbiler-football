#include "menutask.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "../onthepitch/match.hpp"
#include "career/career_database.hpp"
#include "data/careerdata.hpp"
#include "framework/scheduler.hpp"
#include "gametask.hpp"
#include "ingame/gameover.hpp"
#include "ingame/ingame.hpp"
#include "ingame/phasemenu.hpp"
#include "ingame/replaymenu.hpp"
#include "main.hpp"
#include "mainmenu.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "pagefactory.hpp"
#include "presentation/ui/rmlui/runtime_ui_bridge.hpp"
#include "scene/scene2d/scene2d.hpp"
#include "systems/audio/rendering/interface_audiorenderer.hpp"
#include "systems/graphics/rendering/interface_renderer3d.hpp"
#include "utils/database.hpp"
#include "visualoptions.hpp"

using namespace blunted;

namespace {

class FotbilerPipelineFence final : public Command {
public:
  explicit FotbilerPipelineFence(const std::string& name) : Command(name) {}

protected:
  bool Execute(void* = nullptr) override { return true; }
};

void DrainThreadQueue(Thread* thread, const std::string& fenceName) {
  if (!thread) return;
  boost::intrusive_ptr<FotbilerPipelineFence> fence(new FotbilerPipelineFence(fenceName));
  thread->messageQueue.PushMessage(fence);
  fence->Wait();
}

bool EnvironmentFlagEnabled(const char* name) {
  const char* value = std::getenv(name);
  return value && value[0] != '\0' && std::string(value) != "0";
}

bool ModernFrontendAppActive() {
  return EnvironmentFlagEnabled("FOTBILER_UI_MODERN_APP");
}

int EnvironmentInt(const char* name, int fallback) {
  const char* value = std::getenv(name);
  if (!value || value[0] == '\0') return fallback;
  char* end = nullptr;
  const long parsed = std::strtol(value, &end, 10);
  return end && *end == '\0' ? static_cast<int>(parsed) : fallback;
}

float EnvironmentFloat(const char* name, float fallback) {
  const char* value = std::getenv(name);
  if (!value || value[0] == '\0') return fallback;
  char* end = nullptr;
  const float parsed = std::strtof(value, &end);
  return end && *end == '\0' ? parsed : fallback;
}

bool DatabaseHasTeam(int teamID) {
  if (!GetDB() || teamID <= 0) return false;
  auto result = GetDB()->Query("select id from teams where id = " + std::to_string(teamID) +
                               " limit 1");
  return result && !result->data.empty();
}

int FirstDatabaseTeamID() {
  if (!GetDB()) return 0;
  auto result = GetDB()->Query("select id from teams order by id limit 1");
  if (!result || result->data.empty() || result->data.front().empty()) return 0;
  return std::atoi(result->data.front().front().c_str());
}

std::string DatabaseTeamName(int teamID) {
  if (!GetDB() || teamID <= 0) return "Opponent";
  auto result = GetDB()->Query("select name from teams where id = " + std::to_string(teamID) +
                               " limit 1");
  if (result && !result->data.empty() && !result->data.front().empty()) {
    return result->data.front().front();
  }
  return "Opponent";
}

std::string DatabaseTeamLeague(int teamID) {
  if (!GetDB() || teamID <= 0) return "Default League";
  auto result = GetDB()->Query(
      "select coalesce(leagues.name, 'Default League') from teams "
      "left join leagues on teams.league_id = leagues.id where teams.id = " +
      std::to_string(teamID) + " limit 1");
  if (result && !result->data.empty() && !result->data.front().empty() &&
      !result->data.front().front().empty()) {
    return result->data.front().front();
  }
  return "Default League";
}

bool FindCareerOpponent(int userTeamID, int currentWeek, int& opponentTeamID,
                        std::string& opponentName) {
  if (!GetDB() || userTeamID <= 0) return false;

  auto result = GetDB()->Query(
      "select id, name from teams where id <> " + std::to_string(userTeamID) +
      " and league_id = (select league_id from teams where id = " +
      std::to_string(userTeamID) + ") order by id");
  if (!result || result->data.empty()) {
    result = GetDB()->Query("select id, name from teams where id <> " +
                            std::to_string(userTeamID) + " order by id");
  }
  if (!result || result->data.empty()) return false;

  const std::size_t index =
      static_cast<std::size_t>(std::max(0, currentWeek - 1)) % result->data.size();
  if (result->data[index].size() < 2) return false;
  opponentTeamID = std::atoi(result->data[index][0].c_str());
  opponentName = result->data[index][1];
  return opponentTeamID > 0;
}

bool EnsureModernUiCareerSave(CareerDatabase& career) {
  if (career.GetActiveSave()) return true;
  if (career.LoadCareerSlot(-1) || career.LoadCareerSlot(0)) return true;

  int teamID = DatabaseHasTeam(3) ? 3 : FirstDatabaseTeamID();
  if (teamID <= 0 || !DatabaseHasTeam(teamID)) {
    std::fprintf(stderr, "[fotbiler-ui] Career Match: database has no usable team\n");
    return false;
  }

  if (!career.CreateNewCareer("FOTBILER FC", "manager", "EREN")) {
    std::fprintf(stderr, "[fotbiler-ui] Career Match: could not create integration career save\n");
    return false;
  }

  CareerSave* save = career.GetActiveSave();
  if (!save) return false;
  save->club.clubID = teamID;
  save->club.clubName = "FOTBILER FC";
  save->club.leagueName = DatabaseTeamLeague(teamID);
  save->club.stadiumName = "FOTBILER STADIUM";
  save->stadium.name = "FOTBILER STADIUM";
  save->season.currentWeek = std::max(1, save->season.currentWeek);
  save->season.maxWeeks = std::max(1, save->season.maxWeeks);
  save->season.inPreseason = false;

  const bool primarySaved = career.SaveCareerData();
  const bool autoSaved = career.AutoSave();
  if (!primarySaved && !autoSaved) {
    std::fprintf(stderr, "[fotbiler-ui] Career Match: integration career could not be persisted\n");
    return false;
  }

  std::printf("[fotbiler-ui] Career Match: bootstrapped persisted modern career for team %d\n",
              teamID);
  return true;
}

}  // namespace

void SetActiveController(int side, bool keyboard) {
  bool keyboardActive = true;
  const std::vector<SideSelection> sides = GetMenuTask()->GetControllerSetup();
  const std::vector<IHIDevice*>& controllers = GetControllers();
  int menuControllerID = -1;
  for (unsigned int i = 0; i < sides.size(); i++) {
    if (sides.at(i).side == side) {
      int controllerID = sides.at(i).controllerID;
      if (controllerID >= 0 && controllerID < static_cast<int>(controllers.size()) &&
          controllers.at(controllerID)->GetDeviceType() == e_HIDeviceType_Gamepad) {
        menuControllerID = static_cast<HIDGamepad*>(controllers.at(controllerID))->GetGamepadID();
        keyboardActive = false;
      }
      break;
    }
    if (i == sides.size() - 1) menuControllerID = 0;
  }

  GetMenuTask()->SetActiveJoystickID(menuControllerID);
  if (keyboard) {
    if (keyboardActive)
      GetMenuTask()->EnableKeyboard();
    else
      GetMenuTask()->DisableKeyboard();
  } else {
    GetMenuTask()->EnableKeyboard();
  }
}

MenuTask::MenuTask(float aspectRatio, float margin, TTF_Font* defaultFont,
                   TTF_Font* defaultOutlineFont)
    : Gui2Task(GetScene2D(), aspectRatio, margin),
      menuAction(e_MenuAction_None),
      uiDirectMatchReady(false),
      frontendReturnPending(false),
      frontendReturnTarget(blunted::ui::frontend::ReturnTarget::MainMenu),
      menuBackground(nullptr) {
  Gui2Style* style = windowManager->GetStyle();

  windowManager->onSoundClick = [this]() {
    if (GetGameTask() && GetGameTask()->GetMenuScene()) GetGameTask()->GetMenuScene()->PlayClickSound();
  };
  windowManager->onSoundHover = [this]() {
    if (GetGameTask() && GetGameTask()->GetMenuScene()) GetGameTask()->GetMenuScene()->PlayHoverSound();
  };

  style->SetFont(e_TextType_Default, defaultFont);
  style->SetFont(e_TextType_DefaultOutline, defaultOutlineFont);
  style->SetFont(e_TextType_Caption, defaultFont);
  style->SetFont(e_TextType_Title, defaultFont);
  style->SetFont(e_TextType_ToolTip, defaultFont);

  style->SetColor(e_DecorationType_Dark1, Vector3(12, 22, 45));
  style->SetColor(e_DecorationType_Dark2, Vector3(45, 55, 80));
  style->SetColor(e_DecorationType_Bright1, Vector3(255, 255, 255));
  style->SetColor(e_DecorationType_Bright2, Vector3(255, 215, 0));
  style->SetColor(e_DecorationType_Toggled, Vector3(210, 40, 40));

  windowManager->SetTimeStep_ms(10);

  Gui2Root* root = windowManager->GetRoot();
  root->Show();

  menuBackground = new Gui2Image(windowManager, "image_menu_background", 0, 0, 100, 100);
  menuBackground->LoadImage("media/menu/backgrounds/stadium01.png");
  root->AddView(menuBackground);
  if (ModernFrontendAppActive())
    menuBackground->Hide();
  else
    menuBackground->Show();

  PageFactory* pageFactory = new PageFactory();
  windowManager->SetPageFactory(pageFactory);

  uiDirectMatchReady = PrepareFotbilerUiDirectMatch();

  if (!QuickStart()) {
    queuedFixture->team1KitNum = 1;
    queuedFixture->team2KitNum = 2;
    menuAction = e_MenuAction_Menu;
  } else if (!uiDirectMatchReady) {
    int size = GetControllers().size();
    for (int i = 0; i < size; i++) {
      SideSelection side;
      side.controllerID = i;
      if ((size > 1 && i == 1) || (size == 1 && i == 0))
        side.side = -1;
      else
        side.side = 0;
      queuedFixture->sides.push_back(side);
    }

    queuedFixture->teamID1 = "3";
    queuedFixture->teamID2 = "8";
    queuedFixture->team1KitNum = 2;
    queuedFixture->team2KitNum = 2;
    menuAction = e_MenuAction_Menu;
  } else {
    menuAction = e_MenuAction_Menu;
  }
}

MenuTask::~MenuTask() {
  if (Verbose()) printf("exiting menutask.. ");
  delete windowManager->GetPageFactory();
  if (Verbose()) printf("done\n");
}

void MenuTask::SetSingleControlledSide(int side) {
  const int normalizedSide = side > 0 ? 1 : -1;
  const int controllerCount = static_cast<int>(GetControllers().size());
  std::vector<SideSelection> sides;
  const int count = std::max(1, controllerCount);
  sides.reserve(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    SideSelection selection;
    selection.controllerID = i;
    selection.side = i == 0 ? normalizedSide : 0;
    sides.push_back(selection);
  }
  SetControllerSetup(sides);
}

bool MenuTask::PrepareFotbilerUiQuickMatch(const blunted::ui::frontend::LaunchRequest& request) {
  int homeTeamID = request.homeTeamId;
  int awayTeamID = request.awayTeamId;
  if (!DatabaseHasTeam(homeTeamID)) homeTeamID = DatabaseHasTeam(3) ? 3 : FirstDatabaseTeamID();
  if (!DatabaseHasTeam(awayTeamID) || awayTeamID == homeTeamID) {
    awayTeamID = DatabaseHasTeam(8) && homeTeamID != 8 ? 8 : 0;
  }
  if (awayTeamID == homeTeamID || !DatabaseHasTeam(awayTeamID)) {
    std::unique_ptr<DatabaseResult> result;
    if (GetDB()) {
      result = GetDB()->Query("select id from teams where id <> " + std::to_string(homeTeamID) +
                              " order by id limit 1");
    }
    if (!result || result->data.empty() || result->data.front().empty()) return false;
    awayTeamID = std::atoi(result->data.front().front().c_str());
  }

  SetTeamIDs(std::to_string(homeTeamID), std::to_string(awayTeamID));
  queuedFixture->team1KitNum = 1;
  queuedFixture->team2KitNum = 2;
  SetSingleControlledSide(request.controlSide);

  const int duration = std::clamp(request.matchDurationMinutes, 5, 90);
  const float difficulty = std::clamp(request.difficulty, 0.0f, 1.0f);
  GetConfiguration()->Set("match_duration_minutes", static_cast<float>(duration));
  GetConfiguration()->Set("match_difficulty", difficulty);

  std::printf("[fotbiler-ui] Quick Match: team %d vs %d, %d min, difficulty %.2f, side %d\n",
              homeTeamID, awayTeamID, duration, difficulty, request.controlSide);
  return true;
}

bool MenuTask::PrepareFotbilerUiQuickMatch() {
  blunted::ui::frontend::LaunchRequest request;
  request.kind = blunted::ui::frontend::LaunchKind::QuickMatch;
  request.homeTeamId = EnvironmentInt("FOTBILER_UI_HOME_TEAM_ID", 3);
  request.awayTeamId = EnvironmentInt("FOTBILER_UI_AWAY_TEAM_ID", 8);
  request.matchDurationMinutes = EnvironmentInt("FOTBILER_UI_MATCH_DURATION_MINUTES", 10);
  request.difficulty = EnvironmentFloat("FOTBILER_UI_MATCH_DIFFICULTY", 0.75f);
  request.controlSide = EnvironmentInt("FOTBILER_UI_CONTROL_SIDE", -1);
  return PrepareFotbilerUiQuickMatch(request);
}

bool MenuTask::PrepareFotbilerUiCareerMatch() {
  CareerDatabase& career = CareerDatabase::GetInstance();
  career.Initialize("user/career");
  if (!EnsureModernUiCareerSave(career)) {
    std::fprintf(stderr, "[fotbiler-ui] Career Match: no usable career save is available\n");
    return false;
  }

  CareerSave* save = career.GetActiveSave();
  if (!save) return false;

  if (save->club.clubID <= 0 || !DatabaseHasTeam(save->club.clubID)) {
    const int repairedTeamID = DatabaseHasTeam(3) ? 3 : FirstDatabaseTeamID();
    if (repairedTeamID <= 0 || !DatabaseHasTeam(repairedTeamID)) {
      std::fprintf(stderr, "[fotbiler-ui] Career Match: active club has no valid database team\n");
      return false;
    }
    std::fprintf(stderr,
                 "[fotbiler-ui] Career Match: repairing invalid club id %d to database team %d\n",
                 save->club.clubID, repairedTeamID);
    save->club.clubID = repairedTeamID;
    if (save->club.clubName.empty()) save->club.clubName = DatabaseTeamName(repairedTeamID);
    save->club.leagueName = DatabaseTeamLeague(repairedTeamID);
    career.SaveCareerData();
    career.AutoSave();
  }

  const int userTeamID = save->club.clubID;
  int opponentTeamID = 0;
  std::string opponentName;
  bool isHome = (save->season.currentWeek % 2) == 0;
  bool foundStoredFixture = false;

  for (const FixtureResult& fixture : save->season.fixtures) {
    if (fixture.played) continue;
    if (fixture.homeTeamID == userTeamID && fixture.awayTeamID > 0) {
      isHome = true;
      opponentTeamID = fixture.awayTeamID;
      foundStoredFixture = true;
      break;
    }
    if (fixture.awayTeamID == userTeamID && fixture.homeTeamID > 0) {
      isHome = false;
      opponentTeamID = fixture.homeTeamID;
      foundStoredFixture = true;
      break;
    }
  }

  if (opponentTeamID <= 0 || !DatabaseHasTeam(opponentTeamID)) {
    if (!FindCareerOpponent(userTeamID, save->season.currentWeek, opponentTeamID, opponentName)) {
      std::fprintf(stderr, "[fotbiler-ui] Career Match: could not resolve an opponent\n");
      return false;
    }
    foundStoredFixture = false;
  }
  if (opponentName.empty()) opponentName = DatabaseTeamName(opponentTeamID);

  if (!foundStoredFixture) {
    FixtureResult fixture;
    fixture.fixtureID = save->season.currentWeek * 100;
    fixture.homeTeamID = isHome ? userTeamID : opponentTeamID;
    fixture.awayTeamID = isHome ? opponentTeamID : userTeamID;
    fixture.played = false;
    save->season.fixtures.push_back(fixture);
    career.SaveCareerData();
    career.AutoSave();
  }

  career.SetPendingFixture(isHome, userTeamID, opponentTeamID, opponentName);
  if (isHome) {
    SetTeamIDs(std::to_string(userTeamID), std::to_string(opponentTeamID));
    SetSingleControlledSide(-1);
  } else {
    SetTeamIDs(std::to_string(opponentTeamID), std::to_string(userTeamID));
    SetSingleControlledSide(1);
  }
  queuedFixture->team1KitNum = 1;
  queuedFixture->team2KitNum = 2;
  GetConfiguration()->SetBool("career_resume_hub", false);

  std::printf("[fotbiler-ui] Career Match: %s team %d vs team %d (%s)\n",
              isHome ? "home" : "away", userTeamID, opponentTeamID, opponentName.c_str());
  return true;
}

bool MenuTask::PrepareFotbilerUiDirectMatch() {
  bool ready = false;
  if (EnvironmentFlagEnabled("FOTBILER_UI_CAREER_MATCH"))
    ready = PrepareFotbilerUiCareerMatch();
  else if (EnvironmentFlagEnabled("FOTBILER_UI_QUICK_MATCH"))
    ready = PrepareFotbilerUiQuickMatch();

  if (ready) {
    SDL_setenv("FOTBILER_UI_QUICK_MATCH", "0", 1);
    SDL_setenv("FOTBILER_UI_CAREER_MATCH", "0", 1);
  }
  return ready;
}

void MenuTask::ReturnToFotbilerFrontend(blunted::ui::frontend::ReturnTarget target) {
  frontendReturnTarget = target;
  frontendReturnPending = true;
  menuAction = e_MenuAction_Menu;
}

void MenuTask::DrainFotbilerRuntimePipelines() {
  // Match::Exit() enqueues renderer-side destruction work (including crowd
  // audio source deletion). A fence placed after those commands proves they
  // have actually executed before the frontend becomes visible again.
  if (GetAudioSystem()) {
    DrainThreadQueue(GetAudioSystem()->GetAudioRenderer(), "fotbiler_audio_teardown_fence");
  }
  if (GetGraphicsSystem()) {
    DrainThreadQueue(GetGraphicsSystem()->GetRenderer3D(), "fotbiler_graphics_teardown_fence");
  }
}

void MenuTask::ApplyFotbilerDisplaySettings(
    const blunted::ui::frontend::DisplaySettingsRequest& request) {
  GraphicsSystem* graphics = GetGraphicsSystem();
  if (!graphics) return;

  if (!graphics->ApplyDisplaySettings(request.width, request.height, request.fullscreen,
                                      request.vsync)) {
    std::fprintf(stderr, "[fotbiler-ui] failed to apply display settings %dx%d\n",
                 request.width, request.height);
    return;
  }

  int width = 0;
  int height = 0;
  int bpp = 0;
  graphics->GetContextSize(width, height, bpp);
  GetConfiguration()->SetInt("context_x", width);
  GetConfiguration()->SetInt("context_y", height);
  GetConfiguration()->SetInt("context_bpp", bpp);
  GetConfiguration()->SetBool("context_fullscreen", graphics->IsFullscreen());
  GetConfiguration()->SetBool("context_vsync", request.vsync);
  GetConfiguration()->Set("match_difficulty",
                          static_cast<float>(std::clamp(request.difficultyStep, 0, 4)) * 0.25f);
  GetConfiguration()->SetInt("fotbiler_game_speed", request.gameSpeedStep);
  GetConfiguration()->SetInt("fotbiler_master_volume", request.volume);

  if (GetScene2D()) GetScene2D()->SetContextSize(width, height, bpp);

  std::printf("[fotbiler-ui] live display active: %dx%d %s, vsync=%d\n", width, height,
              graphics->IsFullscreen() ? "fullscreen" : "windowed", request.vsync ? 1 : 0);
}

void MenuTask::ProcessPhase() {
  Gui2Task::ProcessPhase();

  if (ModernFrontendAppActive()) {
    if (blunted::ui::frontend::ConsumeQuitRequest()) {
      QuitGame();
      return;
    }

    blunted::ui::frontend::DisplaySettingsRequest displayRequest;
    if (blunted::ui::frontend::ConsumeDisplaySettings(displayRequest)) {
      ApplyFotbilerDisplaySettings(displayRequest);
    }

    blunted::ui::frontend::LaunchRequest request;
    if (blunted::ui::frontend::ConsumeLaunchRequest(request)) {
      bool ready = false;
      if (request.kind == blunted::ui::frontend::LaunchKind::Career)
        ready = PrepareFotbilerUiCareerMatch();
      else if (request.kind == blunted::ui::frontend::LaunchKind::QuickMatch)
        ready = PrepareFotbilerUiQuickMatch(request);

      if (ready) {
        uiDirectMatchReady = true;
        menuAction = e_MenuAction_Menu;
      } else {
        uiDirectMatchReady = false;
        blunted::ui::frontend::ReturnToFrontend(blunted::ui::frontend::GetReturnTarget());
      }
    }
  }

  if (menuAction == e_MenuAction_Menu) {
    windowManager->GetPagePath()->Clear();

    GetGameTask()->Action(e_GameTaskMessage_StopMatch);
    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);

    if (ModernFrontendAppActive()) {
      menuBackground->Hide();
      GetConfiguration()->SetBool("career_resume_hub", false);
      GetConfiguration()->SetBool("league_resume_hub", false);

      if (frontendReturnPending) {
        DrainFotbilerRuntimePipelines();
        blunted::ui::runtime::Reset();
        ReleaseAllButtons();
        blunted::ui::frontend::ReturnToFrontend(frontendReturnTarget);
        frontendReturnPending = false;
      } else if (uiDirectMatchReady) {
        Properties properties;
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_LoadingMatch, properties, 0);
        uiDirectMatchReady = false;
      }
    } else {
      menuBackground->Show();
      Properties properties;
      if (GetConfiguration()->GetBool("league_resume_hub", false)) {
        GetConfiguration()->SetBool("league_resume_hub", false);
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_League_Matchday, properties, 0);
      } else if (GetConfiguration()->GetBool("career_resume_hub", false)) {
        GetConfiguration()->SetBool("career_resume_hub", false);
        CareerSave* save = CareerDatabase::GetInstance().GetActiveSave();
        const int hubPage = (save && save->mode == CareerMode::OWNER) ? (int)e_PageID_OwnerHub
                                                                      : (int)e_PageID_CareerHub;
        windowManager->GetPageFactory()->CreatePage(hubPage, properties, 0);
      } else if (!QuickStart()) {
        if (!IsReleaseVersion())
          windowManager->GetPageFactory()->CreatePage((int)e_PageID_MainMenu, properties, 0);
        else
          windowManager->GetPageFactory()->CreatePage((int)e_PageID_Intro, properties, 0);
      } else {
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_LoadingMatch, properties, 0);
        uiDirectMatchReady = false;
      }
    }

  } else if (menuAction == e_MenuAction_Game) {
    menuBackground->Hide();
    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);
    GetGameTask()->Action(e_GameTaskMessage_StartMatch);
  }

  menuAction = e_MenuAction_None;
}

bool MenuTask::QuickStart() {
  if (uiDirectMatchReady) return true;
  const bool developerQuickStart =
      !IsReleaseVersion() && GetConfiguration()->GetBool("quick_start", false);
  return developerQuickStart && EnvironmentManager::GetInstance().GetTime_ms() < 10000;
}

void MenuTask::QuitGame() {
  EnvironmentManager::GetInstance().SignalQuit();
}

void MenuTask::ReleaseAllButtons() {
  for (int joyID = 0; joyID < UserEventManager::GetInstance().GetJoystickCount(); joyID++) {
    for (unsigned int buttonID = 0; buttonID < blunted::_JOYSTICK_MAXBUTTONS; buttonID++) {
      UserEventManager::GetInstance().SetJoyButtonState(joyID, buttonID, false);
    }
  }
  UserEventManager::GetInstance().SetKeyboardState(SDLK_ESCAPE, false);
}
