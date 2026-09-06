#include "gamepage.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

#include "../../league/leaguecode.hpp"
#include "../../onthepitch/match.hpp"
#include "../../presentation/ui/rmlui/frontend_runtime_bridge.hpp"
#include "../../presentation/ui/rmlui/runtime_ui_bridge.hpp"
#include "../career/career_database.hpp"
#include "../pagefactory.hpp"
#include "framework/scheduler.hpp"
#include "gameover.hpp"
#include "main.hpp"
#include "phasemenu.hpp"
#include "replaymenu.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kMenuSmokeQuitDelay_ms = 2000;

bool MenuSmokeQuickMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_quick_match", false);
}

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

bool EnvironmentFlagEnabled(const char* name) {
  const char* value = std::getenv(name);
  return value && value[0] != '\0' && std::string(value) != "0";
}

bool ModernUiSessionActive() {
  return EnvironmentFlagEnabled("FOTBILER_UI_MODERN_SESSION");
}

bool ModernFrontendAppActive() {
  return EnvironmentFlagEnabled("FOTBILER_UI_MODERN_APP");
}

bool ModernUiActive() {
  return ModernUiSessionActive() || ModernFrontendAppActive();
}

float ReplaySpeedForStep(int step) {
  switch (step) {
    case 0: return 0.5f;
    case 2: return 2.0f;
    default: return 1.0f;
  }
}

void PublishModernMatchSnapshot(Match* match) {
  if (!match) return;

  ui::runtime::MatchSnapshot snapshot;
  snapshot.homeName = match->GetTeam(0)->GetTeamData()->GetName();
  snapshot.awayName = match->GetTeam(1)->GetTeamData()->GetName();
  snapshot.homeShortName = match->GetTeam(0)->GetTeamData()->GetShortName();
  snapshot.awayShortName = match->GetTeam(1)->GetTeamData()->GetShortName();
  snapshot.homeScore = match->GetScore(0);
  snapshot.awayScore = match->GetScore(1);
  snapshot.minute = std::min(90, static_cast<int>(match->GetMatchTime_ms() / 60000));

  MatchData* data = match->GetMatchData();
  if (data) {
    const unsigned long homePossession = data->GetPossessionTime_ms(0);
    const unsigned long awayPossession = data->GetPossessionTime_ms(1);
    const unsigned long possessionTotal = homePossession + awayPossession;
    snapshot.homePossession = possessionTotal > 0
                                  ? static_cast<int>((homePossession * 100) / possessionTotal)
                                  : 50;
    snapshot.awayPossession = 100 - snapshot.homePossession;
    snapshot.homeShots = data->GetShots(0);
    snapshot.awayShots = data->GetShots(1);
    snapshot.homeShotsOnTarget = data->GetShotsOnTarget(0);
    snapshot.awayShotsOnTarget = data->GetShotsOnTarget(1);
    const int homeAttempts = data->GetPassAttempts(0);
    const int awayAttempts = data->GetPassAttempts(1);
    snapshot.homePassAccuracy =
        homeAttempts > 0 ? data->GetPassesCompleted(0) * 100 / homeAttempts : 0;
    snapshot.awayPassAccuracy =
        awayAttempts > 0 ? data->GetPassesCompleted(1) * 100 / awayAttempts : 0;
    snapshot.homeFouls = data->GetFouls(0);
    snapshot.awayFouls = data->GetFouls(1);
  }

  ui::runtime::PublishMatchSnapshot(snapshot);
}

void OpenModernPause(Match* match) {
  if (!match) return;
  PublishModernMatchSnapshot(match);
  match->Pause(true);
  ui::runtime::SetScreen(ui::runtime::Screen::Pause);
}

}  // namespace

