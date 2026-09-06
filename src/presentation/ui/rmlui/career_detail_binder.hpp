#ifndef FOTBILER_CAREER_DETAIL_BINDER_HPP
#define FOTBILER_CAREER_DETAIL_BINDER_HPP

#include <string>

#include "presentation/ui/rmlui/career_detail_view_model.hpp"
#include "presentation/ui/rmlui/rmlui_system.hpp"

namespace blunted::ui {
namespace detail {

inline void BindDetailText(RmlUiSystem& ui, const char* id, const std::string& value) {
  ui.SetElementText(id, value);
}

inline void BindDetailNumber(RmlUiSystem& ui, const char* id, int value) {
  ui.SetElementText(id, std::to_string(value));
}

}  // namespace detail

inline void BindCareerDetailViewModel(RmlUiSystem& ui, const CareerDetailViewModel& view) {
  using namespace detail;

  BindDetailText(ui, "finance-cash", view.finances.cash);
  BindDetailText(ui, "finance-transfer-budget", view.finances.transferBudget);
  BindDetailText(ui, "finance-wage-budget", view.finances.wageBudget);
  BindDetailText(ui, "finance-weekly-wages", view.finances.weeklyWageSpend);
  BindDetailText(ui, "finance-total-revenue", view.finances.totalRevenue);
  BindDetailText(ui, "finance-total-expenses", view.finances.totalExpenses);
  BindDetailText(ui, "finance-net-worth", view.finances.netWorth);
  BindDetailText(ui, "finance-matchday-income", view.finances.matchDayIncome);
  BindDetailText(ui, "finance-sponsor-income", view.finances.sponsorIncome);
  BindDetailText(ui, "finance-merch-income", view.finances.merchandiseIncome);
  BindDetailText(ui, "finance-tv-revenue", view.finances.tvRevenue);
  BindDetailText(ui, "finance-transfer-spending", view.finances.transferSpending);
  BindDetailText(ui, "finance-transfer-income", view.finances.transferIncome);
  BindDetailNumber(ui, "finance-debt-level", view.finances.debtLevel);

  BindDetailNumber(ui, "contracts-expiring-count", view.contracts.expiringSoon);
  BindDetailNumber(ui, "contracts-expiring-count-copy", view.contracts.expiringSoon);
  BindDetailText(ui, "contracts-total-wages", view.contracts.totalWeeklyWages);
  BindDetailText(ui, "contracts-total-wages-copy", view.contracts.totalWeeklyWages);
  for (size_t i = 0; i < view.contracts.players.size() && i < 8; ++i) {
    const CareerContractRowView& player = view.contracts.players[i];
    const std::string suffix = std::to_string(i);
    BindDetailText(ui, ("contract-name-" + suffix).c_str(), player.playerName);
    BindDetailText(ui, ("contract-pos-" + suffix).c_str(), player.position);
    BindDetailNumber(ui, ("contract-ovr-" + suffix).c_str(), player.overall);
    BindDetailNumber(ui, ("contract-years-" + suffix).c_str(), player.yearsRemaining);
    BindDetailText(ui, ("contract-wage-" + suffix).c_str(), player.wage);
  }

  BindDetailNumber(ui, "staff-count", view.staff.count);
  BindDetailText(ui, "staff-payroll", view.staff.totalPayroll);
  BindDetailText(ui, "staff-payroll-copy", view.staff.totalPayroll);
  for (size_t i = 0; i < view.staff.members.size() && i < 8; ++i) {
    const CareerStaffRowView& member = view.staff.members[i];
    const std::string suffix = std::to_string(i);
    BindDetailText(ui, ("staff-name-" + suffix).c_str(), member.name);
    BindDetailText(ui, ("staff-role-" + suffix).c_str(), member.role);
    BindDetailNumber(ui, ("staff-skill-" + suffix).c_str(), member.skill);
    BindDetailNumber(ui, ("staff-morale-" + suffix).c_str(), member.morale);
    BindDetailText(ui, ("staff-salary-" + suffix).c_str(), member.salary);
  }

  BindDetailText(ui, "youth-region", view.youth.regionFocus);
  BindDetailText(ui, "youth-region-copy", view.youth.regionFocus);
  BindDetailNumber(ui, "youth-intake", view.youth.monthlyIntakeSize);
  BindDetailNumber(ui, "youth-promoted", view.youth.promotedCount);
  BindDetailNumber(ui, "youth-count", static_cast<int>(view.youth.prospects.size()));
  for (size_t i = 0; i < view.youth.prospects.size() && i < 8; ++i) {
    const CareerYouthRowView& player = view.youth.prospects[i];
    const std::string suffix = std::to_string(i);
    BindDetailText(ui, ("youth-name-" + suffix).c_str(), player.name);
    BindDetailText(ui, ("youth-pos-" + suffix).c_str(), player.position);
    BindDetailNumber(ui, ("youth-age-" + suffix).c_str(), player.age);
    BindDetailNumber(ui, ("youth-ovr-" + suffix).c_str(), player.overall);
    BindDetailNumber(ui, ("youth-pot-" + suffix).c_str(), player.potential);
  }

  BindDetailText(ui, "scouting-region", view.scouting.activeRegion);
  BindDetailText(ui, "scouting-region-copy", view.scouting.activeRegion);
  BindDetailNumber(ui, "scouting-months", view.scouting.monthsRemaining);
  BindDetailNumber(ui, "scouting-months-copy", view.scouting.monthsRemaining);
  BindDetailNumber(ui, "scouting-shortlist-count", view.scouting.shortlistCount);
  BindDetailNumber(ui, "scouting-discovered-count", view.scouting.discoveredCount);
  for (size_t i = 0; i < view.scouting.prospects.size() && i < 8; ++i) {
    const CareerTransferTargetView& player = view.scouting.prospects[i];
    const std::string suffix = std::to_string(i);
    BindDetailText(ui, ("scout-name-" + suffix).c_str(), player.name);
    BindDetailText(ui, ("scout-pos-" + suffix).c_str(), player.position);
    BindDetailNumber(ui, ("scout-age-" + suffix).c_str(), player.age);
    BindDetailText(ui, ("scout-ovr-" + suffix).c_str(),
                   std::to_string(player.overallLow) + "-" + std::to_string(player.overallHigh));
    BindDetailText(ui, ("scout-pot-" + suffix).c_str(),
                   std::to_string(player.potentialLow) + "-" + std::to_string(player.potentialHigh));
    BindDetailNumber(ui, ("scout-report-" + suffix).c_str(), player.reportQuality);
  }

  BindDetailText(ui, "competition-name", view.competition.leagueName);
  BindDetailNumber(ui, "competition-position", view.competition.position);
  BindDetailNumber(ui, "competition-position-copy", view.competition.position);
  BindDetailNumber(ui, "competition-points", view.competition.points);
  BindDetailNumber(ui, "competition-points-copy", view.competition.points);
  BindDetailNumber(ui, "competition-played", view.competition.played);
  BindDetailNumber(ui, "competition-wins", view.competition.wins);
  BindDetailNumber(ui, "competition-draws", view.competition.draws);
  BindDetailNumber(ui, "competition-losses", view.competition.losses);
  BindDetailNumber(ui, "competition-gf", view.competition.goalsFor);
  BindDetailNumber(ui, "competition-ga", view.competition.goalsAgainst);
  BindDetailNumber(ui, "competition-progress", view.competition.progressPercent);

  BindDetailNumber(ui, "calendar-current-week", view.calendar.currentWeek);
  BindDetailNumber(ui, "calendar-current-week-copy", view.calendar.currentWeek);
  BindDetailNumber(ui, "calendar-max-weeks", view.calendar.maxWeeks);
  BindDetailNumber(ui, "calendar-fixture-count", static_cast<int>(view.calendar.fixtures.size()));
  for (size_t i = 0; i < view.calendar.fixtures.size() && i < 8; ++i) {
    const CareerFixtureDetailView& fixture = view.calendar.fixtures[i];
    const std::string suffix = std::to_string(i);
    BindDetailNumber(ui, ("calendar-fixture-id-" + suffix).c_str(), fixture.fixtureId);
    BindDetailNumber(ui, ("calendar-home-id-" + suffix).c_str(), fixture.homeTeamId);
    BindDetailNumber(ui, ("calendar-away-id-" + suffix).c_str(), fixture.awayTeamId);
    BindDetailText(ui, ("calendar-score-" + suffix).c_str(),
                   fixture.played ? std::to_string(fixture.homeGoals) + "-" +
                                        std::to_string(fixture.awayGoals)
                                  : "UPCOMING");
  }
}

}  // namespace blunted::ui

#endif  // FOTBILER_CAREER_DETAIL_BINDER_HPP
