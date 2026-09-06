#ifndef LEAGUE_SOCCER_FULL_3D_MATCH_ENGINE_HPP
#define LEAGUE_SOCCER_FULL_3D_MATCH_ENGINE_HPP

#include "core/match/match_engine.hpp"
#include "core/match/runtime_match_telemetry.hpp"

namespace blunted {

// Adapter boundary for League-Soccer's playable match engine. Start() signals
// that presentation must launch the existing interactive match flow; when that
// flow finishes, GameOver supplies League-Soccer team-0/team-1 (home/away)
// scores to Complete(), which normalizes them to the controlled club's view.
class Full3DMatchEngine final : public IMatchEngine {
public:
  MatchEngineKind GetKind() const override { return MatchEngineKind::Full3D; }

  MatchEngineRun Start(const MatchRequest&) override {
    MatchEngineRun run;
    run.requiresInteractivePlay = true;
    return run;
  }

  MatchResult Complete(const MatchRequest& request, int engineHomeGoals,
                       int engineAwayGoals) const override {
    MatchResult result = NormalizeHomeAwayResult(request, engineHomeGoals, engineAwayGoals);
    // The playable engine currently exposes only the canonical final score at
    // this boundary. Advanced-stat availability remains false until the 3D
    // engine supplies trustworthy shots/possession/scorer data.
    RuntimeMatchTelemetry::GetInstance().RecordFull3DCompletion(result);
    return result;
  }
};

}  // namespace blunted

#endif  // LEAGUE_SOCCER_FULL_3D_MATCH_ENGINE_HPP