GamePage::GamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      match(0),
      matchReadyTime_ms(0),
      autoQuitTriggered(false),
      modernReplayActive(false),
      modernReplayPlaying(false),
      modernReplayAutoClose(false),
      modernReplaySpeedStep(1),
      modernReplayCam(0),
      modernReplayRequestedOffset_ms(0),
      modernReplayModifier(0.0f),
      modernReplayMinTime_ms(10),
      modernReplayMaxTime_ms(10),
      modernReplayTime_ms(10) {
  if (!ModernUiActive()) {
    Gui2Caption* betaSign =
        new Gui2Caption(windowManager, "caption_betasign", 0, 0, 0, 2, "League-Soccer v0.4.0");
    betaSign->SetColor(Vector3(180, 180, 180));
    betaSign->SetTransparency(0.3f);
    this->AddView(betaSign);
    float w = betaSign->GetTextWidthPercent();
    betaSign->SetPosition(50 - w * 0.5f, 97.0f);
    betaSign->Show();
  }

  this->Show();
  this->SetFocus();
}

GamePage::~GamePage() {
  if (match) {
    if (Verbose()) printf("disconnecting signals\n");
    conn_MatchPhaseChange.disconnect();
    conn_ShortReplayMoment.disconnect();
    conn_ExtendedReplayMoment.disconnect();
    conn_GameOver.disconnect();
    if (modernReplayActive) match->SetAutoUpdateIngameCamera(true);
  }
  if (ModernUiActive()) ui::runtime::Reset();
}

void GamePage::BeginModernReplay() {
  if (!match || modernReplayActive) return;

  match->Pause(true);
  match->SetAutoUpdateIngameCamera(false);

  const signed long replayStart =
      static_cast<signed long>(match->GetActualTime_ms()) - match->GetReplaySize_ms();
  modernReplayMinTime_ms = std::max<signed long>(10, replayStart);
  modernReplayMaxTime_ms =
      std::max<signed long>(10, static_cast<signed long>(match->GetActualTime_ms()) - 10);

  const int requestedOffset = modernReplayRequestedOffset_ms > 0
                                  ? modernReplayRequestedOffset_ms
                                  : std::min(5000, match->GetReplaySize_ms());
  modernReplayTime_ms = std::clamp<signed long>(modernReplayMaxTime_ms - requestedOffset,
                                                 modernReplayMinTime_ms,
                                                 modernReplayMaxTime_ms);
  modernReplaySpeedStep = 1;
  modernReplayCam = modernReplayAutoClose ? 1 : 0;
  modernReplayModifier = 0.0f;
  modernReplayPlaying = true;
  modernReplayActive = true;

  ApplyModernReplayState();
  PublishModernReplaySnapshot();
}

void GamePage::EndModernReplay(bool resumeMatch) {
  if (!match || !modernReplayActive) return;

  modernReplayPlaying = false;
  modernReplayTime_ms = modernReplayMaxTime_ms;
  ApplyModernReplayState();
  modernReplayActive = false;
  modernReplayAutoClose = false;
  modernReplayRequestedOffset_ms = 0;
  ui::runtime::PublishReplaySnapshot(ui::runtime::ReplaySnapshot{});

  GetScheduler()->ResetTaskSequenceTime("game");
  match->SetAutoUpdateIngameCamera(true);

  if (resumeMatch) {
    match->Pause(false);
    ui::runtime::SetScreen(ui::runtime::Screen::None);
    GetMenuTask()->ReleaseAllButtons();
  } else {
    match->Pause(true);
    PublishModernMatchSnapshot(match);
    ui::runtime::SetScreen(ui::runtime::Screen::Pause);
  }
}

void GamePage::ApplyModernReplayState() {
  if (!match || !modernReplayActive) return;

  match->replayState.Lock();
  match->replayState->viewTime_ms = static_cast<unsigned long>(modernReplayTime_ms);
  match->replayState->cam = modernReplayCam;
  match->replayState->modifierValue = modernReplayModifier;
  match->replayState->dirty = true;
  match->replayState.Unlock();
}

