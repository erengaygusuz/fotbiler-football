#ifndef FOTBILER_MATCH_SIMULATION_LAB_HPP
#define FOTBILER_MATCH_SIMULATION_LAB_HPP

#include "match_engine.hpp"

namespace blunted {

// Aggregated telemetry for repeated runs of one match-engine scenario. The lab
// deliberately depends only on IMatchEngine so fast/headless simulation and
// interactive engines can be inspected through the same boundary.
struct MatchSimulationTelemetry {
  int requestedRuns = 0;
  int completedRuns = 0;
  int interactiveRuns = 0;
  int wins = 0;
  int draws = 0;
  int losses = 0;
  long long userGoals = 0;
  long long opponentGoals = 0;
  long long userShots = 0;
  long long opponentShots = 0;
  long long userPossession = 0;

  void Record(const MatchEngineRun& run) {
    if (run.requiresInteractivePlay) {
      ++interactiveRuns;
      return;
    }

    if (!run.result.completed)
      return;

    ++completedRuns;
    userGoals += run.result.userGoals;
    opponentGoals += run.result.opponentGoals;
    userShots += run.result.userShots;
    opponentShots += run.result.opponentShots;
    userPossession += run.result.userPossession;

    if (run.result.userGoals > run.result.opponentGoals)
      ++wins;
    else if (run.result.userGoals == run.result.opponentGoals)
      ++draws;
    else
      ++losses;
  }

  double CompletionRate() const {
    return requestedRuns > 0 ? static_cast<double>(completedRuns) / requestedRuns : 0.0;
  }

  double WinRate() const {
    return completedRuns > 0 ? static_cast<double>(wins) / completedRuns : 0.0;
  }

  double AverageUserGoals() const {
    return completedRuns > 0 ? static_cast<double>(userGoals) / completedRuns : 0.0;
  }

  double AverageOpponentGoals() const {
    return completedRuns > 0 ? static_cast<double>(opponentGoals) / completedRuns : 0.0;
  }

  double AverageGoalDifference() const {
    return completedRuns > 0
               ? static_cast<double>(userGoals - opponentGoals) / completedRuns
               : 0.0;
  }

  double AverageUserShots() const {
    return completedRuns > 0 ? static_cast<double>(userShots) / completedRuns : 0.0;
  }

  double AverageOpponentShots() const {
    return completedRuns > 0 ? static_cast<double>(opponentShots) / completedRuns : 0.0;
  }

  double AverageUserPossession() const {
    return completedRuns > 0 ? static_cast<double>(userPossession) / completedRuns : 0.0;
  }
};

struct MatchSimulationScenario {
  const char* name = "unnamed";
  MatchRequest request;
  int runs = 1;
};

struct MatchSimulationScenarioReport {
  const char* name = "unnamed";
  MatchSimulationTelemetry telemetry;
};

inline MatchSimulationTelemetry RunMatchSimulationBatch(IMatchEngine& engine,
                                                        const MatchRequest& request,
                                                        int runs) {
  MatchSimulationTelemetry telemetry;
  telemetry.requestedRuns = runs > 0 ? runs : 0;

  for (int i = 0; i < telemetry.requestedRuns; ++i)
    telemetry.Record(engine.Start(request));

  return telemetry;
}

inline MatchSimulationScenarioReport RunMatchSimulationScenario(
    IMatchEngine& engine, const MatchSimulationScenario& scenario) {
  MatchSimulationScenarioReport report;
  report.name = scenario.name;
  report.telemetry = RunMatchSimulationBatch(engine, scenario.request, scenario.runs);
  return report;
}

}  // namespace blunted

#endif  // FOTBILER_MATCH_SIMULATION_LAB_HPP
