#ifndef LEAGUE_SOCCER_FULL_3D_MATCH_ENGINE_HPP
#define LEAGUE_SOCCER_FULL_3D_MATCH_ENGINE_HPP

#include "core/match/match_engine.hpp"

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
    return NormalizeHomeAwayResult(request, engineHomeGoals, engineAwayGoals);
  }
};

}  // namespace blunted

#endif  // LEAGUE_SOCCER_FULL_3D_MATCH_ENGINE_HPP
