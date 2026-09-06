#ifndef CAREER_SIMULATION_SCENARIOS_HPP
#define CAREER_SIMULATION_SCENARIOS_HPP

#include <algorithm>
#include <string>
#include <vector>

#include "career_common.hpp"
#include "fast_match_engine.hpp"
#include "core/match/match_simulation_lab.hpp"

namespace blunted {

struct CareerSimulationScenario {
  std::string name;
  int rosterOverall = 70;
  int rosterSize = 11;
  int morale = 70;
  int matchForm = 50;
  int fitness = 100;
  std::string strategy = "Balanced";
  int opponentRating = 70;
  bool userIsHome = true;
  unsigned int seed = 1;
  int runs = 400;
};

struct CareerSimulationScenarioReport {
  CareerSimulationScenario scenario;
  MatchSimulationTelemetry telemetry;
};

inline int ClampScenarioOpponentRating(int rating) {
  return std::max(45, std::min(88, rating));
}

// CareerSim's numeric opponent path maps id N to 45 + (N mod 44).
// Using ids 0..43 therefore gives us explicit, stable 45..88 opponent ratings
// without depending on std::hash<string> for named opponents.
inline std::string StableOpponentTeamIdForRating(int rating) {
  return std::to_string(ClampScenarioOpponentRating(rating) - 45);
}

inline CareerSave BuildCareerScenarioSave(const CareerSimulationScenario& scenario) {
  CareerSave save;
  save.name = "Simulation Lab FC";
  save.activeStrategy = scenario.strategy;

  static const char* positions[] = {"GK", "CB", "CB", "LB", "RB", "DM",
                                    "CM", "CM", "AM", "CF", "ST"};

  const int rosterSize = std::max(0, scenario.rosterSize);
  for (int i = 0; i < rosterSize; ++i) {
    PlayerCareerState player;
    player.name = "Scenario Player " + std::to_string(i);
    player.position = positions[i % 11];
    player.preferredPosition = player.position;
    player.ovr = scenario.rosterOverall;
    player.pot = scenario.rosterOverall;
    player.form = scenario.matchForm;
    player.matchForm = scenario.matchForm;
    player.morale = scenario.morale;
    player.fitness = scenario.fitness;
    save.roster.push_back(player);
  }

  return save;
}

inline MatchRequest BuildCareerScenarioRequest(const CareerSimulationScenario& scenario) {
  MatchRequest request;
  // Intentionally leave opponentName empty so CareerSim uses the stable numeric
  // team-id path rather than implementation-defined std::hash<string>.
  request.opponentName.clear();
  request.opponentTeamDBID = StableOpponentTeamIdForRating(scenario.opponentRating);
  request.userIsHome = scenario.userIsHome;
  return request;
}

inline CareerSimulationScenarioReport RunCareerSimulationScenario(
    const CareerSimulationScenario& scenario) {
  CareerSave save = BuildCareerScenarioSave(scenario);
  FastMatchEngine engine(save);
  CareerCommon::SeedRng(scenario.seed);

  CareerSimulationScenarioReport report;
  report.scenario = scenario;
  report.telemetry =
      RunMatchSimulationBatch(engine, BuildCareerScenarioRequest(scenario), scenario.runs);
  return report;
}

inline CareerSimulationScenario MakeCareerSimulationScenario(
    const std::string& name, int rosterOverall, int opponentRating, bool userIsHome,
    const std::string& strategy, int morale, int matchForm, int fitness, int rosterSize,
    unsigned int seed, int runs) {
  CareerSimulationScenario scenario;
  scenario.name = name;
  scenario.rosterOverall = rosterOverall;
  scenario.opponentRating = opponentRating;
  scenario.userIsHome = userIsHome;
  scenario.strategy = strategy;
  scenario.morale = morale;
  scenario.matchForm = matchForm;
  scenario.fitness = fitness;
  scenario.rosterSize = rosterSize;
  scenario.seed = seed;
  scenario.runs = std::max(1, runs);
  return scenario;
}

inline std::vector<CareerSimulationScenario> BuildDefaultCareerSimulationScenarioCatalogue(
    int runs = 400) {
  const int batchRuns = std::max(1, runs);
  std::vector<CareerSimulationScenario> scenarios;

  const unsigned int teamStrengthSeed = 15101u;
  scenarios.push_back(MakeCareerSimulationScenario(
      "team_strength_weak", 55, 70, true, "Balanced", 70, 50, 100, 11,
      teamStrengthSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "team_strength_equal", 70, 70, true, "Balanced", 70, 50, 100, 11,
      teamStrengthSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "team_strength_strong", 85, 70, true, "Balanced", 70, 50, 100, 11,
      teamStrengthSeed, batchRuns));

  const unsigned int oppositionSeed = 15102u;
  scenarios.push_back(MakeCareerSimulationScenario(
      "opposition_weak", 70, 55, true, "Balanced", 70, 50, 100, 11,
      oppositionSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "opposition_equal", 70, 70, true, "Balanced", 70, 50, 100, 11,
      oppositionSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "opposition_strong", 70, 85, true, "Balanced", 70, 50, 100, 11,
      oppositionSeed, batchRuns));

  const unsigned int venueSeed = 15103u;
  scenarios.push_back(MakeCareerSimulationScenario(
      "venue_home_equal", 70, 70, true, "Balanced", 70, 50, 100, 11,
      venueSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "venue_away_equal", 70, 70, false, "Balanced", 70, 50, 100, 11,
      venueSeed, batchRuns));

  const unsigned int strategySeed = 15104u;
  const char* strategies[] = {"Balanced", "Attacking", "Defensive", "High Pressing",
                              "Counter Attack", "Possession"};
  const char* strategyNames[] = {"balanced", "attacking", "defensive", "high_pressing",
                                 "counter_attack", "possession"};
  for (int i = 0; i < 6; ++i) {
    scenarios.push_back(MakeCareerSimulationScenario(
        std::string("strategy_") + strategyNames[i], 72, 72, true, strategies[i], 70, 50,
        100, 11, strategySeed, batchRuns));
  }

  const unsigned int conditionSeed = 15105u;
  scenarios.push_back(MakeCareerSimulationScenario(
      "condition_low_morale", 70, 70, true, "Balanced", 30, 50, 100, 11,
      conditionSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "condition_high_morale", 70, 70, true, "Balanced", 90, 50, 100, 11,
      conditionSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "condition_low_form", 70, 70, true, "Balanced", 70, 25, 100, 11,
      conditionSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "condition_high_form", 70, 70, true, "Balanced", 70, 85, 100, 11,
      conditionSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "condition_low_fitness", 70, 70, true, "Balanced", 70, 50, 40, 11,
      conditionSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "condition_high_fitness", 70, 70, true, "Balanced", 70, 50, 100, 11,
      conditionSeed, batchRuns));

  const unsigned int edgeSeed = 15106u;
  scenarios.push_back(MakeCareerSimulationScenario(
      "edge_empty_roster", 70, 70, true, "Balanced", 70, 50, 100, 0,
      edgeSeed, batchRuns));
  scenarios.push_back(MakeCareerSimulationScenario(
      "edge_minimal_roster", 70, 70, true, "Balanced", 70, 50, 100, 1,
      edgeSeed, batchRuns));

  return scenarios;
}

}  // namespace blunted

#endif  // CAREER_SIMULATION_SCENARIOS_HPP
