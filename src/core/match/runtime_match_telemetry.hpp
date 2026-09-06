#ifndef FOTBILER_RUNTIME_MATCH_TELEMETRY_HPP
#define FOTBILER_RUNTIME_MATCH_TELEMETRY_HPP

#include "match_simulation_lab.hpp"

namespace blunted {

// Process-local diagnostics for real match-engine usage. This is intentionally
// not persisted in career saves: it is an observability surface for comparing
// completed FastSimulation and Full3D samples during development/tuning.
class RuntimeMatchTelemetry final {
public:
  static RuntimeMatchTelemetry& GetInstance() {
    static RuntimeMatchTelemetry instance;
    return instance;
  }

  void RecordFastRun(const MatchEngineRun& run) {
    ++fast.requestedRuns;
    fast.Record(run);
  }

  void RecordFull3DCompletion(const MatchResult& result) {
    // Full3D reaches this point only after an interactive match has completed.
    // The current lifecycle does not call IMatchEngine::Start(), so count the
    // observed interactive request and completion together here.
    ++full3D.requestedRuns;
    ++full3D.interactiveRuns;
    full3D.RecordCompletedResult(result, true);
  }

  const MatchSimulationTelemetry& GetFastTelemetry() const { return fast; }
  const MatchSimulationTelemetry& GetFull3DTelemetry() const { return full3D; }

  MatchSimulationComparison CompareFull3DAgainstFast() const {
    return CompareMatchSimulationTelemetry(fast, full3D);
  }

  void Reset() {
    fast = MatchSimulationTelemetry{};
    full3D = MatchSimulationTelemetry{};
  }

private:
  RuntimeMatchTelemetry() = default;

  MatchSimulationTelemetry fast;
  MatchSimulationTelemetry full3D;
};

}  // namespace blunted

#endif  // FOTBILER_RUNTIME_MATCH_TELEMETRY_HPP
