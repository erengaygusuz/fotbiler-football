#include "ingame.hpp"

#include <cstdio>

#include "../controllerselect.hpp"
#include "../gameplan.hpp"
#include "../pagefactory.hpp"
#include "../settings.hpp"
#include "../../league/leaguecode.hpp"
#include "main.hpp"
#include "replaymenu.hpp"
#include "utils/localization.hpp"
#include "onthepitch/match.hpp"

using namespace blunted;

namespace {

Gui2Caption* AddSectionLabel(Gui2WindowManager* windowManager, Gui2Grid* grid, int row,
                             const std::string& id, const std::string& caption) {
  Gui2Caption* label = new Gui2Caption(windowManager, id, 0, 0, 36, 2.2f, caption);
  label->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  grid->AddView(label, row, 0);
  return label;
}

Gui2Caption* AddStatCaption(Gui2WindowManager* windowManager, const std::string& id,
                            const std::string& value, bool accent = false) {
  Gui2Caption* caption = new Gui2Caption(windowManager, id, 0, 0, 12, 3, value);
  if (accent) {
    caption->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  }
  return caption;
}

}  // namespace

IngamePage::IngamePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  teamID = pageData.properties->GetInt("teamID", 0);

  Match* match = GetGameTask()->GetMatch();
  if (!match) {
    return;
  }
  match->Pause(true);

  const int score0 = match->GetScore(0);
  const int score1 = match->GetScore(1);
  const std::string team0Name = match->GetTeam(0)->GetTeamData()->GetName();
  const std::string team1Name = match->GetTeam(1)->GetTeamData()->GetName();

  unsigned long matchTime_ms = match->GetMatchTime_ms();
  int matchMinute = static_cast<int>(matchTime_ms / 60000);
  if (matchMinute > 90)
    matchMinute = 90;

  char scoreBuf[256];
  snprintf(scoreBuf, sizeof(scoreBuf), "%s   %d - %d   %s   %d'", team0Name.c_str(), score0,
           score1, team1Name.c_str(), matchMinute);

  // Wide FIFA-era overlay: left navigation, right live match snapshot.
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_ingame", 6, 5, 88, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* kicker =
      new Gui2Caption(windowManager, "caption_ingame_kicker", 3, 2, 82, 2.3f, "MATCHDAY  ·  PAUSED");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(kicker);
  kicker->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_ingame_title", 3, 5, 82, 4,
                                       Localization::GetInstance().Translate("ingame_pause"));
  title->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
  frame->AddView(title);
  title->Show();

  Gui2Caption* scoreLine =
      new Gui2Caption(windowManager, "caption_ingame_score", 3, 9, 82, 3.2f, scoreBuf);
  scoreLine->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright1));
  frame->AddView(scoreLine);
  scoreLine->Show();

  // Navigation panel.
  Gui2Frame* navigationPanel =
      new Gui2Frame(windowManager, "frame_ingame_navigation", 3, 15, 39, 67, true);
  frame->AddView(navigationPanel);
  navigationPanel->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_ingame", 2, 2, 35, 62);
  int row = 0;

  AddSectionLabel(windowManager, grid, row++, "caption_ingame_section_match", "MATCH");

  Gui2Button* buttonResume =
      new Gui2Button(windowManager, "button_resume", 0, 0, 36, 3.4f,
                     Localization::GetInstance().Translate("ingame_resume_match"));
  buttonResume->sig_OnClick.connect([this](...) {
    GetMenuTask()->ReleaseAllButtons();
    GetGameTask()->GetMatch()->Pause(false);
    GoBack();
  });
  grid->AddView(buttonResume, row++, 0);

  Gui2Button* buttonGamePlan =
      new Gui2Button(windowManager, "button_gameplan", 0, 0, 36, 3.4f, "TEAM MANAGEMENT");
  buttonGamePlan->sig_OnClick.connect([this](...) { GoGamePlan(); });
  grid->AddView(buttonGamePlan, row++, 0);

  Gui2Button* buttonSetPieces =
      new Gui2Button(windowManager, "button_setpieces", 0, 0, 36, 3.4f,
                     Localization::GetInstance().Translate("ingame_set_pieces"));
  buttonSetPieces->sig_OnClick.connect([this](...) { GoSetPieceEditor(); });
  grid->AddView(buttonSetPieces, row++, 0);

  AddSectionLabel(windowManager, grid, row++, "caption_ingame_section_media", "MEDIA & PRESENTATION");

  Gui2Button* buttonReplay =
      new Gui2Button(windowManager, "button_replay", 0, 0, 36, 3.4f,
                     Localization::GetInstance().Translate("ingame_replay"));
  buttonReplay->sig_OnClick.connect([this](...) { GoReplay(); });
  grid->AddView(buttonReplay, row++, 0);

  Gui2Button* buttonCameraSettings =
      new Gui2Button(windowManager, "button_camerasettings", 0, 0, 36, 3.4f,
                     Localization::GetInstance().Translate("ingame_camera_settings"));
  buttonCameraSettings->sig_OnClick.connect([this](...) { GoCameraSettings(); });
  grid->AddView(buttonCameraSettings, row++, 0);

  AddSectionLabel(windowManager, grid, row++, "caption_ingame_section_settings", "SETTINGS");

  Gui2Button* buttonControllerSelect =
      new Gui2Button(windowManager, "button_controllerselect", 0, 0, 36, 3.4f,
                     Localization::GetInstance().Translate("ingame_controller_select"));
  buttonControllerSelect->sig_OnClick.connect([this](...) { GoControllerSelect(); });
  grid->AddView(buttonControllerSelect, row++, 0);

  Gui2Button* buttonVisualOptions =
      new Gui2Button(windowManager, "button_visualoptions", 0, 0, 36, 3.4f,
                     Localization::GetInstance().Translate("ingame_visual_options"));
  buttonVisualOptions->sig_OnClick.connect([this](...) { GoVisualOptions(); });
  grid->AddView(buttonVisualOptions, row++, 0);

  Gui2Button* buttonSystemSettings =
      new Gui2Button(windowManager, "button_systemsettings", 0, 0, 36, 3.4f,
                     Localization::GetInstance().Translate("ingame_system_settings"));
  buttonSystemSettings->sig_OnClick.connect([this](...) { GoSystemSettings(); });
  grid->AddView(buttonSystemSettings, row++, 0);

  AddSectionLabel(windowManager, grid, row++, "caption_ingame_section_exit", "EXIT");

  Gui2Button* buttonPreQuit =
      new Gui2Button(windowManager, "button_quit", 0, 0, 36, 3.4f,
                     Localization::GetInstance().Translate("ingame_forfeit_match"));
  buttonPreQuit->sig_OnClick.connect([this](...) { GoPreQuit(); });
  grid->AddView(buttonPreQuit, row++, 0);

  grid->UpdateLayout(0.42f);
  navigationPanel->AddView(grid);
  grid->Show();

  // Match-facts panel mirrors the modern pause mock-up while remaining fully
  // data-driven from MatchData.
  Gui2Frame* factsPanel = new Gui2Frame(windowManager, "frame_ingame_facts", 44, 15, 41, 67, true);
  frame->AddView(factsPanel);
  factsPanel->Show();

  Gui2Caption* factsKicker =
      new Gui2Caption(windowManager, "caption_ingame_facts_kicker", 2, 2, 37, 2.3f, "LIVE");
  factsKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  factsPanel->AddView(factsKicker);
  factsKicker->Show();

  Gui2Caption* factsTitle =
      new Gui2Caption(windowManager, "caption_ingame_facts_title", 2, 5, 37, 3, "MATCH FACTS");
  factsPanel->AddView(factsTitle);
  factsTitle->Show();

  MatchData* md = match->GetMatchData();
  Gui2Grid* statsGrid = new Gui2Grid(windowManager, "stats_grid", 2, 11, 37, 42);
  factsPanel->AddView(statsGrid);

  Gui2Caption* h0 = AddStatCaption(windowManager, "s_t1", md->GetTeamData(0)->GetShortName(), true);
  Gui2Caption* h1 = AddStatCaption(windowManager, "s_l1", "MATCH STATS", true);
  Gui2Caption* h2 = AddStatCaption(windowManager, "s_t2", md->GetTeamData(1)->GetShortName(), true);
  statsGrid->AddView(h0, 0, 0);
  statsGrid->AddView(h1, 1, 0);
  statsGrid->AddView(h2, 2, 0);

  statsGrid->AddView(AddStatCaption(windowManager, "s_v1", int_to_str(md->GetGoalCount(0))), 0, 1);
  statsGrid->AddView(AddStatCaption(windowManager, "s_l2", "SCORE"), 1, 1);
  statsGrid->AddView(AddStatCaption(windowManager, "s_v2", int_to_str(md->GetGoalCount(1))), 2, 1);

  const unsigned long p0 = md->GetPossessionTime_ms(0);
  const unsigned long p1 = md->GetPossessionTime_ms(1);
  const int p0_pct = (p0 + p1 > 0) ? static_cast<int>((p0 * 100) / (p0 + p1)) : 50;
  statsGrid->AddView(AddStatCaption(windowManager, "s_p1", int_to_str(p0_pct) + "%"), 0, 2);
  statsGrid->AddView(AddStatCaption(windowManager, "s_l3", "POSSESSION"), 1, 2);
  statsGrid->AddView(AddStatCaption(windowManager, "s_p2", int_to_str(100 - p0_pct) + "%"), 2, 2);

  statsGrid->AddView(AddStatCaption(windowManager, "s_s1", int_to_str(md->GetShots(0))), 0, 3);
  statsGrid->AddView(AddStatCaption(windowManager, "s_l4", "SHOTS"), 1, 3);
  statsGrid->AddView(AddStatCaption(windowManager, "s_s2", int_to_str(md->GetShots(1))), 2, 3);

  statsGrid->AddView(AddStatCaption(windowManager, "s_ot1", int_to_str(md->GetShotsOnTarget(0))), 0, 4);
  statsGrid->AddView(AddStatCaption(windowManager, "s_otl", "ON TARGET"), 1, 4);
  statsGrid->AddView(AddStatCaption(windowManager, "s_ot2", int_to_str(md->GetShotsOnTarget(1))), 2, 4);

  const int attempts0 = md->GetPassAttempts(0);
  const int attempts1 = md->GetPassAttempts(1);
  const int passPct0 = attempts0 > 0 ? md->GetPassesCompleted(0) * 100 / attempts0 : 0;
  const int passPct1 = attempts1 > 0 ? md->GetPassesCompleted(1) * 100 / attempts1 : 0;
  statsGrid->AddView(AddStatCaption(windowManager, "s_pa1", int_to_str(passPct0) + "%"), 0, 5);
  statsGrid->AddView(AddStatCaption(windowManager, "s_pal", "PASS ACC."), 1, 5);
  statsGrid->AddView(AddStatCaption(windowManager, "s_pa2", int_to_str(passPct1) + "%"), 2, 5);

  statsGrid->AddView(AddStatCaption(windowManager, "s_f1", int_to_str(md->GetFouls(0))), 0, 6);
  statsGrid->AddView(AddStatCaption(windowManager, "s_fl", "FOULS"), 1, 6);
  statsGrid->AddView(AddStatCaption(windowManager, "s_f2", int_to_str(md->GetFouls(1))), 2, 6);

  statsGrid->UpdateLayout(0.5f);
  statsGrid->Show();

  Gui2Caption* hintCaption =
      new Gui2Caption(windowManager, "caption_ingame_hint", 3, 84, 80, 2.3f,
                      "ESC Resume  ·  ENTER Select  ·  Team Management, Replay and Settings remain live");
  frame->AddView(hintCaption);
  hintCaption->Show();

  buttonResume->SetFocus();
  this->Show();
}

