#include <string>

#include <gtest/gtest.h>

#include "core/career/career_common.hpp"
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

}  // namespace