void GamePage::PublishModernReplaySnapshot() {
  if (!modernReplayActive) {
    ui::runtime::PublishReplaySnapshot(ui::runtime::ReplaySnapshot{});
    return;
  }

  const unsigned long duration = static_cast<unsigned long>(
      std::max<signed long>(0, modernReplayMaxTime_ms - modernReplayMinTime_ms));
  const unsigned long elapsed = static_cast<unsigned long>(
      std::max<signed long>(0, modernReplayTime_ms - modernReplayMinTime_ms));

  ui::runtime::ReplaySnapshot snapshot;
  snapshot.active = true;
  snapshot.playing = modernReplayPlaying;
  snapshot.speed = ReplaySpeedForStep(modernReplaySpeedStep);
  snapshot.camera = modernReplayCam + 1;
  snapshot.cameraCount = match ? std::max(1, match->GetReplayCamCount()) : 1;
  snapshot.elapsed_ms = elapsed;
  snapshot.duration_ms = duration;
  snapshot.secondsAgo = static_cast<unsigned long>(
      std::max<signed long>(0, modernReplayMaxTime_ms - modernReplayTime_ms) / 1000);
  snapshot.progressPercent = duration > 0
                                 ? static_cast<int>(std::clamp<unsigned long>(elapsed * 100 / duration,
                                                                              0, 100))
                                 : 100;
  ui::runtime::PublishReplaySnapshot(snapshot);
}

void GamePage::HandleModernReplayCommand(int commandValue) {
  if (!modernReplayActive) return;

  using ui::runtime::Command;
  const Command command = static_cast<Command>(commandValue);
  bool replayStateChanged = false;

  switch (command) {
    case Command::ReplayTogglePlayback:
      if (!modernReplayPlaying && modernReplayTime_ms >= modernReplayMaxTime_ms)
        modernReplayTime_ms = modernReplayMinTime_ms;
      modernReplayPlaying = !modernReplayPlaying;
      replayStateChanged = true;
      break;
    case Command::ReplayCycleSpeed:
      modernReplaySpeedStep = (modernReplaySpeedStep + 1) % 3;
      break;
    case Command::ReplayCycleCamera:
      modernReplayCam = (modernReplayCam + 1) % std::max(1, match->GetReplayCamCount());
      modernReplayModifier = 0.0f;
      replayStateChanged = true;
      break;
    case Command::ReplaySeekBackward:
      modernReplayPlaying = false;
      modernReplayTime_ms =
          std::max<signed long>(modernReplayMinTime_ms, modernReplayTime_ms - 1000);
      replayStateChanged = true;
      break;
    case Command::ReplaySeekForward:
      modernReplayPlaying = false;
      modernReplayTime_ms =
          std::min<signed long>(modernReplayMaxTime_ms, modernReplayTime_ms + 1000);
      replayStateChanged = true;
      break;
    case Command::ReplayCameraUp:
      modernReplayModifier -= 0.15f;
      replayStateChanged = true;
      break;
    case Command::ReplayCameraDown:
      modernReplayModifier += 0.15f;
      replayStateChanged = true;
      break;
    case Command::ReplayExit:
      EndModernReplay(modernReplayAutoClose);
      return;
    default:
      return;
  }

  if (modernReplayCam == 2) {
    if (modernReplayModifier < -1.0f) modernReplayModifier += 2.0f;
    if (modernReplayModifier > 1.0f) modernReplayModifier -= 2.0f;
  } else {
    modernReplayModifier = std::clamp(modernReplayModifier, -1.0f, 1.0f);
  }

  if (replayStateChanged) ApplyModernReplayState();
  PublishModernReplaySnapshot();
}

void GamePage::UpdateModernReplay() {
  if (!match || !modernReplayActive) return;

  if (modernReplayPlaying) {
    const float speed = ReplaySpeedForStep(modernReplaySpeedStep);
    modernReplayTime_ms += static_cast<signed long>(std::lround(10.0f * speed));
    if (modernReplayTime_ms >= modernReplayMaxTime_ms) {
      modernReplayTime_ms = modernReplayMaxTime_ms;
      modernReplayPlaying = false;
      ApplyModernReplayState();
      PublishModernReplaySnapshot();
      if (modernReplayAutoClose) {
        EndModernReplay(true);
      }
      return;
    }
    ApplyModernReplayState();
  }

  PublishModernReplaySnapshot();
}