IngamePage::~IngamePage() {}

void IngamePage::GoControllerRemap() {
  CreatePage(e_PageID_Controller);
}

void IngamePage::GoGamePlan() {
  Properties properties;
  properties.Set("teamID", teamID);
  CreatePage(e_PageID_GamePlan, properties);
}

void IngamePage::GoControllerSelect() {
  Properties properties;
  properties.SetBool("isInGame", true);
  CreatePage(e_PageID_ControllerSelect, properties);
}

void IngamePage::GoCameraSettings() {
  CreatePage(e_PageID_Camera);
}

void IngamePage::GoVisualOptions() {
  CreatePage(e_PageID_VisualOptions);
}

void IngamePage::GoSystemSettings() {
  CreatePage(e_PageID_Settings);
}

void IngamePage::GoReplay() {
  CreatePage(e_PageID_Replay);
}

void IngamePage::GoPreQuit() {
  CreatePage(e_PageID_PreQuit);
}

void IngamePage::GoSetPieceEditor() {
  Properties properties;
  properties.Set("teamDatabaseID",
                 GetGameTask()->GetMatch()->GetTeam(teamID)->GetTeamData()->GetDatabaseID());
  CreatePage((int)e_PageID_SetPieceEditor, properties);
}

void IngamePage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsEscape()) {
    GetMenuTask()->ReleaseAllButtons();
    GetGameTask()->GetMatch()->Pause(false);
  }
  Gui2Page::ProcessWindowingEvent(event);
}

