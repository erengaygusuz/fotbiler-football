#include "playerhud.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "../../data/playerdata.hpp"
#include "../../onthepitch/humangamer.hpp"
#include "../../onthepitch/match.hpp"
#include "../../onthepitch/player/controller/humancontroller.hpp"
#include "../../onthepitch/player/player.hpp"
#include "../../onthepitch/team.hpp"
#include "utils/gui2/windowmanager.hpp"

using namespace blunted;

Gui2PlayerHUD::Gui2PlayerHUD(Gui2WindowManager* windowManager, Match* match)
    : Gui2View(windowManager, "player_hud", 0, 0, 100, 100), match(match) {
  Vector3 textColor = 255;
  Vector3 textOutlineColor = 0;
  Vector3 roleColor(190, 212, 220);

  // Compact lower-third panels: names sit closer to the screen edges while the
  // radar retains the central lower area.
  roleCaption[0] = new Gui2Caption(windowManager, "hud_player0_role", 2.5f, 91.2f, 4.5f, 2.5f, "");
  roleCaption[0]->SetColor(roleColor);
  roleCaption[0]->SetOutlineColor(textOutlineColor);
  this->AddView(roleCaption[0]);
  roleCaption[0]->Show();

  playerNameCaption[0] = new Gui2Caption(windowManager, "hud_player0_name", 7.2f, 91.0f, 15, 2.8f, "");
  playerNameCaption[0]->SetColor(textColor);
  playerNameCaption[0]->SetOutlineColor(textOutlineColor);
  this->AddView(playerNameCaption[0]);
  playerNameCaption[0]->Show();

  conditionCaption[0] = new Gui2Caption(windowManager, "hud_player0_cond", 22.2f, 91.2f, 4, 2.5f, "");
  conditionCaption[0]->SetOutlineColor(textOutlineColor);
  this->AddView(conditionCaption[0]);
  conditionCaption[0]->Show();

  staminaImage[0] = new Gui2Image(windowManager, "hud_player0_stm", 2.5f, 94.4f, 19.5f, 0.85f);
  this->AddView(staminaImage[0]);
  staminaImage[0]->Show();

  powerGaugeImage[0] = new Gui2Image(windowManager, "hud_player0_power", 2.5f, 88.8f, 19.5f, 0.85f);
  this->AddView(powerGaugeImage[0]);
  powerGaugeImage[0]->Hide();

  roleCaption[1] = new Gui2Caption(windowManager, "hud_player1_role", 74, 91.2f, 4.5f, 2.5f, "");
  roleCaption[1]->SetColor(roleColor);
  roleCaption[1]->SetOutlineColor(textOutlineColor);
  this->AddView(roleCaption[1]);
  roleCaption[1]->Show();

  playerNameCaption[1] = new Gui2Caption(windowManager, "hud_player1_name", 78.7f, 91.0f, 15, 2.8f, "");
  playerNameCaption[1]->SetColor(textColor);
  playerNameCaption[1]->SetOutlineColor(textOutlineColor);
  this->AddView(playerNameCaption[1]);
  playerNameCaption[1]->Show();

  conditionCaption[1] = new Gui2Caption(windowManager, "hud_player1_cond", 93.8f, 91.2f, 3.7f, 2.5f, "");
  conditionCaption[1]->SetOutlineColor(textOutlineColor);
  this->AddView(conditionCaption[1]);
  conditionCaption[1]->Show();

  staminaImage[1] = new Gui2Image(windowManager, "hud_player1_stm", 78, 94.4f, 19.5f, 0.85f);
  this->AddView(staminaImage[1]);
  staminaImage[1]->Show();

  powerGaugeImage[1] = new Gui2Image(windowManager, "hud_player1_power", 78, 88.8f, 19.5f, 0.85f);
  this->AddView(powerGaugeImage[1]);
  powerGaugeImage[1]->Hide();

  for (int i = 0; i < 2; i++) {
    lastStamina[i] = -1.0f;
    lastPowerGauge[i] = -1.0f;
  }
  this->Show();
}

