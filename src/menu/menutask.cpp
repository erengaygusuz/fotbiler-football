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
#include "utils/database.hpp"
#include "visualoptions.hpp"

using namespace blunted;

namespace {

bool EnvironmentFlagEnabled(const char* name) {
  const char* value = std::getenv(name);
  return value && value[0] != '\0' && std::string(value) != "0";
}

int EnvironmentInt(const char* name, int fallback) {
  const char* value = std::getenv(name);
  if (!value || value[0] == '\0') {
    return fallback;
  }
  char* end = nullptr;
  const long parsed = std::strtol(value, &end, 10);
  return end && *end == '\0' ? static_cast<int>(parsed) : fallback;
}

float EnvironmentFloat(const char* name, float fallback) {
  const char* value = std::getenv(name);
  if (!value || value[0] == '\0') {
    return fallback;
  }
  char* end = nullptr;
  const float parsed = std::strtof(value, &end);
  return end && *end == '\0' ? parsed : fallback;
}

bool DatabaseHasTeam(int teamID) {
  if (!GetDB() || teamID <= 0) {
    return false;
  }
  auto result = GetDB()->Query("select id from teams where id = " + std::to_string(teamID) +
                               " limit 1");
  return result && !result->data.empty();
}

std::string DatabaseTeamName(int teamID) {
  if (!GetDB() || teamID <= 0) {
    return "Opponent";
  }
  auto result = GetDB()->Query("select name from teams where id = " + std::to_string(teamID) +
                               " limit 1");
  if (result && !result->data.empty() && !result->data.front().empty()) {
    return result->data.front().front();
  }
  return "Opponent";
}

bool FindCareerOpponent(int userTeamID, int currentWeek, int& opponentTeamID,
                        std::string& opponentName) {
  if (!GetDB() || userTeamID <= 0) {
    return false;
  }

  auto result = GetDB()->Query(
      "select id, name from teams where id <> " + std::to_string(userTeamID) +
      " and league_id = (select league_id from teams where id = " +
      std::to_string(userTeamID) + ") order by id");
  if (!result || result->data.empty()) {
    result = GetDB()->Query("select id, name from teams where id <> " +
                            std::to_string(userTeamID) + " order by id");
  }
  if (!result || result->data.empty()) {
    return false;
  }

  const std::size_t index =
      static_cast<std::size_t>(std::max(0, currentWeek - 1)) % result->data.size();
  if (result->data[index].size() < 2) {
    return false;
  }
  opponentTeamID = std::atoi(result->data[index][0].c_str());
  opponentName = result->data[index][1];
  return opponentTeamID > 0;
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
    if (i == sides.size() - 1)
      menuControllerID = 0;
  }

  GetMenuTask()->SetActiveJoystickID(menuControllerID);
  if (keyboard) {
    if (keyboardActive) {
      GetMenuTask()->EnableKeyboard();
    } else {
      GetMenuTask()->DisableKeyboard();
    }
  } else {
    GetMenuTask()->EnableKeyboard();
  }
}

