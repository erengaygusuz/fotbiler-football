#ifndef FOTBILER_INPUT_SETTINGS_HPP
#define FOTBILER_INPUT_SETTINGS_HPP

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace blunted::ui {

inline constexpr std::array<std::string_view, 18> kKeyboardBindingLabels = {
    "MOVE UP",        "MOVE RIGHT",      "MOVE DOWN",       "MOVE LEFT",
    "THROUGH PASS",   "HIGH PASS",       "SHORT PASS",      "SHOOT",
    "KEEPER RUSH",    "SLIDING TACKLE",  "PRESSURE",        "SECOND MAN PRESS",
    "SWITCH PLAYER",  "SPECIAL",         "SPRINT",          "DRIBBLE / FINESSE",
    "SELECT",         "START",
};

inline std::optional<std::size_t> ParseKeyboardBindingAction(std::string_view action) {
  constexpr std::string_view prefix = "bind-key-";
  if (!action.starts_with(prefix) || action.size() == prefix.size()) return std::nullopt;

  std::size_t index = 0;
  for (char ch : action.substr(prefix.size())) {
    if (ch < '0' || ch > '9') return std::nullopt;
    index = index * 10 + static_cast<std::size_t>(ch - '0');
    if (index >= kKeyboardBindingLabels.size()) return std::nullopt;
  }
  return index;
}

}  // namespace blunted::ui

#endif  // FOTBILER_INPUT_SETTINGS_HPP
