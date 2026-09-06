#ifndef FOTBILER_MATCH_ENGINE_HPP
#define FOTBILER_MATCH_ENGINE_HPP

#include <string>
#include <vector>

namespace blunted {

struct MatchRequest {
  std::string opponentName;
  std::string opponentTeamDBID;
  bool userIsHome = true;
};

// Canonical application-facing result. Scores are always from the controlled
// club's perspective, independent of whether the underlying engine uses
// home/away, team-0/team-1, or another representation.
//
// Advanced-stat availability is explicit because not every engine currently
// exposes the same canonical data. In particular, Full3D currently supplies a
// trustworthy final score while shots, possession and scorers remain unknown.
struct MatchResult {
  int userGoals = 0;
  int opponentGoals = 0;
  int userShots = 0;
  int opponentShots = 0;
  int userPossession = 50;
  std::vector<std::string> userScorers;
  bool shotsAvailable = false;
  bool possessionAvailable = false;
  bool scorersAvailable = false;
  bool completed = false;
};

enum class MatchEngineKind { FastSimulation, Full3D };

struct MatchEngineRun {
  bool requiresInteractivePlay = false;
  MatchResult result;
};

// A match engine may resolve immediately (CPU/fast simulation) or request an
// interactive match. Interactive engines normalize their native score through
// Complete() when the playable match finishes.
class IMatchEngine {
public:
  virtual ~IMatchEngine() = default;

  virtual MatchEngineKind GetKind() const = 0;
  virtual MatchEngineRun Start(const MatchRequest& request) = 0;
  virtual MatchResult Complete(const MatchRequest& request, int engineHomeGoals,
                               int engineAwayGoals) const = 0;
};

inline MatchResult NormalizeHomeAwayResult(const MatchRequest& request, int homeGoals,
                                           int awayGoals) {
  MatchResult result;
  result.userGoals = request.userIsHome ? homeGoals : awayGoals;
  result.opponentGoals = request.userIsHome ? awayGoals : homeGoals;
  result.completed = true;
  return result;
}

}  // namespace blunted

#endif  // FOTBILER_MATCH_ENGINE_HPP