Gui2PlayerHUD::~Gui2PlayerHUD() {}

void Gui2PlayerHUD::Put() {
  if (!match) return;

  for (int t = 0; t < 2; t++) {
    Team* team = match->GetTeam(t);
    if (!team) continue;

    Player* activePlayer = nullptr;
    const std::vector<HumanGamer*>& gamers = team->GetHumanGamers();
    if (!gamers.empty() && gamers[0]->GetSelectedPlayerID() >= 0) {
      activePlayer = team->GetPlayer(gamers[0]->GetSelectedPlayerID());
    }
    if (!activePlayer) activePlayer = team->GetDesignatedTeamPossessionPlayer();
    if (!activePlayer) activePlayer = team->GetLastTouchPlayer();

    auto drawBar = [](boost::intrusive_ptr<Image2D>& img, float factor, const Vector3& color) {
      if (!img) return;
      Vector3 imgSize = img->GetSize();
      int w = static_cast<int>(imgSize.coords[0]);
      int h = static_cast<int>(imgSize.coords[1]);
      if (w <= 2 || h <= 2) return;

      img->DrawRectangle(0, 0, w, h, Vector3(8, 24, 35), 220);
      int fillW = static_cast<int>(std::clamp(factor, 0.0f, 1.0f) * (w - 2));
      if (fillW > 0) {
        img->DrawRectangle(1, 1, fillW, std::max(1, h - 2), color, 235);
      }
      img->DrawRectangle(0, 0, w, 1, Vector3(220, 235, 240), 120);
      img->OnChange();
    };

    if (activePlayer && activePlayer->GetPlayerData()) {
      PlayerData* pd = activePlayer->GetPlayerData();
      std::string name = pd->GetLastName();
      if (name != lastPlayerName[t]) {
        playerNameCaption[t]->SetCaption(name);
        lastPlayerName[t] = name;
      }

      std::string role = pd->GetRoleName();
      if (role != lastRole[t]) {
        roleCaption[t]->SetCaption(role);
        lastRole[t] = role;
      }

      std::string condSym = pd->GetConditionSymbol();
      if (condSym != lastCondition[t]) {
        conditionCaption[t]->SetCaption(condSym);
        conditionCaption[t]->SetColor(pd->GetConditionColor());
        lastCondition[t] = condSym;
      }

      float stamina = activePlayer->GetFatigueFactorInv();
      if (std::fabs(stamina - lastStamina[t]) > 0.02f) {
        boost::intrusive_ptr<Image2D> stmImg = staminaImage[t]->GetImage2D();
        Vector3 color = stamina < 0.25f ? Vector3(220, 60, 70) : Vector3(32, 208, 181);
        drawBar(stmImg, stamina, color);
        lastStamina[t] = stamina;
      }
    }

    float gaugeFactor = 0.0f;
    if (!gamers.empty() && gamers[0]->GetHumanController()) {
      HumanController* hc = dynamic_cast<HumanController*>(gamers[0]->GetHumanController());
      if (hc) gaugeFactor = hc->GetGaugeFactor();
    }
    if (gaugeFactor > 0.01f) {
      if (std::fabs(gaugeFactor - lastPowerGauge[t]) > 0.02f) {
        boost::intrusive_ptr<Image2D> pwrImg = powerGaugeImage[t]->GetImage2D();
        Vector3 color = gaugeFactor < 0.5f ? Vector3(32, 208, 181)
                        : gaugeFactor < 0.8f ? Vector3(245, 202, 66)
                                             : Vector3(232, 61, 134);
        drawBar(pwrImg, gaugeFactor, color);
        lastPowerGauge[t] = gaugeFactor;
      }
      powerGaugeImage[t]->Show();
    } else if (lastPowerGauge[t] > 0.01f) {
      powerGaugeImage[t]->Hide();
      lastPowerGauge[t] = 0.0f;
    }
  }
}
