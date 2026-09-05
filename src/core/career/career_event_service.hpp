#ifndef CAREER_EVENT_SERVICE_HPP
#define CAREER_EVENT_SERVICE_HPP

#include <algorithm>
#include <string>
#include <vector>

#include "../../data/careerdata.hpp"

namespace blunted {
namespace CareerEventService {

// Applies a career event to domain state. Returns true when the event is a
// major milestone and the application layer should persist the updated save.
inline bool AddEvent(::CareerSave& save, const std::string& eventType,
                     const std::string& description, int reputationDelta, bool isMajor) {
  save.reputation = std::clamp(save.reputation + reputationDelta, -100, 100);
  save.club.reputation = save.reputation;
  save.recentEvents.emplace_back(eventType, eventType + ": " + description, reputationDelta, 0,
                                 isMajor);

  if (save.recentEvents.size() > 50) {
    save.recentEvents.erase(save.recentEvents.begin());
  }

  if (isMajor) {
    save.legacyStats[eventType]++;
  }

  return isMajor;
}

inline void ModifyBoardConfidence(::CareerSave& save, int delta) {
  save.boardConfidence = std::clamp(save.boardConfidence + delta, 0, 100);
  save.board.confidence = save.boardConfidence;
}

inline int GetReputation(const ::CareerSave* save) {
  return save ? save->reputation : 0;
}

inline int GetLegacyStat(const ::CareerSave* save, const std::string& statName) {
  if (!save) {
    return 0;
  }

  const auto it = save->legacyStats.find(statName);
  return it != save->legacyStats.end() ? it->second : 0;
}

inline std::vector<::CareerEvent> GetRecentEvents(const ::CareerSave* save, int limit) {
  if (!save || limit <= 0) {
    return {};
  }

  std::vector<::CareerEvent> result;
  for (auto it = save->recentEvents.rbegin();
       it != save->recentEvents.rend() && static_cast<int>(result.size()) < limit; ++it) {
    result.push_back(*it);
  }
  return result;
}

}  // namespace CareerEventService
}  // namespace blunted

#endif  // CAREER_EVENT_SERVICE_HPP
