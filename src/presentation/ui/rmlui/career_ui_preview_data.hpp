#ifndef FOTBILER_CAREER_UI_PREVIEW_DATA_HPP
#define FOTBILER_CAREER_UI_PREVIEW_DATA_HPP

#include <string>

#include "data/careerdata.hpp"

namespace blunted::ui {
namespace detail {

inline PlayerCareerState MakePreviewPlayer(int id, const char* name, const char* position,
                                           int overall, int potential, int fitness,
                                           int age, long long value) {
  PlayerCareerState player;
  player.playerID = id;
  player.name = name;
  player.position = position;
  player.preferredPosition = position;
  player.ovr = overall;
  player.pot = potential;
  player.fitness = fitness;
  player.morale = 78;
  player.age = age;
  player.value = value;
  player.contract.yearsRemaining = 3;
  return player;
}

inline InboxItem MakePreviewInboxItem(int id, InboxItemType type, const char* subject,
                                      bool read = false) {
  InboxItem item;
  item.id = id;
  item.type = type;
  item.subject = subject;
  item.read = read;
  return item;
}

}  // namespace detail

// This source deliberately uses CareerSave, the existing career persistence
// model, rather than a second UI-only mock domain. M1A.7 can replace this
// factory with the active loaded save without changing the RmlUi binder.
inline CareerSave BuildCareerPreviewSave() {
  using namespace detail;

  CareerSave save;
  save.name = "FOTBILER FC";
  save.managerName = "EREN";
  save.currentSeason = 2026;
  save.club.clubID = 1;
  save.club.clubName = "FOTBILER FC";
  save.club.leagueName = "FOTBILER PREMIER LEAGUE";
  save.club.stadiumName = "FOTBILER STADIUM";
  save.club.reputation = 76;

  save.transferBudget = 24500000;
  save.finance.cash = 42800000;
  save.finance.transferBudget = 24500000;
  save.finance.wageBudget = 750000;
  save.finance.weeklyWageSpend = 612000;
  save.finances.netWorth = 42800000;
  save.boardConfidence = 78;
  save.board.confidence = 78;

  save.roster = {
      MakePreviewPlayer(10, "EMRE YILMAZ", "CAM", 81, 85, 94, 24, 24000000),
      MakePreviewPlayer(1, "OZAN KOC", "GK", 80, 81, 96, 28, 15000000),
      MakePreviewPlayer(2, "TUNA CELIK", "CB", 79, 82, 91, 26, 18000000),
      MakePreviewPlayer(3, "ARDA GUNES", "CB", 78, 82, 89, 25, 17000000),
      MakePreviewPlayer(4, "KEREM KAYA", "ST", 78, 84, 92, 23, 21000000),
      MakePreviewPlayer(5, "AHMET AKSOY", "CM", 77, 80, 87, 27, 14000000),
      MakePreviewPlayer(6, "DENIZ DEMIR", "LW", 75, 82, 90, 22, 13000000),
      MakePreviewPlayer(7, "MERT ASLAN", "RW", 76, 83, 93, 22, 15000000),
      MakePreviewPlayer(8, "EREN AYDIN", "CM", 74, 79, 90, 24, 10000000),
      MakePreviewPlayer(9, "EMIR CAN", "LB", 72, 78, 92, 23, 8000000),
      MakePreviewPlayer(11, "MERT YALCIN", "RB", 73, 79, 91, 24, 9000000),
  };
  save.squad.roster = save.roster;
  save.squad.startingXIPlayerIDs = {4, 6, 10, 7, 5, 8, 9, 2, 3, 11, 1};
  save.squad.chemistry = 74;
  save.squad.weeklyTrainingFocus = TrainingFocus::SHARPNESS;

  save.activeStrategy = "BALANCED";
  save.season.currentSeason = 2026;
  save.season.currentWeek = 3;
  save.season.maxWeeks = 36;
  save.season.transferWindowOpen = true;
  save.seasonWins = 1;
  save.seasonDraws = 1;
  save.seasonLosses = 0;
  save.seasonGoalsFor = 5;
  save.seasonGoalsAgainst = 2;
  SeasonRecord currentRecord;
  currentRecord.season = 2026;
  currentRecord.teamID = save.club.clubID;
  currentRecord.leaguePosition = 3;
  save.history.push_back(currentRecord);

  save.inbox = {
      MakePreviewInboxItem(1, InboxItemType::SCOUT_REPORT, "Scout report ready"),
      MakePreviewInboxItem(2, InboxItemType::PLAYER_COMPLAINT, "Development update"),
      MakePreviewInboxItem(3, InboxItemType::BOARD_OBJECTIVE, "Monthly review"),
      MakePreviewInboxItem(4, InboxItemType::PRESS_SNIPPET, "Press conference", true),
  };

  save.scouting.activeRegion = "PORTUGAL";
  save.scouting.monthsRemaining = 1;
  ScoutProspect target;
  target.name = "JOAO SILVA";
  target.position = "ST";
  target.age = 22;
  target.ovrEstimateLow = 80;
  target.ovrEstimateHigh = 82;
  target.potEstimateLow = 84;
  target.potEstimateHigh = 87;
  target.askingPrice = 28000000;
  target.region = "PORTUGAL";
  target.reportQuality = 100;
  save.scouting.shortlist.push_back(target);

  save.staff = {
      StaffMember("COACH 1", "COACH", 75, 100000, 2),
      StaffMember("COACH 2", "COACH", 72, 95000, 2),
      StaffMember("COACH 3", "COACH", 70, 90000, 2),
      StaffMember("COACH 4", "COACH", 68, 85000, 2),
      StaffMember("SCOUT 1", "SCOUT", 73, 90000, 2),
      StaffMember("SCOUT 2", "SCOUT", 69, 80000, 2),
      StaffMember("DOCTOR 1", "MEDICAL", 77, 100000, 2),
      StaffMember("DOCTOR 2", "MEDICAL", 71, 90000, 2),
  };
  save.fanBase = 68;
  save.stadium.capacity = 28000;

  return save;
}

}  // namespace blunted::ui

#endif  // FOTBILER_CAREER_UI_PREVIEW_DATA_HPP
