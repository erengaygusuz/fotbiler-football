#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/career/career_simulation_guardrails.hpp"
#include "core/career/career_simulation_scenarios.hpp"

namespace {

const blunted::CareerSimulationScenario* FindScenario(
    const std::vector<blunted::CareerSimulationScenario>& scenarios,
    const std::string& name) {
  for (const auto& scenario : scenarios) {
    if (scenario.name == name)
      return &scenario;
  }
  return nullptr;
}

void ExpectInBand(const blunted::SimulationMetricBand& band, double value,
                  const char* metric) {
  EXPECT_GE(value, band.minimum) << metric << " below guardrail";
  EXPECT_LE(value, band.maximum) << metric << " above guardrail";
}

long long ScorerGoals(const blunted::MatchSimulationTelemetry& telemetry,
                      const std::string& playerName) {
  const auto it = telemetry.userScorerGoals.find(playerName);
  return it != telemetry.userScorerGoals.end() ? it->second : 0LL;
}

TEST(GameplaySimulationGuardrails, StrengthGapGoalBandsRemainSeparated) {
  const auto guardrails = blunted::DefaultCareerSimulationGuardrails();
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(6000);
  const auto* weak = FindScenario(scenarios, "team_strength_weak");
  const auto* equal = FindScenario(scenarios, "team_strength_equal");
  const auto* strong = FindScenario(scenarios, "team_strength_strong");
  ASSERT_NE(weak, nullptr);
  ASSERT_NE(equal, nullptr);
  ASSERT_NE(strong, nullptr);

  const auto weakReport = blunted::RunCareerSimulationScenario(*weak);
  const auto equalReport = blunted::RunCareerSimulationScenario(*equal);
  const auto strongReport = blunted::RunCareerSimulationScenario(*strong);

  ExpectInBand(guardrails.weakTeamUserGoals,
               weakReport.telemetry.AverageUserGoals(), "weak user goals");
  ExpectInBand(guardrails.equalTeamUserGoals,
               equalReport.telemetry.AverageUserGoals(), "equal user goals");
  ExpectInBand(guardrails.strongTeamUserGoals,
               strongReport.telemetry.AverageUserGoals(), "strong user goals");

  EXPECT_LT(weakReport.telemetry.AverageUserGoals(),
            equalReport.telemetry.AverageUserGoals());
  EXPECT_LT(equalReport.telemetry.AverageUserGoals(),
            strongReport.telemetry.AverageUserGoals());
}

TEST(GameplaySimulationGuardrails, EqualHomeDistributionStaysInsideFootballBands) {
  const auto guardrails = blunted::DefaultCareerSimulationGuardrails();
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(8000);
  const auto* equal = FindScenario(scenarios, "opposition_equal");
  ASSERT_NE(equal, nullptr);

  const auto report = blunted::RunCareerSimulationScenario(*equal);
  const auto& telemetry = report.telemetry;
  ASSERT_EQ(telemetry.completedRuns, 8000);

  ExpectInBand(guardrails.equalTeamUserGoals, telemetry.AverageUserGoals(),
               "equal-home user goals");
  ExpectInBand(guardrails.equalHomeOpponentGoals, telemetry.AverageOpponentGoals(),
               "equal-home opponent goals");
  ExpectInBand(guardrails.equalHomeTotalGoals, telemetry.AverageTotalGoals(),
               "equal-home total goals");
  ExpectInBand(guardrails.equalHomeWinRate, telemetry.WinRate(),
               "equal-home win rate");
  ExpectInBand(guardrails.equalHomeDrawRate, telemetry.DrawRate(),
               "equal-home draw rate");
  ExpectInBand(guardrails.equalHomeLossRate, telemetry.LossRate(),
               "equal-home loss rate");
  ExpectInBand(guardrails.equalHomeUserShotConversion, telemetry.UserShotConversion(),
               "equal-home user shot conversion");
  ExpectInBand(guardrails.equalHomeOpponentShotConversion,
               telemetry.OpponentShotConversion(),
               "equal-home opponent shot conversion");
  ExpectInBand(guardrails.equalHomePossession, telemetry.AverageUserPossession(),
               "equal-home possession");

  EXPECT_NEAR(telemetry.WinRate() + telemetry.DrawRate() + telemetry.LossRate(),
              1.0, 1e-12);
}

TEST(GameplaySimulationGuardrails, HomeAdvantageStaysPositiveButBounded) {
  const auto guardrails = blunted::DefaultCareerSimulationGuardrails();
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(8000);
  const auto* home = FindScenario(scenarios, "venue_home_equal");
  const auto* away = FindScenario(scenarios, "venue_away_equal");
  ASSERT_NE(home, nullptr);
  ASSERT_NE(away, nullptr);

  const auto homeReport = blunted::RunCareerSimulationScenario(*home);
  const auto awayReport = blunted::RunCareerSimulationScenario(*away);
  const double advantage = homeReport.telemetry.AverageGoalDifference() -
                           awayReport.telemetry.AverageGoalDifference();

  ExpectInBand(guardrails.homeGoalDifferenceAdvantage, advantage,
               "home goal-difference advantage");
}

TEST(GameplaySimulationGuardrails, StrategyPossessionEffectsStayInsideBands) {
  const auto guardrails = blunted::DefaultCareerSimulationGuardrails();
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(5000);
  const auto* possession = FindScenario(scenarios, "strategy_possession");
  const auto* counter = FindScenario(scenarios, "strategy_counter_attack");
  ASSERT_NE(possession, nullptr);
  ASSERT_NE(counter, nullptr);

  const auto possessionReport = blunted::RunCareerSimulationScenario(*possession);
  const auto counterReport = blunted::RunCareerSimulationScenario(*counter);

  ExpectInBand(guardrails.possessionStrategyPossession,
               possessionReport.telemetry.AverageUserPossession(),
               "possession-strategy possession");
  ExpectInBand(guardrails.counterAttackPossession,
               counterReport.telemetry.AverageUserPossession(),
               "counter-attack possession");
  EXPECT_GT(possessionReport.telemetry.AverageUserPossession(),
            counterReport.telemetry.AverageUserPossession() + 12.0);
}

TEST(GameplaySimulationGuardrails, ScorerDistributionFavorsForwardsAndNeverGoalkeeper) {
  const auto guardrails = blunted::DefaultCareerSimulationGuardrails();
  const auto scenarios = blunted::BuildDefaultCareerSimulationScenarioCatalogue(12000);
  const auto* equal = FindScenario(scenarios, "opposition_equal");
  ASSERT_NE(equal, nullptr);

  const auto report = blunted::RunCareerSimulationScenario(*equal);
  const auto& telemetry = report.telemetry;
  ASSERT_GT(telemetry.userGoals, 0);

  const long long goalkeeperGoals = ScorerGoals(telemetry, "Scenario Player 0");
  const long long forwardGoals =
      ScorerGoals(telemetry, "Scenario Player 9") +
      ScorerGoals(telemetry, "Scenario Player 10");
  const double forwardShare =
      static_cast<double>(forwardGoals) / static_cast<double>(telemetry.userGoals);

  EXPECT_EQ(goalkeeperGoals, 0);
  ExpectInBand(guardrails.forwardScorerShare, forwardShare,
               "CF/ST scorer share");
}

}  // namespace
