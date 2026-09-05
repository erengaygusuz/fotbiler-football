#include "gameover.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

#include "../../data/matchhistory.hpp"
#include "../../league/leaguecode.hpp"
#include "../../onthepitch/match.hpp"
#include "../career/career_database.hpp"
#include "../pagefactory.hpp"
#include "main.hpp"
#include "utils/gui2/events.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

constexpr unsigned long kMenuSmokeQuitDelay_ms = 1000;

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

Gui2Caption* MakeStat(Gui2WindowManager* windowManager, const std::string& id,
                      const std::string& text, bool accent = false) {
  Gui2Caption* caption = new Gui2Caption(windowManager, id, 0, 0, 16, 3, text);
  if (accent) {
    caption->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  }
  return caption;
}

void SaveMatchHistoryIfNeeded(Match* match) {
  if (!match || !match->GetMatchData() || match->GetMatchData()->IsHistorySaved()) {
    return;
  }

  MatchData* md = match->GetMatchData();
  md->SetHistorySaved(true);

  const float poss1 = md->GetPossessionTime_ms(0);
  const float poss2 = md->GetPossessionTime_ms(1);
  const float totalPoss = poss1 + poss2;

  MatchHistoryEntry entry;
  entry.id = 0;

  time_t now = time(nullptr);
  char tsbuf[32];
  std::tm localTime = {};
  if (blunted::GetLocalTime(now, localTime) &&
      strftime(tsbuf, sizeof(tsbuf), "%Y-%m-%d %H:%M:%S", &localTime) > 0) {
    entry.timestamp = tsbuf;
  } else {
    entry.timestamp = "1970-01-01 00:00:00";
  }

  entry.team1_name = match->GetTeam(0)->GetTeamData()->GetName();
  entry.team2_name = match->GetTeam(1)->GetTeamData()->GetName();
  entry.score1 = md->GetGoalCount(0);
  entry.score2 = md->GetGoalCount(1);
  entry.match_time_ms = static_cast<int>(match->GetMatchTime_ms());
  entry.possession1_pct = totalPoss > 0 ? poss1 / totalPoss * 100.0f : 50.0f;
  entry.possession2_pct = totalPoss > 0 ? poss2 / totalPoss * 100.0f : 50.0f;
  entry.shots1 = md->GetShots(0);
  entry.shots2 = md->GetShots(1);
  entry.shots_on_target1 = md->GetShotsOnTarget(0);
  entry.shots_on_target2 = md->GetShotsOnTarget(1);
  entry.passes1 = md->GetPassAttempts(0);
  entry.passes2 = md->GetPassAttempts(1);
  entry.passes_completed1 = md->GetPassesCompleted(0);
  entry.passes_completed2 = md->GetPassesCompleted(1);
  entry.fouls1 = md->GetFouls(0);
  entry.fouls2 = md->GetFouls(1);

  MatchHistory::EnsureTable();
  MatchHistory::SaveMatch(entry);
}

}  // namespace

