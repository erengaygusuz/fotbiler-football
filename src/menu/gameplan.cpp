#include "gameplan.hpp"

#include "../main.hpp"
#include "mainmenu.hpp"
#include "utils/localization.hpp"

using namespace blunted;

GamePlanPage::GamePlanPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  teamID = pageData.properties->GetInt("teamID", 0);
  teamData = GetGameTask()->GetMatch()->GetTeam(teamID)->GetTeamData();

  Gui2Frame* frame = new Gui2Frame(windowManager, "gameplan_frame", 6, 5, 88, 90, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* kicker =
      new Gui2Caption(windowManager, "gameplan_kicker", 3, 2, 82, 2.3f, "MATCHDAY  ·  TACTICS");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(kicker);
  kicker->Show();

  Gui2Caption* header =
      new Gui2Caption(windowManager, "gameplan_header", 3, 5, 82, 4, "TEAM MANAGEMENT");
  frame->AddView(header);
  header->Show();

  Gui2Caption* subtitle = new Gui2Caption(windowManager, "gameplan_subtitle", 3, 9, 82, 2.4f,
                                          teamData->GetName() + "  ·  Lineup, tactics and match shape");
  frame->AddView(subtitle);
  subtitle->Show();

  Gui2Frame* pitchPanel = new Gui2Frame(windowManager, "gameplan_pitch_panel", 3, 15, 53, 63, true);
  frame->AddView(pitchPanel);
  pitchPanel->Show();

  Gui2Caption* pitchKicker =
      new Gui2Caption(windowManager, "gameplan_pitch_kicker", 2, 2, 49, 2.3f, "TEAM SHEET");
  pitchKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  pitchPanel->AddView(pitchKicker);
  pitchKicker->Show();

  Gui2Caption* formation =
      new Gui2Caption(windowManager, "gameplan_formation_label", 2, 6, 49, 3, "CURRENT FORMATION");
  pitchPanel->AddView(formation);
  formation->Show();

  // Keep the existing PlanMap functionality, but give it the dominant left-side
  // presentation used by modern football team-management screens.
  map = new Gui2PlanMap(windowManager, "gameplan_planmap", 4, 11, 45, 45, teamData);
  pitchPanel->AddView(map);
  map->Show();

  Gui2Frame* actionPanel = new Gui2Frame(windowManager, "gameplan_actions", 58, 15, 27, 63, true);
  frame->AddView(actionPanel);
  actionPanel->Show();

  Gui2Caption* actionKicker =
      new Gui2Caption(windowManager, "gameplan_action_kicker", 2, 2, 23, 2.3f, "MATCH PLAN");
  actionKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  actionPanel->AddView(actionKicker);
  actionKicker->Show();

  Gui2Caption* actionTitle =
      new Gui2Caption(windowManager, "gameplan_action_title", 2, 6, 23, 3, "EDIT TEAM");
  actionPanel->AddView(actionTitle);
  actionTitle->Show();

  grid = new Gui2Grid(windowManager, "gameplan_grid", 0, 0, 1, 1);
  gridNav = new Gui2Grid(windowManager, "gameplan_grid_navigation", 2, 12, 23, 31);

  buttonLineup = new Gui2Button(windowManager, "gameplan_button_lineup", 0, 0, 23, 4,
                                Localization::GetInstance().Translate("gameplan_lineup"));
  buttonTactics = new Gui2Button(windowManager, "gameplan_button_tactics", 0, 0, 23, 4,
                                 Localization::GetInstance().Translate("gameplan_tactics"));
  Gui2Button* buttonFormation =
      new Gui2Button(windowManager, "gameplan_button_formation", 0, 0, 23, 4,
                     Localization::GetInstance().Translate("gameplan_formation"));
  Gui2Button* buttonBack = new Gui2Button(windowManager, "gameplan_button_back", 0, 0, 23, 4,
                                          Localization::GetInstance().Translate("action_back"));

  buttonLineup->sig_OnClick.connect([this](...) { GoLineupMenu(); });
  buttonTactics->sig_OnClick.connect([this](...) { GoTacticsMenu(); });
  buttonBack->sig_OnClick.connect([this](...) { GoBack(); });

  if (IsReleaseVersion()) buttonLineup->SetActive(false);
  buttonFormation->SetActive(false);
  this->sig_OnClose.connect([this](...) { OnClose(); });

  gridNav->AddView(buttonLineup, 0, 0);
  gridNav->AddView(buttonTactics, 1, 0);
  gridNav->AddView(buttonFormation, 2, 0);
  gridNav->AddView(buttonBack, 3, 0);
  gridNav->UpdateLayout(0.7f);
  actionPanel->AddView(gridNav);
  gridNav->Show();

  // This hidden host grid is still used by the established submenu implementation.
  frame->AddView(grid);
  grid->AddView(gridNav, 1, 0);

  Gui2Caption* hint = new Gui2Caption(windowManager, "gameplan_hint", 3, 82, 82, 2.3f,
                                      "ENTER Select  ·  Choose two players in Lineup to swap positions");
  frame->AddView(hint);
  hint->Show();

  buttonTactics->SetFocus();
  this->Show();

  if (UpdateNonImportableDB()) {
    namedb = std::make_unique<Database>();
    bool dbSuccess = namedb->Load("databases/names.sqlite");
    if (!dbSuccess) Log(e_FatalError, "MainMenuPage", "GoImportDB", "Could not open database");
  } else {
    namedb = nullptr;
  }
}

