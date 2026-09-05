#ifndef FOTBILER_CAREER_DETAIL_VIEW_MODEL_HPP
#define FOTBILER_CAREER_DETAIL_VIEW_MODEL_HPP

#include <algorithm>
#include <string>
#include <vector>

#include "data/careerdata.hpp"
#include "presentation/ui/rmlui/career_ui_view_model.hpp"

namespace blunted::ui {

struct CareerFinanceDetailView {
  std::string cash;
  std::string transferBudget;
  std::string wageBudget;
  std::string weeklyWageSpend;
  std::string totalRevenue;
  std::string totalExpenses;
  std::string netWorth;
  std::string matchDayIncome;
  std::string sponsorIncome;
  std::string merchandiseIncome;
  std::string tvRevenue;
  std::string transferSpending;
  std::string transferIncome;
  int debtLevel = 0;
};

struct CareerContractRowView {
  std::string playerName;
  std::string position;
  int overall = 0;
  int yearsRemaining = 0;
  std::string wage;
  std::string releaseClause;
  bool transferListed = false;
};

struct CareerContractsDetailView {
  std::vector<CareerContractRowView> players;
  int expiringSoon = 0;
  std::string totalWeeklyWages;
};

struct CareerStaffRowView {
  std::string name;
  std::string role;
  int skill = 0;
  int morale = 0;
  int yearsRemaining = 0;
  std::string salary;
};

struct CareerStaffDetailView {
  std::vector<CareerStaffRowView> members;
  int count = 0;
  std::string totalPayroll;
};

struct CareerYouthRowView {
  std::string name;
  std::string position;
  int age = 0;
  int overall = 0;
  int potential = 0;
  int fitness = 0;
};

struct CareerYouthDetailView {
  std::string regionFocus;
  int monthlyIntakeSize = 0;
  std::vector<CareerYouthRowView> prospects;
  int promotedCount = 0;
};

struct CareerScoutingDetailView {
  std::string activeRegion;
  int monthsRemaining = 0;
  int shortlistCount = 0;
  int discoveredCount = 0;
  std::vector<CareerTransferTargetView> prospects;
};

struct CareerCompetitionDetailView {
  std::string leagueName;
  int position = 0;
  int points = 0;
  int played = 0;
  int wins = 0;
  int draws = 0;
  int losses = 0;
  int goalsFor = 0;
  int goalsAgainst = 0;
  int progressPercent = 0;
};

struct CareerFixtureDetailView {
  int fixtureId = 0;
  int homeTeamId = 0;
  int awayTeamId = 0;
  int homeGoals = 0;
  int awayGoals = 0;
  bool played = false;
};

struct CareerCalendarDetailView {
  int currentWeek = 1;
  int maxWeeks = 38;
  std::vector<CareerFixtureDetailView> fixtures;
};

struct CareerDetailViewModel {
  CareerFinanceDetailView finances;
  CareerContractsDetailView contracts;
  CareerStaffDetailView staff;
  CareerYouthDetailView youth;
  CareerScoutingDetailView scouting;
  CareerCompetitionDetailView competition;
  CareerCalendarDetailView calendar;
};

inline CareerDetailViewModel BuildCareerDetailViewModel(const CareerSave& save) {
  CareerDetailViewModel view;

  view.finances.cash = FormatCareerMoney(save.finance.cash);
  view.finances.transferBudget = FormatCareerMoney(
      save.finance.transferBudget != 0 ? save.finance.transferBudget : save.transferBudget);
  view.finances.wageBudget = FormatCareerMoney(
      save.finance.wageBudget != 0 ? save.finance.wageBudget : save.wageBudget);
  view.finances.weeklyWageSpend = FormatCareerMoney(save.finance.weeklyWageSpend);
  view.finances.totalRevenue = FormatCareerMoney(save.finances.totalRevenue);
  view.finances.totalExpenses = FormatCareerMoney(save.finances.totalExpenses);
  view.finances.netWorth = FormatCareerMoney(save.finances.netWorth);
  view.finances.matchDayIncome = FormatCareerMoney(save.finances.matchDayIncome);
  view.finances.sponsorIncome = FormatCareerMoney(save.finances.sponsorIncome);
  view.finances.merchandiseIncome = FormatCareerMoney(save.finances.merchandiseIncome);
  view.finances.tvRevenue = FormatCareerMoney(save.finances.tvRevenue);
  view.finances.transferSpending = FormatCareerMoney(save.finances.transferSpending);
  view.finances.transferIncome = FormatCareerMoney(save.finances.transferIncome);
  view.finances.debtLevel = save.finances.debtLevel;

  const std::vector<PlayerCareerState>& roster =
      !save.squad.roster.empty() ? save.squad.roster : save.roster;
  view.contracts.players.reserve(roster.size());
  long long totalWeeklyWages = 0;
  for (const PlayerCareerState& player : roster) {
    CareerContractRowView contract;
    contract.playerName = player.name;
    contract.position = !player.preferredPosition.empty() ? player.preferredPosition
                                                           : player.position;
    contract.overall = player.ovr;
    contract.yearsRemaining = player.contract.yearsRemaining;
    contract.wage = FormatCareerMoney(
        player.contract.wage != 0 ? player.contract.wage : player.wage);
    contract.releaseClause = FormatCareerMoney(player.contract.releaseClause);
    contract.transferListed = player.contract.transferListed;
    if (contract.yearsRemaining <= 1) {
      ++view.contracts.expiringSoon;
    }
    totalWeeklyWages += player.contract.wage != 0 ? player.contract.wage : player.wage;
    view.contracts.players.push_back(std::move(contract));
  }
  std::stable_sort(view.contracts.players.begin(), view.contracts.players.end(),
                   [](const CareerContractRowView& left, const CareerContractRowView& right) {
                     return left.yearsRemaining < right.yearsRemaining;
                   });
  view.contracts.totalWeeklyWages = FormatCareerMoney(totalWeeklyWages);

  long long staffPayroll = 0;
  view.staff.members.reserve(save.staff.size());
  for (const StaffMember& member : save.staff) {
    CareerStaffRowView row;
    row.name = member.name;
    row.role = member.role;
    row.skill = member.skill;
    row.morale = member.morale;
    row.yearsRemaining = member.contractYearsRemaining;
    row.salary = FormatCareerMoney(member.salary);
    staffPayroll += member.salary;
    view.staff.members.push_back(std::move(row));
  }
  view.staff.count = static_cast<int>(view.staff.members.size());
  view.staff.totalPayroll = FormatCareerMoney(staffPayroll);

  view.youth.regionFocus = save.youth.regionFocus;
  view.youth.monthlyIntakeSize = save.youth.monthlyIntakeSize;
  view.youth.promotedCount = static_cast<int>(save.youth.promotedPlayers.size());
  view.youth.prospects.reserve(save.youth.prospects.size());
  for (const PlayerCareerState& player : save.youth.prospects) {
    CareerYouthRowView row;
    row.name = player.name;
    row.position = !player.preferredPosition.empty() ? player.preferredPosition
                                                      : player.position;
    row.age = player.age;
    row.overall = player.ovr;
    row.potential = player.pot;
    row.fitness = player.fitness;
    view.youth.prospects.push_back(std::move(row));
  }

  view.scouting.activeRegion = save.scouting.activeRegion;
  view.scouting.monthsRemaining = save.scouting.monthsRemaining;
  view.scouting.shortlistCount = static_cast<int>(save.scouting.shortlist.size());
  view.scouting.discoveredCount = static_cast<int>(save.scouting.discoveredProspects.size());
  view.scouting.prospects.reserve(save.scouting.discoveredProspects.size());
  for (const ScoutProspect& prospect : save.scouting.discoveredProspects) {
    CareerTransferTargetView row;
    row.name = prospect.name;
    row.position = prospect.position;
    row.region = prospect.region;
    row.age = prospect.age;
    row.overallLow = prospect.ovrEstimateLow;
    row.overallHigh = prospect.ovrEstimateHigh;
    row.potentialLow = prospect.potEstimateLow;
    row.potentialHigh = prospect.potEstimateHigh;
    row.reportQuality = prospect.reportQuality;
    row.askingPrice = FormatCareerMoney(prospect.askingPrice);
    view.scouting.prospects.push_back(std::move(row));
  }
  if (view.scouting.prospects.empty()) {
    for (const ScoutProspect& prospect : save.scouting.shortlist) {
      CareerTransferTargetView row;
      row.name = prospect.name;
      row.position = prospect.position;
      row.region = prospect.region;
      row.age = prospect.age;
      row.overallLow = prospect.ovrEstimateLow;
      row.overallHigh = prospect.ovrEstimateHigh;
      row.potentialLow = prospect.potEstimateLow;
      row.potentialHigh = prospect.potEstimateHigh;
      row.reportQuality = prospect.reportQuality;
      row.askingPrice = FormatCareerMoney(prospect.askingPrice);
      view.scouting.prospects.push_back(std::move(row));
    }
  }

  view.competition.leagueName = save.club.leagueName;
  view.competition.position = !save.history.empty() ? save.history.back().leaguePosition : 0;
  view.competition.wins = save.seasonWins;
  view.competition.draws = save.seasonDraws;
  view.competition.losses = save.seasonLosses;
  view.competition.points = save.seasonWins * 3 + save.seasonDraws;
  view.competition.played = save.seasonWins + save.seasonDraws + save.seasonLosses;
  view.competition.goalsFor = save.seasonGoalsFor;
  view.competition.goalsAgainst = save.seasonGoalsAgainst;
  view.competition.progressPercent = std::clamp(
      (save.season.currentWeek * 100) / std::max(1, save.season.maxWeeks), 0, 100);

  view.calendar.currentWeek = save.season.currentWeek;
  view.calendar.maxWeeks = std::max(1, save.season.maxWeeks);
  view.calendar.fixtures.reserve(save.season.fixtures.size());
  for (const FixtureResult& fixture : save.season.fixtures) {
    CareerFixtureDetailView row;
    row.fixtureId = fixture.fixtureID;
    row.homeTeamId = fixture.homeTeamID;
    row.awayTeamId = fixture.awayTeamID;
    row.homeGoals = fixture.homeGoals;
    row.awayGoals = fixture.awayGoals;
    row.played = fixture.played;
    view.calendar.fixtures.push_back(row);
  }

  return view;
}

}  // namespace blunted::ui

#endif  // FOTBILER_CAREER_DETAIL_VIEW_MODEL_HPP