GameOverPage::GameOverPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      match(GetGameTask()->GetMatch()),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoQuitTriggered(false) {
  if (!match || !match->GetMatchData()) {
    return;
  }
  match->Pause(true);

  MatchData* md = match->GetMatchData();
  const std::string homeName = match->GetTeam(0)->GetTeamData()->GetName();
  const std::string awayName = match->GetTeam(1)->GetTeamData()->GetName();
  const int homeGoals = md->GetGoalCount(0);
  const int awayGoals = md->GetGoalCount(1);

  Gui2Frame* frame = new Gui2Frame(windowManager, "gameover_frame", 5, 4, 90, 92, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* kicker = new Gui2Caption(windowManager, "caption_gameover_kicker", 3, 2, 84, 2.3f,
                                        "MATCHDAY  ·  FINAL");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(kicker);
  kicker->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_gameover_title", 3, 5, 84, 4,
                                       Localization::GetInstance().Translate("ingame_fulltime"));
  frame->AddView(title);
  title->Show();

  const std::string scoreStr = homeName + "   " + int_to_str(homeGoals) + " - " +
                               int_to_str(awayGoals) + "   " + awayName;
  Gui2Caption* score = new Gui2Caption(windowManager, "caption_gameover_header", 3, 10, 84, 5,
                                       scoreStr);
  score->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(score);
  score->Show();

  Gui2Frame* statsPanel = new Gui2Frame(windowManager, "gameover_stats_panel", 3, 18, 56, 57, true);
  frame->AddView(statsPanel);
  statsPanel->Show();

  Gui2Caption* factsTitle =
      new Gui2Caption(windowManager, "caption_gameover_facts", 2, 2, 52, 3, "MATCH FACTS");
  statsPanel->AddView(factsTitle);
  factsTitle->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "grid_gameover_stats", 2, 8, 52, 43);
  const float p0 = md->GetPossessionTime_ms(0);
  const float p1 = md->GetPossessionTime_ms(1);
  const float totalPoss = p0 + p1;
  const int pos0 = totalPoss > 0 ? static_cast<int>(std::round(p0 / totalPoss * 100.0f)) : 50;
  const int pos1 = 100 - pos0;

  const int attempts0 = md->GetPassAttempts(0);
  const int attempts1 = md->GetPassAttempts(1);
  const int pass0 = attempts0 > 0 ? static_cast<int>(std::round(md->GetPassesCompleted(0) * 100.0f / attempts0)) : 0;
  const int pass1 = attempts1 > 0 ? static_cast<int>(std::round(md->GetPassesCompleted(1) * 100.0f / attempts1)) : 0;

  grid->AddView(MakeStat(windowManager, "ft_h_home", md->GetTeamData(0)->GetShortName(), true), 0, 0);
  grid->AddView(MakeStat(windowManager, "ft_h_mid", "MATCH STATS", true), 1, 0);
  grid->AddView(MakeStat(windowManager, "ft_h_away", md->GetTeamData(1)->GetShortName(), true), 2, 0);

  auto addRow = [&](int row, const std::string& left, const std::string& label,
                    const std::string& right, const std::string& suffix) {
    grid->AddView(MakeStat(windowManager, "ft_l_" + suffix, left), 0, row);
    grid->AddView(MakeStat(windowManager, "ft_m_" + suffix, label), 1, row);
    grid->AddView(MakeStat(windowManager, "ft_r_" + suffix, right), 2, row);
  };

  addRow(1, int_to_str(pos0) + "%", "POSSESSION", int_to_str(pos1) + "%", "pos");
  addRow(2, int_to_str(md->GetShots(0)), "SHOTS", int_to_str(md->GetShots(1)), "shots");
  addRow(3, int_to_str(md->GetShotsOnTarget(0)), "ON TARGET",
         int_to_str(md->GetShotsOnTarget(1)), "target");
  addRow(4, int_to_str(pass0) + "%", "PASS ACCURACY", int_to_str(pass1) + "%", "pass");
  addRow(5, int_to_str(md->GetFouls(0)), "FOULS", int_to_str(md->GetFouls(1)), "fouls");
  grid->UpdateLayout(0.55f);
  statsPanel->AddView(grid);
  grid->Show();

  Gui2Frame* actionPanel = new Gui2Frame(windowManager, "gameover_action_panel", 61, 18, 26, 57, true);
  frame->AddView(actionPanel);
  actionPanel->Show();

  Gui2Caption* summaryKicker =
      new Gui2Caption(windowManager, "caption_gameover_summary_kicker", 2, 2, 22, 2.3f,
                      "PLAYER OF THE MATCH");
  summaryKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  actionPanel->AddView(summaryKicker);
  summaryKicker->Show();

  Gui2Caption* summary =
      new Gui2Caption(windowManager, "caption_gameover_summary", 2, 7, 22, 5,
                      homeGoals > awayGoals ? homeName : (awayGoals > homeGoals ? awayName : "DRAW"));
  actionPanel->AddView(summary);
  summary->Show();

  buttonOkay = new Gui2Button(windowManager, "button_gameover_ok", 2, 22, 22, 4,
                              Localization::GetInstance().Translate("gameover_continue"));
  buttonOkay->sig_OnClick.connect([this](...) { GoMainMenu(); });
  actionPanel->AddView(buttonOkay);
  buttonOkay->Show();

  Gui2Button* buttonHistory =
      new Gui2Button(windowManager, "button_gameover_history", 2, 29, 22, 4,
                     Localization::GetInstance().Translate("gameover_match_history"));
  buttonHistory->sig_OnClick.connect([this](...) {
    Properties props;
    CreatePage((int)e_PageID_MatchHistory, props);
  });
  actionPanel->AddView(buttonHistory);
  buttonHistory->Show();

  Gui2Caption* footer = new Gui2Caption(windowManager, "caption_gameover_footer", 3, 82, 82, 2.3f,
                                        "ENTER Continue  ·  Match result will be saved automatically");
  frame->AddView(footer);
  footer->Show();

  SaveMatchHistoryIfNeeded(match);
  buttonOkay->SetFocus();
  this->Show();
}

