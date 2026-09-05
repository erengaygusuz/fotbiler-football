#ifndef FOTBILER_CAREER_UI_BINDER_HPP
#define FOTBILER_CAREER_UI_BINDER_HPP

#include <algorithm>
#include <string>
#include <vector>

#include "presentation/ui/rmlui/career_ui_view_model.hpp"
#include "presentation/ui/rmlui/rmlui_system.hpp"

namespace blunted::ui {
namespace detail {

inline void BindText(RmlUiSystem& ui, const char* id, const std::string& value) {
  ui.SetElementText(id, value);
}

inline void BindNumber(RmlUiSystem& ui, const char* id, int value) {
  ui.SetElementText(id, std::to_string(value));
}

inline const CareerPlayerView* FindPlayer(const CareerUiViewModel& view, int playerId) {
  for (const CareerPlayerView& player : view.squad.players) {
    if (player.id == playerId) {
      return &player;
    }
  }
  return nullptr;
}

inline const CareerPlayerView* FindSelectedPlayer(const CareerUiViewModel& view) {
  if (const CareerPlayerView* selected = FindPlayer(view, view.squad.selectedPlayerId)) {
    return selected;
  }
  return view.squad.players.empty() ? nullptr : &view.squad.players.front();
}

inline std::vector<const CareerPlayerView*> StartingPlayers(const CareerUiViewModel& view) {
  std::vector<const CareerPlayerView*> result;
  result.reserve(11);

  for (int playerId : view.squad.startingPlayerIds) {
    if (const CareerPlayerView* player = FindPlayer(view, playerId)) {
      result.push_back(player);
      if (result.size() == 11) {
        return result;
      }
    }
  }

  for (const CareerPlayerView& player : view.squad.players) {
    if (!player.inStartingXI ||
        std::find(result.begin(), result.end(), &player) != result.end()) {
      continue;
    }
    result.push_back(&player);
    if (result.size() == 11) {
      return result;
    }
  }

  for (const CareerPlayerView& player : view.squad.players) {
    if (std::find(result.begin(), result.end(), &player) == result.end()) {
      result.push_back(&player);
      if (result.size() == 11) {
        break;
      }
    }
  }
  return result;
}

}  // namespace detail

inline void BindCareerUiViewModel(RmlUiSystem& ui, const CareerUiViewModel& view) {
  using namespace detail;

  // Shared shell. Missing ids are intentionally ignored, so the same binder
  // can be called after any career document is loaded.
  BindText(ui, "career-club-name", view.header.clubName);
  BindText(ui, "career-club-meta", "MANAGER CAREER · " + view.header.seasonLabel);
  BindText(ui, "career-manager-name", view.header.managerName);
  BindText(ui, "career-transfer-budget", view.header.transferBudget);
  BindNumber(ui, "career-board-confidence", view.header.boardConfidence);
  BindNumber(ui, "career-inbox-count", view.header.unreadMessages);
  BindNumber(ui, "career-squad-size", static_cast<int>(view.squad.players.size()));
  BindText(ui, "career-formation", view.squad.formation);
  BindText(ui, "career-tactical-plan", view.tactics.activeStrategy);
  if (view.season.leaguePosition > 0) {
    BindNumber(ui, "career-league-position", view.season.leaguePosition);
  }

  // Career Central summary.
  BindNumber(ui, "central-form-goals", view.season.goalsFor);
  BindNumber(ui, "central-form-conceded", view.season.goalsAgainst);
  if (view.season.leaguePosition > 0) {
    BindNumber(ui, "central-form-position", view.season.leaguePosition);
  }
  BindNumber(ui, "central-inbox-count", view.header.unreadMessages);
  BindNumber(ui, "central-board-confidence", view.header.boardConfidence);

  // Squad selected player and compact list.
  if (const CareerPlayerView* selected = FindSelectedPlayer(view)) {
    BindText(ui, "selected-player-name", selected->name);
    BindText(ui, "selected-player-meta",
             selected->position + " · AGE " + std::to_string(selected->age));
    BindNumber(ui, "selected-player-overall", selected->overall);
    BindText(ui, "selected-player-value", "VALUE " + selected->value);
    BindText(ui, "selected-player-contract",
             "CONTRACT " + std::to_string(selected->contractYears) + "Y");
  }

  for (size_t i = 0; i < view.squad.players.size() && i < 6; ++i) {
    const CareerPlayerView& player = view.squad.players[i];
    const std::string suffix = std::to_string(i);
    BindText(ui, ("squad-list-name-" + suffix).c_str(), player.name);
    BindText(ui, ("squad-list-pos-" + suffix).c_str(), player.position);
    BindNumber(ui, ("squad-list-ovr-" + suffix).c_str(), player.overall);
    BindNumber(ui, ("squad-list-fit-" + suffix).c_str(), player.fitness);
  }

  const std::vector<const CareerPlayerView*> starters = StartingPlayers(view);
  for (size_t i = 0; i < starters.size(); ++i) {
    const CareerPlayerView& player = *starters[i];
    const std::string suffix = std::to_string(i);
    BindNumber(ui, ("formation-ovr-" + suffix).c_str(), player.overall);
    BindText(ui, ("formation-name-" + suffix).c_str(), player.name);
    BindText(ui, ("formation-pos-" + suffix).c_str(), player.position);
  }

  // Season summary.
  BindNumber(ui, "season-points", view.season.points);
  BindNumber(ui, "season-goals", view.season.goalsFor);
  BindNumber(ui, "season-conceded", view.season.goalsAgainst);
  BindNumber(ui, "season-progress", view.season.progressPercent);
  if (view.season.leaguePosition > 0) {
    BindNumber(ui, "season-position", view.season.leaguePosition);
  }

  // Transfers use the career scouting shortlist as the canonical source.
  if (!view.transfers.shortlist.empty()) {
    const CareerTransferTargetView& target = view.transfers.shortlist.front();
    BindText(ui, "transfer-target-name", target.name);
    BindText(ui, "transfer-target-meta",
             target.position + " · " + std::to_string(target.age) + " · " + target.region);
    BindNumber(ui, "transfer-target-overall", target.overallHigh);
    BindText(ui, "transfer-target-value", target.askingPrice);
    BindText(ui, "transfer-target-potential",
             "POTENTIAL " + std::to_string(target.potentialHigh));
    BindText(ui, "transfer-scout-region", target.region);
  }
  BindText(ui, "transfer-active-region", view.transfers.activeRegion);
  BindNumber(ui, "transfer-scout-months", view.transfers.scoutingMonthsRemaining);

  // Office.
  BindNumber(ui, "office-confidence", view.office.boardConfidence);
  BindNumber(ui, "office-inbox-count", view.office.unreadMessages);
  BindText(ui, "office-club-balance", view.office.clubBalance);
  BindText(ui, "office-transfer-budget", view.office.transferBudget);
  BindText(ui, "office-weekly-wages", view.office.weeklyWageSpend);
  BindNumber(ui, "office-staff-count", view.office.staffCount);
  BindNumber(ui, "office-reputation", view.office.reputation);
  BindNumber(ui, "office-fans", view.office.fanBase);
  BindNumber(ui, "office-stadium-capacity", view.office.stadiumCapacity);

  // Tactics.
  BindText(ui, "tactics-plan", view.tactics.activeStrategy);
  BindText(ui, "tactics-training-focus", view.tactics.trainingFocus);
  BindNumber(ui, "tactics-chemistry", view.tactics.chemistry);
}

}  // namespace blunted::ui

#endif  // FOTBILER_CAREER_UI_BINDER_HPP
