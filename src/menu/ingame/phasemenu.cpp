#include "phasemenu.hpp"

#include "../gameplan.hpp"
#include "../pagefactory.hpp"
#include "main.hpp"
#include "utils/localization.hpp"
#include "../../onthepitch/match.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kMenuSmokeAdvanceDelay_ms = 500;

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

const char* PhaseName(e_MatchPhase phase) {
  switch (phase) {
    case e_MatchPhase_2ndHalf: return "second half";
    case e_MatchPhase_1stExtraTime: return "first extra time";
    case e_MatchPhase_2ndExtraTime: return "second extra time";
    case e_MatchPhase_Penalties: return "penalties";
    default: return "next phase";
  }
}

Gui2Caption* PhaseStat(Gui2WindowManager* windowManager, const std::string& id,
                       const std::string& text, bool accent = false) {
  Gui2Caption* caption = new Gui2Caption(windowManager, id, 0, 0, 15, 3, text);
  if (accent) {
    caption->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  }
  return caption;
}

}  // namespace

MatchPhasePage::MatchPhasePage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoAdvanceTriggered(false) {
  Match* match = GetGameTask()->GetMatch();
  if (!match) {
    return;
  }
  match->Pause(true);
  nextPhase = static_cast<e_MatchPhase>(pageData.properties->GetInt("nextphase"));

  std::string titleText = "MATCH BREAK";
  std::string actionText = "CONTINUE MATCH";
  if (nextPhase == e_MatchPhase_2ndHalf) {
    titleText = "HALF-TIME";
    actionText = "START SECOND HALF";
  } else if (nextPhase == e_MatchPhase_1stExtraTime) {
    titleText = "END OF 90 MINUTES";
    actionText = "START EXTRA TIME";
  } else if (nextPhase == e_MatchPhase_2ndExtraTime) {
    titleText = "EXTRA-TIME BREAK";
    actionText = "CONTINUE EXTRA TIME";
  } else if (nextPhase == e_MatchPhase_Penalties) {
    titleText = "PENALTY SHOOTOUT";
    actionText = "START PENALTIES";
  }

  Gui2Frame* frame = new Gui2Frame(windowManager, "phase_frame", 6, 5, 88, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* kicker =
      new Gui2Caption(windowManager, "phase_kicker", 3, 2, 82, 2.3f, "MATCHDAY  ·  BREAK");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(kicker);
  kicker->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_phase", 3, 5, 82, 4, titleText);
  frame->AddView(title);
  title->Show();

  Gui2Frame* factsPanel = new Gui2Frame(windowManager, "phase_facts", 3, 14, 56, 63, true);
  frame->AddView(factsPanel);
  factsPanel->Show();

  MatchData* md = match->GetMatchData();
  const std::string score = match->GetTeam(0)->GetTeamData()->GetName() + "   " +
                            int_to_str(md->GetGoalCount(0)) + " - " +
                            int_to_str(md->GetGoalCount(1)) + "   " +
                            match->GetTeam(1)->GetTeamData()->GetName();
  Gui2Caption* scoreCaption =
      new Gui2Caption(windowManager, "phase_score", 2, 2, 52, 4, score);
  scoreCaption->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  factsPanel->AddView(scoreCaption);
  scoreCaption->Show();

  Gui2Grid* statsGrid = new Gui2Grid(windowManager, "phase_stats_grid", 2, 9, 52, 43);
  const unsigned long p0 = md->GetPossessionTime_ms(0);
  const unsigned long p1 = md->GetPossessionTime_ms(1);
  const int p0Pct = p0 + p1 > 0 ? static_cast<int>((p0 * 100) / (p0 + p1)) : 50;
  const int p1Pct = 100 - p0Pct;
  const int passAttempts0 = md->GetPassAttempts(0);
  const int passAttempts1 = md->GetPassAttempts(1);
  const int passPct0 = passAttempts0 > 0 ? md->GetPassesCompleted(0) * 100 / passAttempts0 : 0;
  const int passPct1 = passAttempts1 > 0 ? md->GetPassesCompleted(1) * 100 / passAttempts1 : 0;

  statsGrid->AddView(PhaseStat(windowManager, "ph_home", md->GetTeamData(0)->GetShortName(), true), 0, 0);
  statsGrid->AddView(PhaseStat(windowManager, "ph_label", "MATCH FACTS", true), 1, 0);
  statsGrid->AddView(PhaseStat(windowManager, "ph_away", md->GetTeamData(1)->GetShortName(), true), 2, 0);

  auto row = [&](int index, const std::string& left, const std::string& label,
                 const std::string& right, const std::string& suffix) {
    statsGrid->AddView(PhaseStat(windowManager, "ph_l_" + suffix, left), 0, index);
    statsGrid->AddView(PhaseStat(windowManager, "ph_m_" + suffix, label), 1, index);
    statsGrid->AddView(PhaseStat(windowManager, "ph_r_" + suffix, right), 2, index);
  };
  row(1, int_to_str(p0Pct) + "%", "POSSESSION", int_to_str(p1Pct) + "%", "pos");
  row(2, int_to_str(md->GetShots(0)), "SHOTS", int_to_str(md->GetShots(1)), "shots");
  row(3, int_to_str(md->GetShotsOnTarget(0)), "ON TARGET",
      int_to_str(md->GetShotsOnTarget(1)), "target");
  row(4, int_to_str(passPct0) + "%", "PASS ACCURACY", int_to_str(passPct1) + "%", "pass");
  row(5, int_to_str(md->GetFouls(0)), "FOULS", int_to_str(md->GetFouls(1)), "fouls");
  statsGrid->UpdateLayout(0.55f);
  factsPanel->AddView(statsGrid);
  statsGrid->Show();

  Gui2Frame* actionPanel = new Gui2Frame(windowManager, "phase_actions", 61, 14, 24, 63, true);
  frame->AddView(actionPanel);
  actionPanel->Show();

  Gui2Caption* talkKicker = new Gui2Caption(windowManager, "phase_talk_kicker", 2, 2, 20, 2.3f,
                                            "NEXT PHASE");
  talkKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  actionPanel->AddView(talkKicker);
  talkKicker->Show();

  Gui2Caption* talk = new Gui2Caption(windowManager, "phase_talk", 2, 6, 20, 7,
                                      "Review the shape, make changes and return to the pitch.");
  actionPanel->AddView(talk);
  talk->Show();

  buttonNext = new Gui2Button(windowManager, "button_next", 2, 21, 20, 4, actionText);
  buttonNext->sig_OnClick.connect([this](...) { ContinueGame(); });
  actionPanel->AddView(buttonNext);
  buttonNext->Show();

  Gui2Button* buttonGamePlan =
      new Gui2Button(windowManager, "button_phase_gameplan", 2, 28, 20, 4, "TEAM MANAGEMENT");
  buttonGamePlan->sig_OnClick.connect([this](...) { GoGamePlan(); });
  actionPanel->AddView(buttonGamePlan);
  buttonGamePlan->Show();

  grid = new Gui2Grid(windowManager, "grid", 0, 0, 1, 1);
  frame->AddView(grid);

  Gui2Caption* footer = new Gui2Caption(windowManager, "phase_footer", 3, 82, 82, 2.3f,
                                        "ENTER Continue  ·  Team Management available before restart");
  frame->AddView(footer);
  footer->Show();

  buttonNext->SetFocus();
  this->Show();
}

MatchPhasePage::~MatchPhasePage() {}

void MatchPhasePage::Process() {
  Gui2Page::Process();
  if (!autoAdvanceTriggered && MenuSmokeFullMatchEnabled() &&
      EnvironmentManager::GetInstance().GetTime_ms() >= pageCreatedTime_ms + kMenuSmokeAdvanceDelay_ms) {
    autoAdvanceTriggered = true;
    printf("[menu-smoke] Continuing %s automatically\n", PhaseName(nextPhase));
    ContinueGame();
  }
}

void MatchPhasePage::GoGamePlan() {
  Properties properties;
  CreatePage(e_PageID_GamePlan, properties);
}

void MatchPhasePage::ContinueGame() {
  GetMenuTask()->ReleaseAllButtons();
  GetGameTask()->GetMatch()->Pause(false);
  GoBack();
}

void MatchPhasePage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsEscape()) {
    ContinueGame();
    event->Ignore();
  } else {
    Gui2Page::ProcessWindowingEvent(event);
  }
}