GamePlanPage::~GamePlanPage() {}

void GamePlanPage::OnClose() { namedb.reset(); }

void GamePlanPage::Deactivate() {
  grid->RemoveView(1, 0);
}

void GamePlanPage::Reactivate() {
  grid->AddView(gridNav, 1, 0);
  gridNav->Show();
  buttonTactics->SetFocus();
}

Vector3 GamePlanPage::GetButtonColor(int id) {
  Vector3 color = windowManager->GetStyle()->GetColor(e_DecorationType_Bright1);
  if (id > 10) color = windowManager->GetStyle()->GetColor(e_DecorationType_Bright2);
  if (id > 21) color = windowManager->GetStyle()->GetColor(e_DecorationType_Dark2);
  return color;
}

void GamePlanPage::GoLineupMenu() {
  Deactivate();
  lineupMenu = new GamePlanSubMenu(windowManager, buttonLineup, grid, "lineup_submenu");
  lineupMenu->sig_OnClose.connect([this](...) { SaveLineup(); });
  lineupMenu->sig_OnClose.connect([this](...) { Reactivate(); });

  const auto& playerData = teamData->GetPlayerData();
  for (unsigned int i = 0; i < playerData.size(); i++) {
    std::string role = "[" + playerData.at(i)->GetRoleName() + "]";
    std::string name = playerData.at(i)->GetLastName();
    std::string cond = playerData.at(i)->GetConditionSymbol();
    std::string label = role + "  " + name + "   " + cond;
    Vector3 color = GetButtonColor(i);
    Gui2Button* button =
        lineupMenu->AddButton("playerbutton_id" + int_to_str(playerData.at(i)->GetDatabaseID()),
                              label, i, 0, color);
    button->sig_OnClick.connect([this](Gui2Button* btn) { LineupMenuOnClick(btn); });
    button->SetToggleable(true);
    if (i == 0) button->SetFocus();
  }
  lineupMenu->Show();
}

void GamePlanPage::LineupMenuOnClick(Gui2Button* button) {
  Gui2Button* selected = lineupMenu->GetToggledButton(button);
  if (selected) {
    selected->SetToggled(false);
    button->SetToggled(false);
    int rowSelected = lineupMenu->GetGrid()->GetRow(selected);
    int rowButton = lineupMenu->GetGrid()->GetRow(button);
    assert(rowSelected != -1 && rowButton != -1);
    lineupMenu->GetGrid()->RemoveView(rowSelected, 0);
    lineupMenu->GetGrid()->RemoveView(rowButton, 0);
    lineupMenu->GetGrid()->AddView(button, rowSelected, 0);
    button->Show();
    button->SetColor(GetButtonColor(rowSelected));
    lineupMenu->GetGrid()->AddView(selected, rowButton, 0);
    selected->Show();
    selected->SetColor(GetButtonColor(rowButton));
    lineupMenu->GetGrid()->UpdateLayout(0.5);
    selected->SetFocus();

    int id1 = atoi(selected->GetName().substr(selected->GetName().rfind("id") + 2).c_str());
    int id2 = atoi(button->GetName().substr(button->GetName().rfind("id") + 2).c_str());
    teamData->SwitchPlayers(id1, id2);
  }
}

