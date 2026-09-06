#include <gtest/gtest.h>

#include "core/career/career_common.hpp"
#include "core/career/fast_match_engine.hpp"
#include "core/match/runtime_match_telemetry.hpp"
#include "integration/league_soccer/full_3d_match_engine.hpp"

namespace {

TEST(GameplaySimulationLab, CompletedFull3DHomeScoreFeedsSharedTelemetry) {
  auto& runtime = blunted::RuntimeMatchTelemetry::GetInstance();
  runtime.Reset();

  blunted::MatchRequest request;
  request.userIsHome = true;

  blunted::Full3DMatchEngine engine;
  const blunted::MatchResult result = engine.Complete(request, 3, 1);
  const auto& telemetry = runtime.GetFull3DTelemetry();

  ASSERT_TRUE(result.completed);
  EXPECT_EQ(result.userGoals, 3);
  EXPECT_EQ(result.opponentGoals, 1);
  EXPECT_EQ(telemetry.requestedRuns, 1);
  EXPECT_EQ(telemetry.interactiveRuns, 1);
  EXPECT_EQ(telemetry.completedRuns, 1);
  EXPECT_EQ(telemetry.completedInteractiveRuns, 1);
  EXPECT_EQ(telemetry.wins, 1);
  EXPECT_DOUBLE_EQ(telemetry.AverageUserGoals(), 3.0);
  EXPECT_DOUBLE_EQ(telemetry.AverageOpponentGoals(), 1.0);
}

TEST(GameplaySimulationLab, CompletedFull3DAwayScoreNormalizesBeforeTelemetry) {
  auto& runtime = blunted::RuntimeMatchTelemetry::GetInstance();
  runtime.Reset();

  blunted::MatchRequest request;
  request.userIsHome = false;

  blunted::Full3DMatchEngine engine;
  const blunted::MatchResult result = engine.Complete(request, 2, 4);
  const auto& telemetry = runtime.GetFull3DTelemetry();

  EXPECT_EQ(result.userGoals, 4);
  EXPECT_EQ(result.opponentGoals, 2);
  ASSERT_EQ(telemetry.completedRuns, 1);
  EXPECT_EQ(telemetry.userGoals, 4);
  EXPECT_EQ(telemetry.opponentGoals, 2);
  EXPECT_EQ(telemetry.wins, 1);
}

TEST(GameplaySimulationLab, Full3DDoesNotFabricateUnavailableAdvancedStats) {
  auto& runtime = blunted::RuntimeMatchTelemetry::GetInstance();
  runtime.Reset();

  blunted::MatchRequest request;
  request.userIsHome = true;

  blunted::Full3DMatchEngine engine;
  const blunted::MatchResult result = engine.Complete(request, 1, 1);
  const auto& telemetry = runtime.GetFull3DTelemetry();

  ASSERT_TRUE(result.completed);
  EXPECT_FALSE(result.shotsAvailable);
  EXPECT_FALSE(result.possessionAvailable);
  EXPECT_FALSE(result.scorersAvailable);
  EXPECT_FALSE(telemetry.HasShotStats());
  EXPECT_FALSE(telemetry.HasPossessionStats());
  EXPECT_FALSE(telemetry.HasScorerStats());
  EXPECT_EQ(telemetry.shotStatRuns, 0);
  EXPECT_EQ(telemetry.possessionStatRuns, 0);
  EXPECT_EQ(telemetry.scorerStatRuns, 0);
  EXPECT_DOUBLE_EQ(telemetry.AverageUserShots(), 0.0);
  EXPECT_DOUBLE_EQ(telemetry.AverageUserPossession(), 0.0);
  EXPECT_TRUE(telemetry.userScorerGoals.empty());
}

TEST(GameplaySimulationLab, FastAndFull3DComparisonUsesOnlySharedMetrics) {
  auto& runtime = blunted::RuntimeMatchTelemetry::GetInstance();
  runtime.Reset();

  CareerSave save;
  blunted::CareerCommon::SeedRng(15107);

  blunted::MatchRequest request;
  request.opponentTeamDBID = "25";
  request.userIsHome = true;

  blunted::FastMatchEngine fastEngine(save);
  const blunted::MatchEngineRun fastRun = fastEngine.Start(request);
  ASSERT_TRUE(fastRun.result.completed);

  blunted::Full3DMatchEngine full3DEngine;
  full3DEngine.Complete(request, 2, 1);

  const auto& fast = runtime.GetFastTelemetry();
  const auto& full3D = runtime.GetFull3DTelemetry();
  const auto comparison = runtime.CompareFull3DAgainstFast();

  EXPECT_TRUE(fast.HasShotStats());
  EXPECT_TRUE(fast.HasPossessionStats());
  EXPECT_TRUE(fast.HasScorerStats());
  EXPECT_FALSE(full3D.HasShotStats());
  EXPECT_FALSE(full3D.HasPossessionStats());
  EXPECT_FALSE(full3D.HasScorerStats());

  EXPECT_TRUE(comparison.scoreComparable);
  EXPECT_FALSE(comparison.shotsComparable);
  EXPECT_FALSE(comparison.possessionComparable);
  EXPECT_FALSE(comparison.scorersComparable);
  EXPECT_DOUBLE_EQ(comparison.userGoalsDelta,
                   full3D.AverageUserGoals() - fast.AverageUserGoals());
  EXPECT_DOUBLE_EQ(comparison.opponentGoalsDelta,
                   full3D.AverageOpponentGoals() - fast.AverageOpponentGoals());
}

}  // namespace
