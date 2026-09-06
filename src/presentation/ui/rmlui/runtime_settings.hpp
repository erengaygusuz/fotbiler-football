#ifndef FOTBILER_RUNTIME_SETTINGS_HPP
#define FOTBILER_RUNTIME_SETTINGS_HPP

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace blunted::ui {

struct RuntimeSettings {
  std::vector<std::pair<int, int>> resolutions{
      {1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}};
  std::size_t resolutionIndex = 2;
  bool fullscreen = true;
  bool vsync = true;
  int difficultyStep = 3;
  int gameSpeedStep = 1;
  int volume = 80;

  int Width() const { return resolutions.at(resolutionIndex).first; }
  int Height() const { return resolutions.at(resolutionIndex).second; }
};

inline std::filesystem::path RuntimeSettingsPath() {
  return std::filesystem::path("user") / "fotbiler_ui.settings";
}

inline void SaveRuntimeSettings(const RuntimeSettings& state) {
  const std::filesystem::path path = RuntimeSettingsPath();
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  std::ofstream file(path);
  if (!file) return;

  file << "fullscreen=" << (state.fullscreen ? 1 : 0) << '\n';
  file << "width=" << state.Width() << '\n';
  file << "height=" << state.Height() << '\n';
  file << "vsync=" << (state.vsync ? 1 : 0) << '\n';
  file << "difficulty=" << state.difficultyStep << '\n';
  file << "game_speed=" << state.gameSpeedStep << '\n';
  file << "volume=" << state.volume << '\n';
}

inline RuntimeSettings LoadRuntimeSettings() {
  RuntimeSettings state;
  std::ifstream file(RuntimeSettingsPath());
  if (!file) return state;

  int savedWidth = state.Width();
  int savedHeight = state.Height();
  std::string line;
  while (std::getline(file, line)) {
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) continue;

    const std::string key = line.substr(0, eq);
    const int number = std::atoi(line.substr(eq + 1).c_str());
    if (key == "fullscreen") state.fullscreen = number != 0;
    else if (key == "width") savedWidth = number;
    else if (key == "height") savedHeight = number;
    else if (key == "vsync") state.vsync = number != 0;
    else if (key == "difficulty") state.difficultyStep = std::clamp(number, 0, 4);
    else if (key == "game_speed") state.gameSpeedStep = std::clamp(number, 0, 2);
    else if (key == "volume") state.volume = std::clamp(number, 0, 100);
  }

  for (std::size_t i = 0; i < state.resolutions.size(); ++i) {
    if (state.resolutions[i].first == savedWidth && state.resolutions[i].second == savedHeight) {
      state.resolutionIndex = i;
      break;
    }
  }
  return state;
}

inline const char* RuntimeDifficultyName(int step) {
  switch (std::clamp(step, 0, 4)) {
    case 0: return "BEGINNER";
    case 1: return "AMATEUR";
    case 2: return "REGULAR";
    case 3: return "PROFESSIONAL";
    default: return "TOP PLAYER";
  }
}

inline const char* RuntimeGameSpeedName(int step) {
  switch (std::clamp(step, 0, 2)) {
    case 0: return "SLOW";
    case 1: return "NORMAL";
    default: return "FAST";
  }
}

}  // namespace blunted::ui

#endif  // FOTBILER_RUNTIME_SETTINGS_HPP