void GamePage::Process() {
  Gui2Page::Process();

  if (!match) {
    GetGameTask()->matchLifetimeMutex.lock();
    if (GetGameTask()->GetMatch() != 0) {
      match = GetGameTask()->GetMatch();

      if (Verbose()) printf("connecting signals\n");

      conn_MatchPhaseChange =
          match->sig_OnMatchPhaseChange.connect([this](...) { GoMatchPhasePage(); });
      conn_ShortReplayMoment =
          match->sig_OnShortReplayMoment.connect([this](...) { GoShortReplayPage(); });
      conn_ExtendedReplayMoment =
          match->sig_OnExtendedReplayMoment.connect([this](...) { GoExtendedReplayPage(); });
      conn_GameOver = match->sig_OnGameOver.connect([this](...) { GoGameOverPage(); });

      if (ModernUiActive()) {
        GetMenuTask()->SetMenuBackgroundVisible(false);
        PublishModernMatchSnapshot(match);
        ui::runtime::SetScreen(ui::runtime::Screen::None);
        if (ModernFrontendAppActive()) ui::frontend::BeginMatch();
      }

      if (MenuSmokeQuickMatchEnabled() || MenuSmokeFullMatchEnabled()) {
        matchReadyTime_ms = EnvironmentManager::GetInstance().GetTime_ms();
        printf("[menu-smoke] Gameplay page reached and live match is active\n");
      }
    }
    GetGameTask()->matchLifetimeMutex.unlock();
  }

  if (ModernUiActive() && match) {
    const ui::runtime::Screen screen = ui::runtime::GetScreen();
    if (screen == ui::runtime::Screen::Replay && !modernReplayActive) {
      BeginModernReplay();
    } else if (screen != ui::runtime::Screen::Replay && modernReplayActive) {
      EndModernReplay(false);
    }

    const ui::runtime::Command command = ui::runtime::ConsumeCommand();
    if (command == ui::runtime::Command::ResumeMatch) {
      match->Pause(false);
      ui::runtime::SetScreen(ui::runtime::Screen::None);
      GetMenuTask()->ReleaseAllButtons();
    } else if (command == ui::runtime::Command::ExitMatch) {
      LeagueClearPendingFixture();
      CareerDatabase::GetInstance().ClearPendingFixture();
      ui::runtime::SetScreen(ui::runtime::Screen::None);

      if (ModernFrontendAppActive()) {
        const ui::frontend::ReturnTarget target =
            ui::frontend::GetSessionKind() == ui::frontend::SessionKind::Career
                ? ui::frontend::ReturnTarget::CareerCentral
                : ui::frontend::ReturnTarget::MatchSetup;
        this->Exit();
        GetMenuTask()->ReturnToFotbilerFrontend(target);
        delete this;
        return;
      }

      EnvironmentManager::GetInstance().SignalQuit();
    } else if (command != ui::runtime::Command::None) {
      HandleModernReplayCommand(static_cast<int>(command));
    }

    if (modernReplayActive) UpdateModernReplay();
    if (match->GetPause()) PublishModernMatchSnapshot(match);
  }

  if (match && !autoQuitTriggered && MenuSmokeQuickMatchEnabled() && !MenuSmokeFullMatchEnabled() &&
      matchReadyTime_ms != 0 &&
      EnvironmentManager::GetInstance().GetTime_ms() >=
          matchReadyTime_ms + kMenuSmokeQuitDelay_ms) {
    autoQuitTriggered = true;
    printf("[menu-smoke] Quick Match verification succeeded, quitting test run\n");
    EnvironmentManager::GetInstance().SignalQuit();
  }
}

void GamePage::GoShortReplayPage() {
  if (ModernUiActive() && match) {
    modernReplayRequestedOffset_ms = std::min(3000, match->GetReplaySize_ms());
    modernReplayAutoClose = true;
    ui::runtime::SetScreen(ui::runtime::Screen::Replay);
    return;
  }
  CreatePage((int)e_PageID_Replay);
}

