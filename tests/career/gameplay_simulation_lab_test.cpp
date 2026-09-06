#include <string>

#include <gtest/gtest.h>

#include "core/career/career_common.hpp"
#include "core/career/career_simulation_scenarios.hpp"
#include "core/career/fast_match_engine.hpp"
#include "core/match/match_simulation_lab.hpp"
#include "integration/league_soccer/full_3d_match_engine.hpp"

namespace {

CareerSave BuildRoster(int overall) {
  CareerSave save;
  save.name = "Simulation Lab FC";
  save.activeStrategy = "Balanced";

  static const char* positions[] = {"GK", "CB", "CB", "LB", "RB", "DM",
                                    "CM", "CM", "AM", "CF", "ST"};

  for (int i = 0; i < 11; ++i) {
    PlayerCareerState player;
    player.name = "Lab Player " + std::to_string(i);
    player.position = positions[i];
    player.preferredPosition = positions[i];
    player.ovr = overall;
    player.pot = overall;
    player.morale = 70;
    player.matchForm = 50;
    player.fitness = 100;
    save.roster.push_back(player);
  }

  return save;
}

blunted::MatchRequest DefaultRequest() {
  blunted::MatchRequest request;
  request.opponentName = "M1.5 Reference Opponent";
  request.opponentTeamDBID = "150";
  request.userIsHome = true;
  return request;
}

void ExpectSameTelemetry(const blunted::MatchSimulationTelemetry& a,
                         const blunted::MatchSimulationTelemetry& b) {
  EXPECT_EQ(a.requestedRuns, b.requestedRuns);
  EXPECT_EQ(a.completedRuns, b.completedRuns);
  EXPECT_EQ(a.interactiveRuns, b.interactiveRuns);
  EXPECT_EQ(a.wins, b.wins);
  EXPECT_EQ(a.draws, b.draws);
  EXPECT_EQ(a.losses, b.losses);
  EXPECT_EQ(a.userGoals, b.userGoals);
  EXPECT_EQ(a.opponentGoals, b.opponentGoals);
  EXPECT_EQ(a.userShots, b.userShots);
  EXPECT_EQ(a.opponentShots, b.opponentShots);
  EXPECT_EQ(a.userPossession, b.userPossession);
}

const blunted::CareerSimulationScenario* FindScenario(
    const std::vector<blunted::CareerSimulationScenario>& scenarios,
    const std::string& name) {
  for (const auto& scenario : scenarios) {
    if (scenario.name == name)
      return &scenario;
  }
  return nullptr;
}

int CountScenarioPrefix(const std::vector<blunted::CareerSimulationScenario>& scenarios,
                        const std::string& prefix) {
  int count = 0;
  for (const auto& scenario : scenarios) {
    if (scenario.name.rfind(prefix, 0) == 0)
      ++count;
  }
  return count;
}

TEST(GameplaySimulationLab, SeededFastBatchIsReproducible) {
  CareerSave firstSave = BuildRoster(74);
  blunted::FastMatchEngine firstEngine(firstSave);
  blunted::CareerCommon::SeedRng(20260906u);
  const auto first =
      blunted::RunMatchSimulationBatch(firstEngine, DefaultRequest(), 250);

  CareerSave secondSave = BuildRoster(74);
  blunted::FastMatchEngine secondEngine(secondSave);
  blunted::CareerCommon::SeedRng(20260906u);
  const auto second =
      blunted::RunMatchSimulationBatch(secondEngine, DefaultRequest(), 250);

  ExpectSameTelemetry(first, second);
  EXPECT_EQ(first.completedRuns, 250);
  EXPECT_EQ(first.interactiveRuns, 0);
  EXPECT_EQ(first.wins + first.draws + first.losses, first.completedRuns);
}

TEST(GameplaySimulationLab, StrongerSquadProducesBetterBatchGoalDifference) {
  CareerSave strongSave = BuildRoster(90);
  blunted::FastMatchEngine strongEngine(strongSave);
  blunted::CareerCommon::SeedRng(7331u);
  const auto strong =
      blunted::RunMatchSimulationBatch(strongEngine, DefaultRequest(), 500);

  CareerSave weakSave = BuildRoster(50);
  blunted::FastMatchEngine weakEngine(weakSave);
  blunted::CareerCommon::SeedRng(7331u);
  const auto weak =
      blunted::RunMatchSimulationBatch(weakEngine, DefaultRequest(), 500);

  ASSERT_EQ(strong.completedRuns, 500);
  ASSERT_EQ(weak.completedRuns, 500);
  EXPECT_GT(strong.AverageGoalDifference(), weak.AverageGoalDifference());
  EXPECT_GT(strong.WinRate(), weak.WinRate());
}

TEST(GameplaySimulationLab, InteractiveRunsAreSeparatedFromHeadlessResults) {
  blunted::Full3DMatchEngine engine;
  const auto telemetry =
      blunted::RunMatchSimulationBatch(engine, DefaultRequest(), 4);

  EXPECT_EQ(telemetry.requestedRuns, 4);
  EXPECT_EQ(telemetry.interactiveRuns, 4);
  EXPECT_EQ(telemetry.completedRuns, 0);
  EXPECT_DOUBLE_EQ(telemetry.CompletionRate(), 0.0);
}

TEST(GameplaySimulationLab, NonPositiveBatchSizeRunsNothing) {
  CareerSave save = BuildRoster(70);
  blunted::FastMatchEngine engine(save);
  const auto telemetry =
      blunted::RunMatchSimulationBatch(engine, DefaultRequest(), -10);

  EXPECT_EQ(telemetry.requestedRuns, 0);
  EXPECT_EQ(telemetry.completedRuns, 0);
  EXPECT_EQ(telemetry.interactiveRuns, 0);
}

TEST(GameplaySimulationLab, DefaultScenarioCatalogueCoversMilestoneMatrix) {
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(40);

  EXPECT_EQ(scenarios.size(), 22u);
  EXPECT_EQ(CountScenarioPrefix(scenarios, "team_strength_"), 3);
  EXPECT_EQ(CountScenarioPrefix(scenarios, "opposition_"), 3);
  EXPECT_EQ(CountScenarioPrefix(scenarios, "venue_"), 2);
  EXPECT_EQ(CountScenarioPrefix(scenarios, "strategy_"), 6);
  EXPECT_EQ(CountScenarioPrefix(scenarios, "condition_"), 6);
  EXPECT_EQ(CountScenarioPrefix(scenarios, "edge_"), 2);

  for (const auto& scenario : scenarios)
    EXPECT_EQ(scenario.runs, 40) << scenario.name;
}

TEST(GameplaySimulationLab, NumericOpponentRatingsUseStableExplicitIds) {
  EXPECT_EQ(blunted::StableOpponentTeamIdForRating(45), "0");
  EXPECT_EQ(blunted::StableOpponentTeamIdForRating(65), "20");
  EXPECT_EQ(blunted::StableOpponentTeamIdForRating(88), "43");
  EXPECT_EQ(blunted::StableOpponentTeamIdForRating(1), "0");
  EXPECT_EQ(blunted::StableOpponentTeamIdForRating(999), "43");
}

TEST(GameplaySimulationLab, StrongerOppositionReducesBatchPerformance) {
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(1200);
  const auto* weakOpponent = FindScenario(scenarios, "opposition_weak");
  const auto* strongOpponent = FindScenario(scenarios, "opposition_strong");
  ASSERT_NE(weakOpponent, nullptr);
  ASSERT_NE(strongOpponent, nullptr);

  const auto weakReport = blunted::RunCareerSimulationScenario(*weakOpponent);
  const auto strongReport = blunted::RunCareerSimulationScenario(*strongOpponent);

  ASSERT_EQ(weakReport.telemetry.completedRuns, 1200);
  ASSERT_EQ(strongReport.telemetry.completedRuns, 1200);
  EXPECT_GT(weakReport.telemetry.AverageGoalDifference(),
            strongReport.telemetry.AverageGoalDifference());
  EXPECT_GT(weakReport.telemetry.WinRate(), strongReport.telemetry.WinRate());
}

TEST(GameplaySimulationLab, HomeVenueOutperformsPairedAwayScenario) {
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(1200);
  const auto* home = FindScenario(scenarios, "venue_home_equal");
  const auto* away = FindScenario(scenarios, "venue_away_equal");
  ASSERT_NE(home, nullptr);
  ASSERT_NE(away, nullptr);

  const auto homeReport = blunted::RunCareerSimulationScenario(*home);
  const auto awayReport = blunted::RunCareerSimulationScenario(*away);

  EXPECT_GT(homeReport.telemetry.AverageGoalDifference(),
            awayReport.telemetry.AverageGoalDifference());
  EXPECT_GT(homeReport.telemetry.AverageUserPossession(),
            awayReport.telemetry.AverageUserPossession());
}

TEST(GameplaySimulationLab, PossessionStrategyControlsMoreBallThanCounterAttack) {
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(800);
  const auto* possession = FindScenario(scenarios, "strategy_possession");
  const auto* counter = FindScenario(scenarios, "strategy_counter_attack");
  ASSERT_NE(possession, nullptr);
  ASSERT_NE(counter, nullptr);

  const auto possessionReport = blunted::RunCareerSimulationScenario(*possession);
  const auto counterReport = blunted::RunCareerSimulationScenario(*counter);

  EXPECT_GT(possessionReport.telemetry.AverageUserPossession(),
            counterReport.telemetry.AverageUserPossession());
}

TEST(GameplaySimulationLab, BetterMoraleAndFormImproveLargeSampleGoalDifference) {
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(1600);
  const auto* lowMorale = FindScenario(scenarios, "condition_low_morale");
  const auto* highMorale = FindScenario(scenarios, "condition_high_morale");
  const auto* lowForm = FindScenario(scenarios, "condition_low_form");
  const auto* highForm = FindScenario(scenarios, "condition_high_form");
  ASSERT_NE(lowMorale, nullptr);
  ASSERT_NE(highMorale, nullptr);
  ASSERT_NE(lowForm, nullptr);
  ASSERT_NE(highForm, nullptr);

  const auto lowMoraleReport = blunted::RunCareerSimulationScenario(*lowMorale);
  const auto highMoraleReport = blunted::RunCareerSimulationScenario(*highMorale);
  const auto lowFormReport = blunted::RunCareerSimulationScenario(*lowForm);
  const auto highFormReport = blunted::RunCareerSimulationScenario(*highForm);

  EXPECT_GT(highMoraleReport.telemetry.AverageGoalDifference(),
            lowMoraleReport.telemetry.AverageGoalDifference());
  EXPECT_GT(highFormReport.telemetry.AverageGoalDifference(),
            lowFormReport.telemetry.AverageGoalDifference());
}

TEST(GameplaySimulationLab, FitnessAndRosterEdgeScenariosRemainRunnable) {
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(80);
  const char* names[] = {"condition_low_fitness", "condition_high_fitness",
                         "edge_empty_roster", "edge_minimal_roster"};

  for (const char* name : names) {
    const auto* scenario = FindScenario(scenarios, name);
    ASSERT_NE(scenario, nullptr) << name;
    const auto report = blunted::RunCareerSimulationScenario(*scenario);
    EXPECT_EQ(report.telemetry.completedRuns, 80) << name;
    EXPECT_EQ(report.telemetry.interactiveRuns, 0) << name;
  }
}

}  // namespace
