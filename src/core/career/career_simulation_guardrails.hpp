#ifndef CAREER_SIMULATION_GUARDRAILS_HPP
#define CAREER_SIMULATION_GUARDRAILS_HPP

namespace blunted {

// Statistical guardrails are intentionally bands rather than golden numbers.
// They should reject obviously broken football/simulation behaviour while
// leaving room for deliberate tuning inside a plausible envelope.
struct SimulationMetricBand {
  double minimum = 0.0;
  double maximum = 0.0;

  bool Contains(double value) const { return value >= minimum && value <= maximum; }
};

struct CareerSimulationGuardrails {
  SimulationMetricBand weakTeamUserGoals;
  SimulationMetricBand equalTeamUserGoals;
  SimulationMetricBand strongTeamUserGoals;

  SimulationMetricBand equalHomeOpponentGoals;
  SimulationMetricBand equalHomeTotalGoals;
  SimulationMetricBand equalHomeWinRate;
  SimulationMetricBand equalHomeDrawRate;
  SimulationMetricBand equalHomeLossRate;
  SimulationMetricBand equalHomeUserShotConversion;
  SimulationMetricBand equalHomePossession;

  // Difference between paired home and away average goal difference.
  SimulationMetricBand homeGoalDifferenceAdvantage;

  SimulationMetricBand possessionStrategyPossession;
  SimulationMetricBand counterAttackPossession;

  // Share of user goals scored by CF/ST in the canonical 11-player scenario.
  SimulationMetricBand forwardScorerShare;
};

inline CareerSimulationGuardrails DefaultCareerSimulationGuardrails() {
  CareerSimulationGuardrails guardrails;

  // Current fast-sim baseline around a 70-rated opponent. These envelopes are
  // deliberately wider than the seeded sample variance so normal tuning does
  // not require changing tests for tiny statistical movement.
  guardrails.weakTeamUserGoals = {0.15, 0.65};
  guardrails.equalTeamUserGoals = {0.85, 1.45};
  guardrails.strongTeamUserGoals = {1.65, 2.55};

  guardrails.equalHomeOpponentGoals = {0.70, 1.25};
  guardrails.equalHomeTotalGoals = {1.75, 2.60};
  guardrails.equalHomeWinRate = {0.32, 0.48};
  guardrails.equalHomeDrawRate = {0.24, 0.36};
  guardrails.equalHomeLossRate = {0.24, 0.38};
  guardrails.equalHomeUserShotConversion = {0.13, 0.25};
  guardrails.equalHomePossession = {51.5, 54.5};

  guardrails.homeGoalDifferenceAdvantage = {0.18, 0.50};

  guardrails.possessionStrategyPossession = {61.0, 65.0};
  guardrails.counterAttackPossession = {43.0, 47.0};

  guardrails.forwardScorerShare = {0.60, 0.77};
  return guardrails;
}

}  // namespace blunted

#endif  // CAREER_SIMULATION_GUARDRAILS_HPP
