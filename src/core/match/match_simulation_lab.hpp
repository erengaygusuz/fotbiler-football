#ifndef FOTBILER_MATCH_SIMULATION_LAB_HPP
#define FOTBILER_MATCH_SIMULATION_LAB_HPP

#include <map>
#include <string>

#include "match_engine.hpp"

namespace blunted {

// Aggregated telemetry for repeated runs of one match-engine scenario. The lab
// deliberately depends only on IMatchEngine so fast/headless simulation and
// interactive engines can be inspected through the same boundary.
struct MatchSimulationTelemetry {
  int requestedRuns = 0;
  int completedRuns = 0;
  int interactiveRuns = 0;
  int completedInteractiveRuns = 0;
  int shotStatRuns = 0;
  int possessionStatRuns = 0;
  int scorerStatRuns = 0;
  int wins = 0;
  int draws = 0;
  int losses = 0;
  long long userGoals = 0;
  long long opponentGoals = 0;
  long long userGoalsWithShotStats = 0;
  long long opponentGoalsWithShotStats = 0;
  long long userShots = 0;
  long long opponentShots = 0;
  long long userPossession = 0;
  std::map<std::string, long long> userScorerGoals;

  void RecordCompletedResult(const MatchResult& result,
                             bool interactiveCompletion = false) {
    if (!result.completed)
      return;

    ++completedRuns;
    if (interactiveCompletion)
      ++completedInteractiveRuns;

    userGoals += result.userGoals;
    opponentGoals += result.opponentGoals;

    if (result.shotsAvailable) {
      ++shotStatRuns;
      userShots += result.userShots;
      opponentShots += result.opponentShots;
      userGoalsWithShotStats += result.userGoals;
      opponentGoalsWithShotStats += result.opponentGoals;
    }

    if (result.possessionAvailable) {
      ++possessionStatRuns;
      userPossession += result.userPossession;
    }

    if (result.scorersAvailable) {
      ++scorerStatRuns;
      for (const auto& scorer : result.userScorers)
        ++userScorerGoals[scorer];
    }

    if (result.userGoals > result.opponentGoals)
      ++wins;
    else if (result.userGoals == result.opponentGoals)
      ++draws;
    else
      ++losses;
  }

  void Record(const MatchEngineRun& run) {
    if (run.requiresInteractivePlay) {
      ++interactiveRuns;
      return;
    }
    RecordCompletedResult(run.result);
  }

  double CompletionRate() const {
    return requestedRuns > 0 ? static_cast<double>(completedRuns) / requestedRuns : 0.0;
  }

  double WinRate() const {
    return completedRuns > 0 ? static_cast<double>(wins) / completedRuns : 0.0;
  }

  double DrawRate() const {
    return completedRuns > 0 ? static_cast<double>(draws) / completedRuns : 0.0;
  }

  double LossRate() const {
    return completedRuns > 0 ? static_cast<double>(losses) / completedRuns : 0.0;
  }

  double AverageUserGoals() const {
    return completedRuns > 0 ? static_cast<double>(userGoals) / completedRuns : 0.0;
  }

  double AverageOpponentGoals() const {
    return completedRuns > 0 ? static_cast<double>(opponentGoals) / completedRuns : 0.0;
  }

  double AverageTotalGoals() const {
    return completedRuns > 0
               ? static_cast<double>(userGoals + opponentGoals) / completedRuns
               : 0.0;
  }

  double AverageGoalDifference() const {
    return completedRuns > 0
               ? static_cast<double>(userGoals - opponentGoals) / completedRuns
               : 0.0;
  }

  bool HasShotStats() const { return shotStatRuns > 0; }
  bool HasPossessionStats() const { return possessionStatRuns > 0; }
  bool HasScorerStats() const { return scorerStatRuns > 0; }

  double AverageUserShots() const {
    return shotStatRuns > 0 ? static_cast<double>(userShots) / shotStatRuns : 0.0;
  }

  double AverageOpponentShots() const {
    return shotStatRuns > 0 ? static_cast<double>(opponentShots) / shotStatRuns : 0.0;
  }

  double UserShotConversion() const {
    return userShots > 0 ? static_cast<double>(userGoalsWithShotStats) / userShots : 0.0;
  }

  double OpponentShotConversion() const {
    return opponentShots > 0
               ? static_cast<double>(opponentGoalsWithShotStats) / opponentShots
               : 0.0;
  }

  double AverageUserPossession() const {
    return possessionStatRuns > 0
               ? static_cast<double>(userPossession) / possessionStatRuns
               : 0.0;
  }
};

// Distribution comparison intentionally works only on metrics that both sides
// actually expose. This allows Full3D score samples to be compared with fast
// simulation today without treating placeholder shots/possession as real data.
struct MatchSimulationComparison {
  bool scoreComparable = false;
  bool shotsComparable = false;
  bool possessionComparable = false;
  bool scorersComparable = false;
  double userGoalsDelta = 0.0;
  double opponentGoalsDelta = 0.0;
  double goalDifferenceDelta = 0.0;
  double winRateDelta = 0.0;
  double userShotsDelta = 0.0;
  double possessionDelta = 0.0;
};

inline MatchSimulationComparison CompareMatchSimulationTelemetry(
    const MatchSimulationTelemetry& reference,
    const MatchSimulationTelemetry& candidate) {
  MatchSimulationComparison comparison;
  comparison.scoreComparable = reference.completedRuns > 0 && candidate.completedRuns > 0;
  comparison.shotsComparable = reference.HasShotStats() && candidate.HasShotStats();
  comparison.possessionComparable =
      reference.HasPossessionStats() && candidate.HasPossessionStats();
  comparison.scorersComparable = reference.HasScorerStats() && candidate.HasScorerStats();

  if (comparison.scoreComparable) {
    comparison.userGoalsDelta = candidate.AverageUserGoals() - reference.AverageUserGoals();
    comparison.opponentGoalsDelta =
        candidate.AverageOpponentGoals() - reference.AverageOpponentGoals();
    comparison.goalDifferenceDelta =
        candidate.AverageGoalDifference() - reference.AverageGoalDifference();
    comparison.winRateDelta = candidate.WinRate() - reference.WinRate();
  }
  if (comparison.shotsComparable)
    comparison.userShotsDelta = candidate.AverageUserShots() - reference.AverageUserShots();
  if (comparison.possessionComparable) {
    comparison.possessionDelta =
        candidate.AverageUserPossession() - reference.AverageUserPossession();
  }
  return comparison;
}

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