void GamePage::GoExtendedReplayPage() {
  if (ModernUiActive() && match) {
    modernReplayRequestedOffset_ms = match->GetReplaySize_ms();
    modernReplayAutoClose = true;
    ui::runtime::SetScreen(ui::runtime::Screen::Replay);
    return;
  }

  this->Exit();

  Properties properties;
  ReplayPage* replayPage = static_cast<ReplayPage*>(
      windowManager->GetPageFactory()->CreatePage((int)e_PageID_Replay, properties, 0));

  int replayHistoryOffset_ms = match->GetReplaySize_ms();
  bool stayInReplay = true;
  replayPage->Autorun(replayHistoryOffset_ms, stayInReplay);

  delete this;
}

void GamePage::GoMatchPhasePage() {
  e_MatchPhase nextPhase = match->GetMatchPhase();

  Properties properties;
  properties.Set("nextphase", (int)nextPhase);
  CreatePage((int)e_PageID_MatchPhase, properties, 0);
}

void GamePage::GoGameOverPage() {
  CreatePage((int)e_PageID_GameOver);
}

void GamePage::OnCreatedMatch() {}

void GamePage::ProcessWindowingEvent(WindowingEvent* event) {
  if (Verbose())
    if (event->IsEscape())
      printf("escape!\n");
  event->Ignore();
}

void GamePage::ProcessKeyboardEvent(KeyboardEvent* event) {
  if (event->GetKeyOnce(SDLK_TAB)) {
    if (match) match->ToggleStatsOverlay();
    return;
  }

  if (event->GetKeyOnce(SDLK_ESCAPE)) {
    if (ModernUiActive()) {
      OpenModernPause(match);
      return;
    }

    int controllerID = 0;
    const std::vector<IHIDevice*>& controllers = GetControllers();
    for (unsigned int c = 0; c < controllers.size(); c++) {
      if (controllers.at(c)->GetDeviceType() == e_HIDeviceType_Keyboard) {
        controllerID = c;
        break;
      }
    }

    if (Verbose()) printf("controller index %i (keyboard) pressed start\n", controllerID);

    int teamID = 0;
    const std::vector<SideSelection> sides = GetMenuTask()->GetControllerSetup();
    for (unsigned int s = 0; s < sides.size(); s++) {
      if (sides.at(s).controllerID == (signed int)controllerID) {
        teamID = int(round(sides.at(s).side * 0.5 + 0.5));
        break;
      }
    }

    Properties properties;
    properties.Set("teamID", teamID);
    CreatePage((int)e_PageID_Ingame, properties);
    return;
  }

  event->Ignore();
}

void GamePage::ProcessJoystickEvent(JoystickEvent* event) {
  const std::vector<IHIDevice*>& controllers = GetControllers();
  for (unsigned int c = 0; c < controllers.size(); c++) {
    if (controllers.at(c)->GetDeviceType() == e_HIDeviceType_Gamepad) {
      HIDGamepad* gamepad = static_cast<HIDGamepad*>(controllers.at(c));
      int joyID = gamepad->GetGamepadID();

      if (event->GetButton(joyID, gamepad->GetControllerMapping(
                                      gamepad->GetFunctionMapping(e_ButtonFunction_Start)))) {
        if (ModernUiActive()) {
          OpenModernPause(match);
          return;
        }

        if (Verbose())
          printf("controller index %i, gamepad/joy ID %i pressed start\n", c, joyID);

        int teamID = 0;
        const std::vector<SideSelection> sides = GetMenuTask()->GetControllerSetup();
        for (unsigned int s = 0; s < sides.size(); s++) {
          if (sides.at(s).controllerID == (signed int)c) {
            teamID = int(round(sides.at(s).side * 0.5 + 0.5));
            break;
          }
        }

        Properties properties;
        properties.Set("teamID", teamID);
        CreatePage((int)e_PageID_Ingame, properties);
        return;
      }
    }
  }

  event->Ignore();
}
