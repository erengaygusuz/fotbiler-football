#ifndef FOTBILER_INPUT_SETTINGS_HPP
#define FOTBILER_INPUT_SETTINGS_HPP

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace blunted::ui {

inline constexpr std::array<std::string_view, 18> kKeyboardBindingLabels = {
    "MOVE UP",        "MOVE RIGHT",      "MOVE DOWN",       "MOVE LEFT",
    "THROUGH PASS",   "HIGH PASS",       "SHORT PASS",      "SHOOT",
    "KEEPER RUSH",    "SLIDING TACKLE",  "PRESSURE",        "SECOND MAN PRESS",
    "SWITCH PLAYER",  "SPECIAL",         "SPRINT",          "DRIBBLE / FINESSE",
    "SELECT",         "START",
};

inline constexpr std::array<std::string_view, 14> kControllerButtonLabels = {
    "DPAD UP", "DPAD RIGHT", "DPAD DOWN", "DPAD LEFT", "Y / TRIANGLE", "B / CIRCLE",
    "A / CROSS", "X / SQUARE", "L1 / LB", "L2 / LT", "R1 / RB", "R2 / RT", "SELECT / BACK",
    "START / OPTIONS",
};

inline std::optional<std::size_t> ParseIndexedAction(std::string_view action,
                                                     std::string_view prefix,
                                                     std::size_t limit) {
  if (action.size() <= prefix.size() || action.substr(0, prefix.size()) != prefix) {
    return std::nullopt;
  }

  std::size_t index = 0;
  for (char ch : action.substr(prefix.size())) {
    if (ch < '0' || ch > '9') return std::nullopt;
    index = index * 10 + static_cast<std::size_t>(ch - '0');
    if (index >= limit) return std::nullopt;
  }
  return index;
}

inline std::optional<std::size_t> ParseKeyboardBindingAction(std::string_view action) {
  return ParseIndexedAction(action, "bind-key-", kKeyboardBindingLabels.size());
}

inline std::optional<std::size_t> ParseGamepadSelectAction(std::string_view action) {
  return ParseIndexedAction(action, "select-gamepad-", 8);
}

inline std::optional<std::size_t> ParseGamepadPhysicalBindingAction(std::string_view action) {
  return ParseIndexedAction(action, "bind-gamepad-physical-", kControllerButtonLabels.size());
}

inline std::optional<std::size_t> ParseGamepadFunctionAction(std::string_view action) {
  return ParseIndexedAction(action, "cycle-gamepad-function-", kKeyboardBindingLabels.size());
}

// Legacy HIDGamepad stores physical axis directions as negative integers.
// axis 0 negative => -1, axis 0 positive => -2, axis 1 negative => -3, etc.
inline int EncodeGamepadAxisDirection(int axis, bool positive) {
  if (axis < 0) return 0;
  const int encoded = axis * 2 + (positive ? 1 : 0);
  return -(encoded + 1);
}

inline std::string DescribePhysicalGamepadBinding(int binding) {
  if (binding >= 0) return "BUTTON " + std::to_string(binding);
  const int encoded = -binding - 1;
  const int axis = encoded / 2;
  const bool positive = (encoded % 2) != 0;
  return "AXIS " + std::to_string(axis) + (positive ? "+" : "-");
}

}  // namespace blunted::ui

#endif  // FOTBILER_INPUT_SETTINGS_HPP