void GamePlanPage::SaveLineup() {
  if (UpdateNonImportableDB()) {
    const std::vector<Gui2Button*>& allButtons = lineupMenu->GetAllButtons();
    for (unsigned int i = 0; i < allButtons.size(); i++) {
      Gui2View* button = lineupMenu->GetGrid()->FindView(i, 0);
      int id = atoi(button->GetName().substr(button->GetName().rfind("id") + 2).c_str());
      PlayerData* playerData = teamData->GetPlayerDataByDatabaseID(id);
      unsigned int formationorder = i;
      auto result = namedb->Query("select id from playernames where fakefirstname = \"" +
                                  playerData->GetFirstName() + "\" and fakelastname = \"" +
                                  playerData->GetLastName() + "\" limit 1;");
      if (result->data.size() > 0) {
        int playerDatabaseID = atoi(result->data.at(0).at(0).c_str());
        namedb->Query("update playernames set formationorder = " + int_to_str(formationorder) +
                      " where id = " + int_to_str(playerDatabaseID) + ";");
      } else if (Verbose()) {
        printf("WARNING: player does not exist in namedb: %s %s\n",
               playerData->GetFirstName().c_str(), playerData->GetLastName().c_str());
      }
    }
  }
  teamData->SaveLineup();
}

void GamePlanPage::GoTacticsMenu() {
  Deactivate();
  tacticsSliders.clear();
  tacticsMenu = new GamePlanSubMenu(windowManager, buttonTactics, grid, "tactics_submenu");
  tacticsMenu->sig_OnClose.connect([this](...) { SaveTactics(); });
  tacticsMenu->sig_OnClose.connect([this](...) { Reactivate(); });

  const Properties& userProps = teamData->GetTactics().userProperties;
  const map_Properties* userPropMap = userProps.GetProperties();
  const Properties& factoryProps = teamData->GetTactics().factoryProperties;

  map_Properties::const_iterator iter = userPropMap->begin();
  int i = 0;
  while (iter != userPropMap->end()) {
    const std::string& tacticName = (*iter).first;
    TacticsSlider slider;
    slider.id = i;
    slider.tacticName = tacticName;
    slider.widget = tacticsMenu->AddSlider(
        "tacticsslider_" + slider.tacticName,
        teamData->GetTactics().humanReadableNames.Get(slider.tacticName.c_str(), slider.tacticName), i, 0);
    slider.widget->AddHelperValue(Vector3(80, 80, 250), "factory default for this team",
                                  factoryProps.GetReal(slider.tacticName.c_str()));
    slider.widget->SetValue(userProps.GetReal(slider.tacticName.c_str()));
    slider.widget->sig_OnChange.connect([this, id = slider.id](Gui2Slider* s) { TacticsMenuOnChange(s, id); });
    if (i == 0) slider.widget->SetFocus();
    tacticsSliders.push_back(slider);
    ++i;
    ++iter;
  }
  tacticsMenu->Show();
}

void GamePlanPage::SaveTactics() {
  if (UpdateNonImportableDB()) {
    std::string tactics_xml;
    for (unsigned int i = 0; i < tacticsSliders.size(); i++) {
      tactics_xml += "<" + tacticsSliders.at(i).tacticName + ">" +
                     real_to_str(tacticsSliders.at(i).widget->GetValue()) + "</" +
                     tacticsSliders.at(i).tacticName + ">\n";
    }
    auto result = namedb->Query("select id from clubnames where faketargetname = \"" +
                                teamData->GetName() + "\" limit 1;");
    if (result->data.size() > 0) {
      int teamDatabaseID = atoi(result->data.at(0).at(0).c_str());
      namedb->Query("update clubnames set tactics_xml = \"" + tactics_xml +
                    "\" where id = " + int_to_str(teamDatabaseID) + ";");
    } else if (Verbose()) {
      printf("WARNING: team does not exist in namedb: %s\n", teamData->GetName().c_str());
    }
  }
  teamData->SaveTactics();
}

void GamePlanPage::TacticsMenuOnChange(Gui2Slider* slider, int id) {
  Properties& userProps = teamData->GetTacticsWritable().userProperties;
  userProps.Set(tacticsSliders.at(id).tacticName.c_str(), tacticsSliders.at(id).widget->GetValue());
}
