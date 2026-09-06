#ifndef CAREER_LIFECYCLE_SERVICE_HPP
#define CAREER_LIFECYCLE_SERVICE_HPP

#include <string>

#include "data/careerdata.hpp"

namespace blunted::CareerLifecycleService {

inline CareerMode ResolveMode(const std::string& mode) {
  if (mode == "player")
    return CareerMode::PLAYER;
  if (mode == "mygm")
    return CareerMode::GM;
  if (mode == "mycoach")
    return CareerMode::COACH;
  if (mode == "owner")
    return CareerMode::OWNER;
  return CareerMode::MANAGER;
}

// Creates only the initial career domain state. Feature-specific initialization
// (board objectives, sponsor offers, transfer repository seeding) remains at
// the application/composition edge so this service does not depend on UI,
// persistence adapters, or concrete infrastructure.
inline CareerSave CreateInitialSave(const std::string& careerName, const std::string& mode,
                                    const std::string& managerName) {
  CareerSave save;
  save.name = careerName;
  save.managerName = managerName;
  save.club.clubName = careerName;
  save.mode = ResolveMode(mode);
  save.reputation = 50;
  save.club.reputation = 50;
  save.boardConfidence = 75;
  save.board.confidence = 75;
  save.transferBudget = 15000000;
  save.wageBudget = 250000;
  save.finance.transferBudget = save.transferBudget;
  save.finance.wageBudget = save.wageBudget;
  save.club.leagueName = "Default League";
  save.season.currentSeason = 1;
  save.currentSeason = 1;
  save.season.currentWeek = 1;
  save.season.inPreseason = true;
  save.season.maxWeeks = 38;
  save.season.transferWindowOpen = true;
  save.trainingPoints = (save.mode == CareerMode::COACH) ? 20 : 10;
  save.stadium.name = careerName + " Stadium";
  return save;
}

}  // namespace blunted::CareerLifecycleService

#endif  // CAREER_LIFECYCLE_SERVICE_HPP
