#ifndef CAREER_FIXTURE_SESSION_HPP
#define CAREER_FIXTURE_SESSION_HPP

#include <string>

namespace blunted {

// Runtime bridge between the career fixture calendar and a played 3D match.
// This is application/session state rather than menu presentation state.
struct CareerPendingFixture {
  bool hasFixture = false;
  bool isHome = true;
  int userTeamDBID = 0;
  int opponentTeamDBID = 0;
  std::string opponentName;
};

}  // namespace blunted

#endif  // CAREER_FIXTURE_SESSION_HPP
