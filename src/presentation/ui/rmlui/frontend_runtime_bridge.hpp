#ifndef FOTBILER_FRONTEND_RUNTIME_BRIDGE_HPP
#define FOTBILER_FRONTEND_RUNTIME_BRIDGE_HPP

#include <atomic>
#include <mutex>

namespace blunted::ui::frontend {

enum class AppMode : int {
  Frontend = 0,
  Loading,
  Match,
};

enum class SessionKind : int {
  None = 0,
  QuickMatch,
  Career,
};

enum class ReturnTarget : int {
  MainMenu = 0,
  MatchSetup,
  CareerCentral,
};

enum class LaunchKind : int {
  None = 0,
  QuickMatch,
  Career,
};

struct LaunchRequest {
  LaunchKind kind = LaunchKind::None;
  int homeTeamId = 3;
  int awayTeamId = 8;
  int matchDurationMinutes = 10;
  float difficulty = 0.75f;
  int controlSide = -1;
};

struct DisplaySettingsRequest {
  int width = 1920;
  int height = 1080;
  bool fullscreen = true;
  bool vsync = true;
  int difficultyStep = 3;
  int gameSpeedStep = 1;
  int volume = 80;
};

inline std::atomic<int> g_appMode{static_cast<int>(AppMode::Frontend)};
inline std::atomic<int> g_sessionKind{static_cast<int>(SessionKind::None)};
inline std::atomic<int> g_returnTarget{static_cast<int>(ReturnTarget::MainMenu)};
inline std::atomic<bool> g_quitRequested{false};
inline std::atomic<bool> g_launchPending{false};
inline std::atomic<bool> g_displaySettingsPending{false};
inline std::mutex g_launchMutex;
inline std::mutex g_displaySettingsMutex;
inline LaunchRequest g_launchRequest;
inline DisplaySettingsRequest g_displaySettingsRequest;

inline void SetAppMode(AppMode mode) {
  g_appMode.store(static_cast<int>(mode), std::memory_order_release);
}

inline AppMode GetAppMode() {
  return static_cast<AppMode>(g_appMode.load(std::memory_order_acquire));
}

inline void SetSessionKind(SessionKind kind) {
  g_sessionKind.store(static_cast<int>(kind), std::memory_order_release);
}

inline SessionKind GetSessionKind() {
  return static_cast<SessionKind>(g_sessionKind.load(std::memory_order_acquire));
}

inline void SetReturnTarget(ReturnTarget target) {
  g_returnTarget.store(static_cast<int>(target), std::memory_order_release);
}

inline ReturnTarget GetReturnTarget() {
  return static_cast<ReturnTarget>(g_returnTarget.load(std::memory_order_acquire));
}

inline void PublishQuickMatchLaunch(int homeTeamId, int awayTeamId, int durationMinutes,
                                    float difficulty, int controlSide) {
  std::lock_guard<std::mutex> lock(g_launchMutex);
  g_launchRequest.kind = LaunchKind::QuickMatch;
  g_launchRequest.homeTeamId = homeTeamId;
  g_launchRequest.awayTeamId = awayTeamId;
  g_launchRequest.matchDurationMinutes = durationMinutes;
  g_launchRequest.difficulty = difficulty;
  g_launchRequest.controlSide = controlSide;
  SetSessionKind(SessionKind::QuickMatch);
  SetReturnTarget(ReturnTarget::MatchSetup);
  SetAppMode(AppMode::Loading);
  g_launchPending.store(true, std::memory_order_release);
}

inline void PublishCareerLaunch() {
  std::lock_guard<std::mutex> lock(g_launchMutex);
  g_launchRequest = LaunchRequest{};
  g_launchRequest.kind = LaunchKind::Career;
  SetSessionKind(SessionKind::Career);
  SetReturnTarget(ReturnTarget::CareerCentral);
  SetAppMode(AppMode::Loading);
  g_launchPending.store(true, std::memory_order_release);
}

inline bool ConsumeLaunchRequest(LaunchRequest& outRequest) {
  if (!g_launchPending.exchange(false, std::memory_order_acq_rel)) {
    return false;
  }
  std::lock_guard<std::mutex> lock(g_launchMutex);
  outRequest = g_launchRequest;
  g_launchRequest = LaunchRequest{};
  return outRequest.kind != LaunchKind::None;
}

inline void PublishDisplaySettings(const DisplaySettingsRequest& request) {
  std::lock_guard<std::mutex> lock(g_displaySettingsMutex);
  g_displaySettingsRequest = request;
  g_displaySettingsPending.store(true, std::memory_order_release);
}

inline bool ConsumeDisplaySettings(DisplaySettingsRequest& outRequest) {
  if (!g_displaySettingsPending.exchange(false, std::memory_order_acq_rel)) {
    return false;
  }
  std::lock_guard<std::mutex> lock(g_displaySettingsMutex);
  outRequest = g_displaySettingsRequest;
  return true;
}

inline void RequestQuit() {
  g_quitRequested.store(true, std::memory_order_release);
}

inline bool ConsumeQuitRequest() {
  return g_quitRequested.exchange(false, std::memory_order_acq_rel);
}

inline void BeginMatch() {
  SetAppMode(AppMode::Match);
}

inline void ReturnToFrontend(ReturnTarget target) {
  SetReturnTarget(target);
  SetSessionKind(SessionKind::None);
  SetAppMode(AppMode::Frontend);
}

inline void Reset() {
  SetAppMode(AppMode::Frontend);
  SetSessionKind(SessionKind::None);
  SetReturnTarget(ReturnTarget::MainMenu);
  g_quitRequested.store(false, std::memory_order_release);
  g_launchPending.store(false, std::memory_order_release);
  g_displaySettingsPending.store(false, std::memory_order_release);
  {
    std::lock_guard<std::mutex> lock(g_launchMutex);
    g_launchRequest = LaunchRequest{};
  }
  {
    std::lock_guard<std::mutex> lock(g_displaySettingsMutex);
    g_displaySettingsRequest = DisplaySettingsRequest{};
  }
}

}  // namespace blunted::ui::frontend

#endif  // FOTBILER_FRONTEND_RUNTIME_BRIDGE_HPP
