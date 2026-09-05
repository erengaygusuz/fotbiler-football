#ifndef FOTBILER_SCREEN_ROUTER_HPP
#define FOTBILER_SCREEN_ROUTER_HPP

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace blunted::ui {

enum class ScreenId {
  MainMenu,
  CareerModeSelect,
  MatchSetup,
  MatchLoading,
  CareerCentral,
  Squad,
  Transfers,
  Office,
  Season,
  Tactics,
  Inbox,
  Finances,
  Contracts,
  Staff,
  Scouting,
  YouthAcademy,
  PlayerDetails,
  CompetitionDetails,
  CalendarDetails,
  MatchDetails,
  RuntimeSettings,
  PauseMenu,
  MatchStats,
  TeamManagement,
  ControllerSelectModern,
  CameraSettings,
  VisualSettings,
  ReplayModern,
  Halftime,
  Fulltime,
  MatchHud,
};

struct ScreenRoute {
  ScreenId id;
  std::string_view name;
  std::string_view documentPath;
};

class ScreenRouter {
public:
  using DocumentLoader = std::function<bool(const std::string&)>;

  explicit ScreenRouter(DocumentLoader loader) : loader(std::move(loader)) {}

  bool Navigate(ScreenId target) {
    if (current && *current == target) {
      return true;
    }

    const ScreenRoute* route = FindRoute(target);
    if (!route || !loader || !loader(std::string(route->documentPath))) {
      return false;
    }

    if (current) {
      history.push_back(*current);
    }
    current = target;
    return true;
  }

  bool NavigateByName(std::string_view name) {
    const ScreenRoute* route = FindRoute(name);
    return route ? Navigate(route->id) : false;
  }

  // Replace the current navigation stack with one canonical screen. Runtime
  // round-trips use this so Back never returns to a stale loading document.
  bool Reset(ScreenId target) {
    const ScreenRoute* route = FindRoute(target);
    if (!route || !loader || !loader(std::string(route->documentPath))) {
      return false;
    }
    history.clear();
    current = target;
    return true;
  }

  bool Back() {
    if (history.empty() || !loader) {
      return false;
    }

    const ScreenId target = history.back();
    history.pop_back();
    const ScreenRoute* route = FindRoute(target);
    if (!route || !loader(std::string(route->documentPath))) {
      history.push_back(target);
      return false;
    }

    current = target;
    return true;
  }

  std::optional<ScreenId> Current() const { return current; }
  bool CanGoBack() const { return !history.empty(); }

  static const ScreenRoute* FindRoute(ScreenId id) {
    for (const ScreenRoute& route : Routes()) {
      if (route.id == id) {
        return &route;
      }
    }
    return nullptr;
  }

  static const ScreenRoute* FindRoute(std::string_view name) {
    for (const ScreenRoute& route : Routes()) {
      if (route.name == name) {
        return &route;
      }
    }
    return nullptr;
  }

private:
  static const std::array<ScreenRoute, 31>& Routes() {
    static constexpr std::array<ScreenRoute, 31> routes = {{
        {ScreenId::MainMenu, "main-menu", "media/ui/fotbiler/main_menu.rml"},
        {ScreenId::CareerModeSelect, "career-mode-select", "media/ui/fotbiler/mode_select.rml"},
        {ScreenId::MatchSetup, "match-setup", "media/ui/fotbiler/match_setup.rml"},
        {ScreenId::MatchLoading, "match-loading", "media/ui/fotbiler/loading_match.rml"},
        {ScreenId::CareerCentral, "career-central", "media/ui/fotbiler/career_central.rml"},
        {ScreenId::Squad, "squad", "media/ui/fotbiler/squad.rml"},
        {ScreenId::Transfers, "transfers", "media/ui/fotbiler/transfers.rml"},
        {ScreenId::Office, "office", "media/ui/fotbiler/office.rml"},
        {ScreenId::Season, "season", "media/ui/fotbiler/season.rml"},
        {ScreenId::Tactics, "tactics", "media/ui/fotbiler/tactics.rml"},
        {ScreenId::Inbox, "inbox", "media/ui/fotbiler/inbox.rml"},
        {ScreenId::Finances, "finances", "media/ui/fotbiler/finances.rml"},
        {ScreenId::Contracts, "contracts", "media/ui/fotbiler/contracts.rml"},
        {ScreenId::Staff, "staff", "media/ui/fotbiler/staff.rml"},
        {ScreenId::Scouting, "scouting", "media/ui/fotbiler/scouting.rml"},
        {ScreenId::YouthAcademy, "youth-academy", "media/ui/fotbiler/youth_academy.rml"},
        {ScreenId::PlayerDetails, "player-details", "media/ui/fotbiler/player_details.rml"},
        {ScreenId::CompetitionDetails, "competition-details", "media/ui/fotbiler/competition_details.rml"},
        {ScreenId::CalendarDetails, "calendar-details", "media/ui/fotbiler/calendar_details.rml"},
        {ScreenId::MatchDetails, "match-details", "media/ui/fotbiler/match_details.rml"},
        {ScreenId::RuntimeSettings, "runtime-settings", "media/ui/fotbiler/runtime_settings.rml"},
        {ScreenId::PauseMenu, "pause-menu", "media/ui/fotbiler/pause_menu.rml"},
        {ScreenId::MatchStats, "match-stats", "media/ui/fotbiler/match_stats.rml"},
        {ScreenId::TeamManagement, "team-management", "media/ui/fotbiler/team_management.rml"},
        {ScreenId::ControllerSelectModern, "controller-select-modern", "media/ui/fotbiler/controller_select_modern.rml"},
        {ScreenId::CameraSettings, "camera-settings", "media/ui/fotbiler/camera_settings.rml"},
        {ScreenId::VisualSettings, "visual-settings", "media/ui/fotbiler/visual_settings.rml"},
        {ScreenId::ReplayModern, "replay-modern", "media/ui/fotbiler/replay_modern.rml"},
        {ScreenId::Halftime, "halftime", "media/ui/fotbiler/halftime.rml"},
        {ScreenId::Fulltime, "fulltime", "media/ui/fotbiler/fulltime.rml"},
        {ScreenId::MatchHud, "match-hud", "media/ui/fotbiler/match_hud.rml"},
    }};
    return routes;
  }

  DocumentLoader loader;
  std::optional<ScreenId> current;
  std::vector<ScreenId> history;
};

}  // namespace blunted::ui

#endif  // FOTBILER_SCREEN_ROUTER_HPP
