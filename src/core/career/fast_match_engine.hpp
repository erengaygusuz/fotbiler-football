#ifndef CAREER_FAST_MATCH_ENGINE_HPP
#define CAREER_FAST_MATCH_ENGINE_HPP

#include "career_sim.hpp"
#include "core/match/match_engine.hpp"

namespace blunted {

class FastMatchEngine final : public IMatchEngine {
public:
  explicit FastMatchEngine(CareerSave& save) : save(save) {}

  MatchEngineKind GetKind() const override { return MatchEngineKind::FastSimulation; }

  MatchEngineRun Start(const MatchRequest& request) override {
    const SimulatedMatch simulated = CareerSim::SimulateMatchResult(
        save, request.opponentName, request.opponentTeamDBID, request.userIsHome);

    MatchEngineRun run;
    run.requiresInteractivePlay = false;
    run.result.userGoals = simulated.homeGoals;
    run.result.opponentGoals = simulated.awayGoals;
    run.result.userShots = simulated.homeShots;
    run.result.opponentShots = simulated.awayShots;
    run.result.userPossession = simulated.homePossession;
    run.result.userScorers = simulated.scorers;
    run.result.completed = simulated.played;
    return run;
  }

  MatchResult Complete(const MatchRequest& request, int engineHomeGoals,
                       int engineAwayGoals) const override {
    return NormalizeHomeAwayResult(request, engineHomeGoals, engineAwayGoals);
  }

private:
  CareerSave& save;
};

inline SimulatedMatch ToLegacySimulatedMatch(const MatchRequest& request,
                                             const MatchResult& result) {
  SimulatedMatch legacy;
  legacy.opponentName = request.opponentName;
  legacy.homeGoals = result.userGoals;
  legacy.awayGoals = result.opponentGoals;
  legacy.homeShots = result.userShots;
  legacy.awayShots = result.opponentShots;
  legacy.homePossession = result.userPossession;
  legacy.scorers = result.userScorers;
  legacy.played = result.completed;
  return legacy;
}

}  // namespace blunted

#endif  // CAREER_FAST_MATCH_ENGINE_HPP
