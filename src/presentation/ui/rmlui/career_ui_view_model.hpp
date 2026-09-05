#ifndef FOTBILER_CAREER_UI_VIEW_MODEL_HPP
#define FOTBILER_CAREER_UI_VIEW_MODEL_HPP

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "data/careerdata.hpp"

namespace blunted::ui {

struct CareerHeaderView {
  std::string clubName;
  std::string managerName;
  std::string seasonLabel;
  std::string leagueName;
  std::string transferBudget;
  int boardConfidence = 0;
  int unreadMessages = 0;
};

struct CareerPlayerView {
  int id = 0;
  std::string name;
  std::string position;
  int overall = 0;
  int potential = 0;
  int fitness = 0;
  int morale = 0;
  int age = 0;
  std::string value;
  int contractYears = 0;
  bool inStartingXI = false;
};

struct CareerSquadView {
  std::vector<CareerPlayerView> players;
  std::vector<int> startingPlayerIds;
  int selectedPlayerId = 0;
  std::string formation = "4-2-3-1";
  int chemistry = 0;
};

struct CareerSeasonView {
  int season = 1;
  int week = 1;
  int maxWeeks = 38;
  int wins = 0;
  int draws = 0;
  int losses = 0;
  int goalsFor = 0;
  int goalsAgainst = 0;
  int points = 0;
  int progressPercent = 0;
  int leaguePosition = 0;
  bool transferWindowOpen = false;
};

struct CareerTransferTargetView {
  std::string name;
  std::string position;
  std::string region;
  int age = 0;
  int overallLow = 0;
  int overallHigh = 0;
  int potentialLow = 0;
  int potentialHigh = 0;
  int reportQuality = 0;
  std::string askingPrice;
};

struct CareerTransfersView {
  std::vector<CareerTransferTargetView> shortlist;
  std::string activeRegion;
  int scoutingMonthsRemaining = 0;
};

struct CareerTacticsView {
  std::string activeStrategy;
  std::string trainingFocus;
  int chemistry = 0;
};

struct CareerOfficeView {
  int boardConfidence = 0;
  int unreadMessages = 0;
  std::string clubBalance;
  std::string transferBudget;
  std::string weeklyWageSpend;
  int staffCount = 0;
  int reputation = 0;
  int fanBase = 0;
  int stadiumCapacity = 0;
};

struct CareerUiViewModel {
  CareerHeaderView header;
  CareerSquadView squad;
  CareerSeasonView season;
  CareerTransfersView transfers;
  CareerTacticsView tactics;
  CareerOfficeView office;
};

inline std::string FormatCareerMoney(long long value) {
  const bool negative = value < 0;
  const long long magnitude = negative ? -value : value;
  std::ostringstream out;
  if (negative) {
    out << '-';
  }
  out << "€";

  if (magnitude >= 1000000) {
    out << std::fixed << std::setprecision(1)
        << static_cast<double>(magnitude) / 1000000.0 << 'M';
  } else if (magnitude >= 1000) {
    out << std::fixed << std::setprecision(0)
        << static_cast<double>(magnitude) / 1000.0 << 'K';
  } else {
    out << magnitude;
  }
  return out.str();
}

inline std::string FormatCareerSeasonLabel(int season) {
  if (season >= 1900 && season < 9999) {
    const int nextYear = (season + 1) % 100;
    std::ostringstream out;
    out << "SEASON " << season << '/' << std::setw(2) << std::setfill('0') << nextYear;
    return out.str();
  }
  return "SEASON " + std::to_string(std::max(1, season));
}

inline std::string TrainingFocusLabel(TrainingFocus focus) {
  switch (focus) {
    case TrainingFocus::FITNESS:
      return "FITNESS";
    case TrainingFocus::SHARPNESS:
      return "SHARPNESS";
    case TrainingFocus::ATTACKING:
      return "ATTACKING";
    case TrainingFocus::DEFENDING:
      return "DEFENDING";
    case TrainingFocus::SET_PIECES:
      return "SET PIECES";
    case TrainingFocus::YOUTH:
      return "YOUTH";
  }
  return "SHARPNESS";
}

inline CareerUiViewModel BuildCareerUiViewModel(const CareerSave& save) {
  CareerUiViewModel view;

  view.header.clubName = save.club.clubName.empty() ? save.name : save.club.clubName;
  if (view.header.clubName.empty()) {
    view.header.clubName = "UNNAMED CLUB";
  }
  view.header.managerName = save.managerName;
  view.header.seasonLabel = FormatCareerSeasonLabel(save.currentSeason);
  view.header.leagueName = save.club.leagueName;
  view.header.transferBudget = FormatCareerMoney(
      save.finance.transferBudget != 0 ? save.finance.transferBudget : save.transferBudget);
  view.header.boardConfidence =
      save.board.confidence != 50 ? save.board.confidence : save.boardConfidence;
  view.header.unreadMessages = static_cast<int>(std::count_if(
      save.inbox.begin(), save.inbox.end(), [](const InboxItem& item) { return !item.read; }));

  const std::vector<PlayerCareerState>& roster =
      !save.squad.roster.empty() ? save.squad.roster : save.roster;
  const std::unordered_set<int> startingIds(save.squad.startingXIPlayerIDs.begin(),
                                            save.squad.startingXIPlayerIDs.end());
  view.squad.startingPlayerIds = save.squad.startingXIPlayerIDs;
  view.squad.players.reserve(roster.size());
  for (const PlayerCareerState& player : roster) {
    CareerPlayerView playerView;
    playerView.id = player.playerID;
    playerView.name = player.name;
    playerView.position = !player.preferredPosition.empty() ? player.preferredPosition
                                                            : player.position;
    playerView.overall = player.ovr;
    playerView.potential = player.pot;
    playerView.fitness = player.fitness;
    playerView.morale = player.morale;
    playerView.age = player.age;
    playerView.value = FormatCareerMoney(player.value);
    playerView.contractYears = player.contract.yearsRemaining;
    playerView.inStartingXI = startingIds.find(player.playerID) != startingIds.end();
    view.squad.players.push_back(std::move(playerView));
  }

  if (!view.squad.players.empty()) {
    const auto selected = std::max_element(
        view.squad.players.begin(), view.squad.players.end(),
        [](const CareerPlayerView& left, const CareerPlayerView& right) {
          return left.overall < right.overall;
        });
    view.squad.selectedPlayerId = selected->id;
  }
  view.squad.chemistry = save.squad.chemistry;

  view.season.season = save.season.currentSeason > 0 ? save.season.currentSeason
                                                     : save.currentSeason;
  view.season.week = save.season.currentWeek;
  view.season.maxWeeks = std::max(1, save.season.maxWeeks);
  view.season.wins = save.seasonWins;
  view.season.draws = save.seasonDraws;
  view.season.losses = save.seasonLosses;
  view.season.goalsFor = save.seasonGoalsFor;
  view.season.goalsAgainst = save.seasonGoalsAgainst;
  view.season.points = view.season.wins * 3 + view.season.draws;
  view.season.progressPercent = std::clamp(
      (view.season.week * 100) / view.season.maxWeeks, 0, 100);
  view.season.transferWindowOpen = save.season.transferWindowOpen;
  if (!save.history.empty()) {
    view.season.leaguePosition = save.history.back().leaguePosition;
  }

  view.transfers.activeRegion = save.scouting.activeRegion;
  view.transfers.scoutingMonthsRemaining = save.scouting.monthsRemaining;
  view.transfers.shortlist.reserve(save.scouting.shortlist.size());
  for (const ScoutProspect& prospect : save.scouting.shortlist) {
    CareerTransferTargetView target;
    target.name = prospect.name;
    target.position = prospect.position;
    target.region = prospect.region;
    target.age = prospect.age;
    target.overallLow = prospect.ovrEstimateLow;
    target.overallHigh = prospect.ovrEstimateHigh;
    target.potentialLow = prospect.potEstimateLow;
    target.potentialHigh = prospect.potEstimateHigh;
    target.reportQuality = prospect.reportQuality;
    target.askingPrice = FormatCareerMoney(prospect.askingPrice);
    view.transfers.shortlist.push_back(std::move(target));
  }

  view.tactics.activeStrategy = save.activeStrategy.empty() ? "Balanced" : save.activeStrategy;
  view.tactics.trainingFocus = TrainingFocusLabel(save.squad.weeklyTrainingFocus);
  view.tactics.chemistry = save.squad.chemistry;

  view.office.boardConfidence = view.header.boardConfidence;
  view.office.unreadMessages = view.header.unreadMessages;
  view.office.clubBalance = FormatCareerMoney(
      save.finance.cash != 0 ? save.finance.cash : save.finances.netWorth);
  view.office.transferBudget = view.header.transferBudget;
  view.office.weeklyWageSpend = FormatCareerMoney(save.finance.weeklyWageSpend);
  view.office.staffCount = static_cast<int>(save.staff.size());
  view.office.reputation = save.club.reputation;
  view.office.fanBase = save.fanBase;
  view.office.stadiumCapacity = save.stadium.capacity;

  return view;
}

}  // namespace blunted::ui

#endif  // FOTBILER_CAREER_UI_VIEW_MODEL_HPP
