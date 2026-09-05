#ifndef _HPP_GUI2_MENUTASK
#define _HPP_GUI2_MENUTASK

#include "../gamedefines.hpp"
#include "../presentation/ui/rmlui/frontend_runtime_bridge.hpp"
#include "ingame/gamepage.hpp"
#include "scene/scene3d/scene3d.hpp"
#include "utils/gui2/guitask.hpp"
#include "utils/gui2/widgets/image.hpp"

class Match;
class MatchData;

using namespace blunted;

constexpr float kMenuAspectRatio = 16.0f / 9.0f;

enum e_MenuAction {
  e_MenuAction_Menu,
  e_MenuAction_Game,
  e_MenuAction_None
};

struct SideSelection {
  int controllerID = -1;
  Gui2Image* controllerImage = nullptr;
  int side = 0;
};

struct QueuedFixture {
  QueuedFixture() {
    team1KitNum = 1;
    team2KitNum = 2;
    matchData = 0;
  }
  std::vector<SideSelection> sides;
  std::string teamID1, teamID2;
  int team1KitNum, team2KitNum;
  MatchData* matchData;
};

void SetActiveController(int side, bool keyboard);

class MenuTask : public Gui2Task {
public:
  MenuTask(float aspectRatio, float margin, TTF_Font* defaultFont, TTF_Font* defaultOutlineFont);
  virtual ~MenuTask();

  virtual void ProcessPhase();

  bool QuickStart();
  void QuitGame();

  void ReleaseAllButtons();

  // Single-process Fotbiler lifecycle. Match pages request a return here;
  // MenuTask stops the live match, drains graphics/audio teardown commands,
  // then reveals the frontend in the same process/window.
  void ReturnToFotbilerFrontend(blunted::ui::frontend::ReturnTarget target);

  void SetControllerSetup(const std::vector<SideSelection>& sides) {
    queuedFixture.Lock();
    queuedFixture->sides = sides;
    queuedFixture.Unlock();
  }
  const std::vector<SideSelection> GetControllerSetup() { return queuedFixture.GetData().sides; }
  void SetTeamIDs(const std::string& id1, const std::string& id2) {
    queuedFixture.Lock();
    queuedFixture->teamID1 = id1;
    queuedFixture->teamID2 = id2;
    queuedFixture.Unlock();
  }
  int GetTeamID(int whichOne) {
    if (whichOne == 0)
      return atoi(queuedFixture.GetData().teamID1.c_str());
    else
      return atoi(queuedFixture.GetData().teamID2.c_str());
  }
  int GetTeamKitNum(int teamID) {
    if (teamID == 0)
      return queuedFixture.GetData().team1KitNum;
    else
      return queuedFixture.GetData().team2KitNum;
  }
  void SetMatchData(MatchData* matchData) {
    queuedFixture.Lock();
    queuedFixture->matchData = matchData;
    queuedFixture.Unlock();
  }
  MatchData* GetMatchData() {
    return queuedFixture.GetData().matchData;
  }

  void SetMenuAction(e_MenuAction menuAction) { this->menuAction = menuAction; }

  void SetMenuBackgroundVisible(bool visible) {
    if (!menuBackground) return;
    if (visible)
      menuBackground->Show();
    else
      menuBackground->Hide();
  }

protected:
  bool PrepareFotbilerUiDirectMatch();
  bool PrepareFotbilerUiQuickMatch();
  bool PrepareFotbilerUiQuickMatch(const blunted::ui::frontend::LaunchRequest& request);
  bool PrepareFotbilerUiCareerMatch();
  void SetSingleControlledSide(int side);
  void ApplyFotbilerDisplaySettings(const blunted::ui::frontend::DisplaySettingsRequest& request);
  void DrainFotbilerRuntimePipelines();

  e_MenuAction menuAction;
  bool uiDirectMatchReady;
  bool frontendReturnPending;
  blunted::ui::frontend::ReturnTarget frontendReturnTarget;

  Gui2Image* menuBackground;
  Lockable<QueuedFixture> queuedFixture;
};

#endif