GameOverPage::~GameOverPage() {}

void GameOverPage::Process() {
  Gui2Page::Process();
  if (!autoQuitTriggered && MenuSmokeFullMatchEnabled() &&
      EnvironmentManager::GetInstance().GetTime_ms() >= pageCreatedTime_ms + kMenuSmokeQuitDelay_ms) {
    autoQuitTriggered = true;
    printf("[menu-smoke] Full match complete: %s %i - %i %s\n",
           match->GetTeam(0)->GetTeamData()->GetName().c_str(),
           match->GetMatchData()->GetGoalCount(0), match->GetMatchData()->GetGoalCount(1),
           match->GetTeam(1)->GetTeamData()->GetName().c_str());
    printf("[menu-smoke] Full-match verification succeeded, quitting test run\n");
    EnvironmentManager::GetInstance().SignalQuit();
  }
}

void GameOverPage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsEscape()) {
    GoMainMenu();
    return;
  }
  event->Ignore();
}

void GameOverPage::GoRematch() {
  windowManager->GetPagePath()->Clear();
  GetGameTask()->Action(e_GameTaskMessage_StopMatch);
  GetGameTask()->Action(e_GameTaskMessage_StartMatch);
  this->Exit();
  Properties properties;
  windowManager->GetPageFactory()->CreatePage((int)e_PageID_Game, properties, 0);
  delete this;
}

void GameOverPage::GoMainMenu() {
  bool leagueMatchPlayed = false;
  if (match && LeagueHasPendingFixture()) {
    MatchData* md = match->GetMatchData();
    if (md) {
      leagueMatchPlayed = LeagueConsumePlayedFixture(md->GetGoalCount(0), md->GetGoalCount(1));
    } else {
      LeagueClearPendingFixture();
    }
  }

  bool resumeCareer = false;
  if (!leagueMatchPlayed && CareerDatabase::GetInstance().HasPendingFixture()) {
    MatchData* md = match ? match->GetMatchData() : nullptr;
    if (md) {
      resumeCareer = CareerDatabase::GetInstance().ConsumePlayedFixture(md->GetGoalCount(0),
                                                                        md->GetGoalCount(1));
    } else {
      CareerDatabase::GetInstance().ClearPendingFixture();
    }
  }
  if (resumeCareer) GetConfiguration()->SetBool("career_resume_hub", true);
  if (leagueMatchPlayed) GetConfiguration()->SetBool("league_resume_hub", true);

  this->Exit();
  GetMenuTask()->SetMenuAction(e_MenuAction_Menu);
  delete this;
}