PreQuitPage::PreQuitPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_prequit", 25, 36, 50, 28, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* kicker =
      new Gui2Caption(windowManager, "caption_prequit_kicker", 3, 2, 44, 2.3f, "MATCH");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(kicker);
  kicker->Show();

  Gui2Caption* restartCaption =
      new Gui2Caption(windowManager, "caption_prequit_info", 3, 6, 44, 3,
                      Localization::GetInstance().Translate("ingame_forfeit_confirm"));
  frame->AddView(restartCaption);
  restartCaption->Show();

  Gui2Button* okButton =
      new Gui2Button(windowManager, "button_prequit_ok", 0, 0, 42, 3.6f,
                     Localization::GetInstance().Translate("ingame_forfeit"));
  Gui2Button* cancelButton =
      new Gui2Button(windowManager, "button_prequit_cancel", 0, 0, 42, 3.6f,
                     Localization::GetInstance().Translate("ingame_continue_match"));
  okButton->sig_OnClick.connect([this](...) { GoMenu(); });
  cancelButton->sig_OnClick.connect([this](...) { GoBack(); });

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_prequit", 3, 12, 44, 12);
  grid->AddView(cancelButton, 0, 0);
  grid->AddView(okButton, 1, 0);
  grid->UpdateLayout(0.6f);
  frame->AddView(grid);
  grid->Show();

  cancelButton->SetFocus();
  this->Show();
}

PreQuitPage::~PreQuitPage() {}

void PreQuitPage::GoMenu() {
  LeagueClearPendingFixture();
  this->Exit();
  GetMenuTask()->SetMenuAction(e_MenuAction_Menu);
  delete this;
}
