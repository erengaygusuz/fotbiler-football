#include "scoreboard.hpp"

#include "../../onthepitch/match.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

Gui2ScoreBoard::Gui2ScoreBoard(Gui2WindowManager* windowManager, Match* match)
    : Gui2View(windowManager, "scoreboard", 25, 1.5f, 50, 5.5f), match(match) {
  x_percent = 25;
  y_percent = 1.5f;
  width_percent = 50;
  height_percent = 5.5f;

  Vector3 textColor = 255;
  Vector3 accentColor = windowManager->GetStyle()->GetColor(e_DecorationType_Bright2);
  Vector3 textOutlineColor = 0;
  goalCount[0] = 0;
  goalCount[1] = 0;

  // Keep the existing textured broadcast plate but make it compact and centered,
  // closer to modern FIFA-era scorebugs instead of a full-screen-width ribbon.
  constexpr float kScoreboardBackgroundAspectRatio = 1024.0f / 64.0f;
  const float bgW = windowManager->GetWidthPercentForHeight(height_percent,
                                                            kScoreboardBackgroundAspectRatio);
  const float bgX = (width_percent - bgW) * 0.5f;
  Gui2Image* bg = new Gui2Image(windowManager, "image_scoreboard_bg", bgX, 0, bgW, height_percent);
  bg->LoadImage("media/menu/scoreboard_bg.png");
  this->AddView(bg);
  bg->Show();

  const float logoW = windowManager->GetWidthPercentForHeight(height_percent * 0.78f, 1.0f);
  const float logoY = height_percent * 0.11f;
  const float logoH = height_percent * 0.78f;

  leagueLogo = new Gui2Image(windowManager, "game_scoreboard_leaguelogo", bgX + bgW * 0.018f,
                             logoY, logoW, logoH);
  leagueLogo->LoadImage("media/menu/league.png");
  this->AddView(leagueLogo);
  leagueLogo->Show();

  const float timeX = bgX + bgW * 0.09f;
  timeCaption = new Gui2Caption(windowManager, "game_scoreboard_timecaption", timeX, 0.4f,
                                bgW * 0.13f, height_percent * 0.75f, "0:00");
  timeCaption->SetColor(accentColor);
  timeCaption->SetOutlineColor(textOutlineColor);
  this->AddView(timeCaption);
  timeCaption->Show();

  const float team1LogoX = bgX + bgW * 0.235f;
  teamLogo[0] = new Gui2Image(windowManager, "game_scoreboard_team1logo", team1LogoX, logoY,
                              logoW, logoH);
  teamLogo[0]->LoadImage(match->GetTeam(0)->GetTeamData()->GetLogoUrl());
  this->AddView(teamLogo[0]);
  teamLogo[0]->Show();

  teamNameCaption[0] = new Gui2Caption(windowManager, "game_scoreboard_team1name",
                                       team1LogoX + logoW + bgW * 0.012f, 0.4f,
                                       bgW * 0.13f, height_percent * 0.75f,
                                       match->GetTeam(0)->GetTeamData()->GetShortName());

  goalCountCaption[0] = new Gui2Caption(windowManager, "game_scoreboard_team1goals",
                                        bgX + bgW * 0.445f, 0.2f, bgW * 0.045f,
                                        height_percent * 0.82f, "0");
  goalCountCaption[1] = new Gui2Caption(windowManager, "game_scoreboard_team2goals",
                                        bgX + bgW * 0.515f, 0.2f, bgW * 0.045f,
                                        height_percent * 0.82f, "0");

  teamNameCaption[1] = new Gui2Caption(windowManager, "game_scoreboard_team2name",
                                       bgX + bgW * 0.575f, 0.4f, bgW * 0.13f,
                                       height_percent * 0.75f,
                                       match->GetTeam(1)->GetTeamData()->GetShortName());

  const float team2LogoX = bgX + bgW * 0.74f;
  teamLogo[1] = new Gui2Image(windowManager, "game_scoreboard_team2logo", team2LogoX, logoY,
                              logoW, logoH);
  teamLogo[1]->LoadImage(match->GetTeam(1)->GetTeamData()->GetLogoUrl());
  this->AddView(teamLogo[1]);
  teamLogo[1]->Show();

  const float tvW = windowManager->GetWidthPercentForHeight(height_percent * 0.62f, 2.0f);
  tvLogo = new Gui2Image(windowManager, "game_scoreboard_tvlogo", bgX + bgW - tvW - bgW * 0.018f,
                         height_percent * 0.19f, tvW, height_percent * 0.62f);
  tvLogo->LoadImage("media/menu/tvlogo.png");
  this->AddView(tvLogo);
  tvLogo->Show();

  for (int i = 0; i < 2; ++i) {
    teamNameCaption[i]->SetColor(textColor);
    teamNameCaption[i]->SetOutlineColor(textOutlineColor);
    goalCountCaption[i]->SetColor(textColor);
    goalCountCaption[i]->SetOutlineColor(textOutlineColor);
    this->AddView(teamNameCaption[i]);
    teamNameCaption[i]->Show();
    this->AddView(goalCountCaption[i]);
    goalCountCaption[i]->Show();
  }

  SetGoalCount(0, 0);
  SetGoalCount(1, 0);
  this->Show();
}

Gui2ScoreBoard::~Gui2ScoreBoard() {}

void Gui2ScoreBoard::GetImages(std::vector<boost::intrusive_ptr<Image2D>>& target) {
  Gui2View::GetImages(target);
}

void Gui2ScoreBoard::Redraw() {}

void Gui2ScoreBoard::SetTimeStr(const std::string& timeStr) {
  this->timeStr = timeStr;
  timeCaption->SetCaption(timeStr);
}

void Gui2ScoreBoard::SetGoalCount(int teamID, int goalCount) {
  this->goalCount[teamID] = goalCount;
  goalCountCaption[teamID]->SetCaption(int_to_str(goalCount));
}