MenuTask::MenuTask(float aspectRatio, float margin, TTF_Font* defaultFont,
                   TTF_Font* defaultOutlineFont)
    : Gui2Task(GetScene2D(), aspectRatio, margin),
      menuAction(e_MenuAction_None),
      uiDirectMatchReady(false),
      menuBackground(nullptr) {
  Gui2Style* style = windowManager->GetStyle();

  windowManager->onSoundClick = [this]() {
    if (GetGameTask() && GetGameTask()->GetMenuScene()) {
      GetGameTask()->GetMenuScene()->PlayClickSound();
    }
  };
  windowManager->onSoundHover = [this]() {
    if (GetGameTask() && GetGameTask()->GetMenuScene()) {
      GetGameTask()->GetMenuScene()->PlayHoverSound();
    }
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
      if ((size > 1 && i == 1) || (size == 1 && i == 0)) {
        side.side = -1;
      } else {
        side.side = 0;
      }
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
  if (Verbose())
    printf("exiting menutask.. ");

  delete windowManager->GetPageFactory();

  if (Verbose())
    printf("done\n");
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

bool MenuTask::PrepareFotbilerUiQuickMatch() {
  int homeTeamID = EnvironmentInt("FOTBILER_UI_HOME_TEAM_ID", 3);
  int awayTeamID = EnvironmentInt("FOTBILER_UI_AWAY_TEAM_ID", 8);
  if (!DatabaseHasTeam(homeTeamID)) {
    homeTeamID = 3;
  }
  if (!DatabaseHasTeam(awayTeamID) || awayTeamID == homeTeamID) {
    awayTeamID = 8;
  }
  if (awayTeamID == homeTeamID || !DatabaseHasTeam(awayTeamID)) {
    std::unique_ptr<DatabaseResult> result;
    if (GetDB()) {
      result = GetDB()->Query("select id from teams where id <> " + std::to_string(homeTeamID) +
                              " order by id limit 1");
    }
    if (!result || result->data.empty() || result->data.front().empty()) {
      return false;
    }
    awayTeamID = std::atoi(result->data.front().front().c_str());
  }

  SetTeamIDs(std::to_string(homeTeamID), std::to_string(awayTeamID));
  queuedFixture->team1KitNum = 1;
  queuedFixture->team2KitNum = 2;
  SetSingleControlledSide(EnvironmentInt("FOTBILER_UI_CONTROL_SIDE", -1));

  const int duration =
      std::clamp(EnvironmentInt("FOTBILER_UI_MATCH_DURATION_MINUTES", 10), 5, 90);
  const float difficulty =
      std::clamp(EnvironmentFloat("FOTBILER_UI_MATCH_DIFFICULTY", 0.75f), 0.0f, 1.0f);
  GetConfiguration()->Set("match_duration_minutes", static_cast<float>(duration));
  GetConfiguration()->Set("match_difficulty", difficulty);

  std::printf("[fotbiler-ui] Quick Match: team %d vs %d, %d min, difficulty %.2f, side %d\n",
              homeTeamID, awayTeamID, duration, difficulty,
              EnvironmentInt("FOTBILER_UI_CONTROL_SIDE", -1));
  return true;
}

bool MenuTask::PrepareFotbilerUiCareerMatch() {
  CareerDatabase& career = CareerDatabase::GetInstance();
  career.Initialize("user/career");
  if (!career.GetActiveSave()) {
    // The spawned runtime is a new process. Prefer the autosave because a
    // previous 3D result is committed there immediately after full time.
    if (!career.LoadCareerSlot(-1) && !career.LoadCareerSlot(0)) {
      std::fprintf(stderr, "[fotbiler-ui] Career Match: no career save could be loaded\n");
      return false;
    }
  }

  CareerSave* save = career.GetActiveSave();
  if (!save || save->club.clubID <= 0 || !DatabaseHasTeam(save->club.clubID)) {
    std::fprintf(stderr, "[fotbiler-ui] Career Match: active club has no valid database team\n");
    return false;
  }

  const int userTeamID = save->club.clubID;
  int opponentTeamID = 0;
  std::string opponentName;
  bool isHome = (save->season.currentWeek % 2) == 0;
  bool foundStoredFixture = false;

  for (const FixtureResult& fixture : save->season.fixtures) {
    if (fixture.played) {
      continue;
    }
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
  if (opponentName.empty()) {
    opponentName = DatabaseTeamName(opponentTeamID);
  }

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
  if (EnvironmentFlagEnabled("FOTBILER_UI_CAREER_MATCH")) {
    ready = PrepareFotbilerUiCareerMatch();
  } else if (EnvironmentFlagEnabled("FOTBILER_UI_QUICK_MATCH")) {
    ready = PrepareFotbilerUiQuickMatch();
  }

  if (ready) {
    SDL_setenv("FOTBILER_UI_QUICK_MATCH", "0", 1);
    SDL_setenv("FOTBILER_UI_CAREER_MATCH", "0", 1);
  }
  return ready;
}

void MenuTask::ProcessPhase() {
  Gui2Task::ProcessPhase();

  if (menuAction == e_MenuAction_Menu) {
    windowManager->GetPagePath()->Clear();

    GetGameTask()->Action(e_GameTaskMessage_StopMatch);
    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);

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
      if (!IsReleaseVersion()) {
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_MainMenu, properties, 0);
      } else {
        windowManager->GetPageFactory()->CreatePage((int)e_PageID_Intro, properties, 0);
      }
    } else {
      windowManager->GetPageFactory()->CreatePage((int)e_PageID_LoadingMatch, properties, 0);
    }

  } else if (menuAction == e_MenuAction_Game) {
    menuBackground->Hide();
    GetGameTask()->Action(e_GameTaskMessage_StopMenuScene);
    GetGameTask()->Action(e_GameTaskMessage_StartMatch);
  }

  menuAction = e_MenuAction_None;
}

bool MenuTask::QuickStart() {
  const bool developerQuickStart =
      !IsReleaseVersion() && GetConfiguration()->GetBool("quick_start", false);
  return (uiDirectMatchReady || developerQuickStart) &&
         EnvironmentManager::GetInstance().GetTime_ms() < 10000;
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
