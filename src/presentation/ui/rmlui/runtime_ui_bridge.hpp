#ifndef FOTBILER_RUNTIME_UI_BRIDGE_HPP
#define FOTBILER_RUNTIME_UI_BRIDGE_HPP

#include <atomic>
#include <mutex>
#include <string>

namespace blunted::ui::runtime {

enum class Screen : int {
  None = 0,
  Loading,
  Pause,
  MatchStats,
  TeamManagement,
  Replay,
  Settings,
  CameraSettings,
  ControllerSelect,
  VisualSettings,
};

enum class Command : int {
  None = 0,
  ResumeMatch,
  ExitMatch,
};

struct MatchSnapshot {
  std::string homeName;
  std::string awayName;
  std::string homeShortName;
  std::string awayShortName;
  int homeScore = 0;
  int awayScore = 0;
  int minute = 0;
  int homePossession = 50;
  int awayPossession = 50;
  int homeShots = 0;
  int awayShots = 0;
  int homeShotsOnTarget = 0;
  int awayShotsOnTarget = 0;
  int homePassAccuracy = 0;
  int awayPassAccuracy = 0;
  int homeFouls = 0;
  int awayFouls = 0;
};

inline std::atomic<int> g_screen{static_cast<int>(Screen::None)};
inline std::atomic<int> g_command{static_cast<int>(Command::None)};
inline std::mutex g_snapshotMutex;
inline MatchSnapshot g_snapshot;

inline void SetScreen(Screen screen) {
  g_screen.store(static_cast<int>(screen), std::memory_order_release);
}

inline Screen GetScreen() {
  return static_cast<Screen>(g_screen.load(std::memory_order_acquire));
}

inline void SendCommand(Command command) {
  g_command.store(static_cast<int>(command), std::memory_order_release);
}

inline Command ConsumeCommand() {
  return static_cast<Command>(
      g_command.exchange(static_cast<int>(Command::None), std::memory_order_acq_rel));
}

inline void PublishMatchSnapshot(const MatchSnapshot& snapshot) {
  std::lock_guard<std::mutex> lock(g_snapshotMutex);
  g_snapshot = snapshot;
}

inline MatchSnapshot ReadMatchSnapshot() {
  std::lock_guard<std::mutex> lock(g_snapshotMutex);
  return g_snapshot;
}

inline void Reset() {
  SetScreen(Screen::None);
  g_command.store(static_cast<int>(Command::None), std::memory_order_release);
  std::lock_guard<std::mutex> lock(g_snapshotMutex);
  g_snapshot = MatchSnapshot{};
}

}  // namespace blunted::ui::runtime

#endif  // FOTBILER_RUNTIME_UI_BRIDGE_HPP
