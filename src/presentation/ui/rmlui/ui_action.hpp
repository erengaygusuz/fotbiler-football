#ifndef FOTBILER_UI_ACTION_HPP
#define FOTBILER_UI_ACTION_HPP

#include <cctype>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace blunted::ui {

struct UiAction {
  std::string name;
  std::unordered_map<std::string, std::string> arguments;

  explicit operator bool() const { return !name.empty(); }

  bool HasArgument(std::string_view key) const {
    return arguments.find(std::string(key)) != arguments.end();
  }

  std::string Argument(std::string_view key, std::string fallback = {}) const {
    const auto it = arguments.find(std::string(key));
    return it == arguments.end() ? std::move(fallback) : it->second;
  }

  std::optional<int> IntArgument(std::string_view key) const {
    const auto it = arguments.find(std::string(key));
    if (it == arguments.end() || it->second.empty()) return std::nullopt;

    int value = 0;
    const char* begin = it->second.data();
    const char* end = begin + it->second.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) return std::nullopt;
    return value;
  }

  std::optional<long long> LongLongArgument(std::string_view key) const {
    const auto it = arguments.find(std::string(key));
    if (it == arguments.end() || it->second.empty()) return std::nullopt;

    long long value = 0;
    const char* begin = it->second.data();
    const char* end = begin + it->second.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) return std::nullopt;
    return value;
  }
};

namespace detail {

inline std::string TrimActionArgument(std::string value) {
  std::size_t first = 0;
  while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }

  std::size_t last = value.size();
  while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }

  return value.substr(first, last - first);
}

inline void StoreActionArgument(UiAction& action, std::string& key, std::string& value,
                                bool sawEquals) {
  if (!sawEquals) {
    key.clear();
    value.clear();
    return;
  }

  key = TrimActionArgument(std::move(key));
  value = TrimActionArgument(std::move(value));
  if (!key.empty()) action.arguments[std::move(key)] = std::move(value);
  key.clear();
  value.clear();
}

}  // namespace detail

// data-action-args format:
//   player-id=42;value=Attacking;amount=12500000
// Backslash escapes ';', '=' and '\\' inside keys or values.
inline UiAction MakeUiAction(std::string name, std::string_view serializedArguments = {}) {
  UiAction action;
  action.name = std::move(name);

  std::string key;
  std::string value;
  bool readingValue = false;
  bool sawEquals = false;
  bool escaping = false;

  auto append = [&](char ch) {
    (readingValue ? value : key).push_back(ch);
  };

  for (char ch : serializedArguments) {
    if (escaping) {
      append(ch);
      escaping = false;
      continue;
    }

    if (ch == '\\') {
      escaping = true;
      continue;
    }

    if (ch == ';') {
      detail::StoreActionArgument(action, key, value, sawEquals);
      readingValue = false;
      sawEquals = false;
      continue;
    }

    if (ch == '=' && !readingValue) {
      readingValue = true;
      sawEquals = true;
      continue;
    }

    append(ch);
  }

  if (escaping) append('\\');
  detail::StoreActionArgument(action, key, value, sawEquals);
  return action;
}

}  // namespace blunted::ui

#endif  // FOTBILER_UI_ACTION_HPP
