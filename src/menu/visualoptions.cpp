#include "visualoptions.hpp"

#include "../main.hpp"
#include "utils/localization.hpp"

using namespace blunted;

VisualOptionsPage::VisualOptionsPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "frame_visualoptions", 8, 7, 84, 86, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* kicker =
      new Gui2Caption(windowManager, "visualoptions_kicker", 3, 2, 78, 2.3f, "PRESENTATION");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(kicker);
  kicker->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_visualoptions", 3, 5, 78, 4, "VISUAL OPTIONS");
  frame->AddView(title);
  title->Show();

  Gui2Caption* subtitle = new Gui2Caption(windowManager, "visualoptions_subtitle", 3, 9, 78, 2.4f,
                                          "Change kits and match lighting while the match is paused.");
  frame->AddView(subtitle);
  subtitle->Show();

  Gui2Frame* kitPanel = new Gui2Frame(windowManager, "visualoptions_kits", 3, 16, 50, 57, true);
  frame->AddView(kitPanel);
  kitPanel->Show();

  Gui2Caption* kitKicker =
      new Gui2Caption(windowManager, "visualoptions_kit_kicker", 2, 2, 46, 2.3f, "MATCH VISUALS");
  kitKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  kitPanel->AddView(kitKicker);
  kitKicker->Show();

  Gui2Caption* kitTitle =
      new Gui2Caption(windowManager, "visualoptions_kit_title", 2, 6, 46, 3, "TEAM KITS");
  kitPanel->AddView(kitTitle);
  kitTitle->Show();

  kitSelectionPulldown[0] =
      new Gui2Pulldown(windowManager, "pulldown_visualoptions_kitselection_t1", 0, 0, 28, 4);
  kitSelectionPulldown[1] =
      new Gui2Pulldown(windowManager, "pulldown_visualoptions_kitselection_t2", 0, 0, 28, 4);

  Gui2Caption* kitSelectionCaption1 =
      new Gui2Caption(windowManager, "caption_visualoptions_kitselection_t1", 0, 0, 17, 3,
                      GetGameTask()->GetMatch()->GetTeam(0)->GetTeamData()->GetName());
  Gui2Caption* kitSelectionCaption2 =
      new Gui2Caption(windowManager, "caption_visualoptions_kitselection_t2", 0, 0, 17, 3,
                      GetGameTask()->GetMatch()->GetTeam(1)->GetTeamData()->GetName());

  kitSelectionPulldown[0]->AddEntry(TR("visualoptions_kit01"), "team1kit01");
  kitSelectionPulldown[0]->AddEntry(TR("visualoptions_kit02"), "team1kit02");
  kitSelectionPulldown[1]->AddEntry(TR("visualoptions_kit01"), "team2kit01");
  kitSelectionPulldown[1]->AddEntry(TR("visualoptions_kit02"), "team2kit02");
  kitSelectionPulldown[1]->SetSelected(1);
  kitSelectionPulldown[0]->sig_OnChange.connect(
      std::bind(&VisualOptionsPage::OnChangeKit, this, kitSelectionPulldown[0]));
  kitSelectionPulldown[1]->sig_OnChange.connect(
      std::bind(&VisualOptionsPage::OnChangeKit, this, kitSelectionPulldown[1]));

  Gui2Grid* kitGrid = new Gui2Grid(windowManager, "grid_visualoptions_kits", 2, 13, 46, 28);
  kitGrid->AddView(kitSelectionCaption1, 0, 0);
  kitGrid->AddView(kitSelectionPulldown[0], 1, 0);
  kitGrid->AddView(kitSelectionCaption2, 0, 1);
  kitGrid->AddView(kitSelectionPulldown[1], 1, 1);
  kitGrid->UpdateLayout(0.8f);
  kitPanel->AddView(kitGrid);
  kitGrid->Show();

  Gui2Frame* environmentPanel =
      new Gui2Frame(windowManager, "visualoptions_environment", 55, 16, 26, 57, true);
  frame->AddView(environmentPanel);
  environmentPanel->Show();

  Gui2Caption* envKicker =
      new Gui2Caption(windowManager, "visualoptions_env_kicker", 2, 2, 22, 2.3f, "ENVIRONMENT");
  envKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  environmentPanel->AddView(envKicker);
  envKicker->Show();

  Gui2Caption* envTitle =
      new Gui2Caption(windowManager, "visualoptions_env_title", 2, 6, 22, 3, "LIGHTING");
  environmentPanel->AddView(envTitle);
  envTitle->Show();

  Gui2Button* randomizeSunButton =
      new Gui2Button(windowManager, "button_visualoptions_randomizesun", 2, 14, 22, 4,
                     TR("visualoptions_randomize_sun"));
  randomizeSunButton->sig_OnClick.connect([this](...) { OnRandomizeSun(); });
  environmentPanel->AddView(randomizeSunButton);
  randomizeSunButton->Show();

  Gui2Button* backButton = new Gui2Button(windowManager, "button_visualoptions_back", 2, 22, 22, 4,
                                          Localization::GetInstance().Translate("action_back"));
  backButton->sig_OnClick.connect([this](...) { GoBack(); });
  environmentPanel->AddView(backButton);
  backButton->Show();

  Gui2Caption* hint = new Gui2Caption(windowManager, "visualoptions_hint", 3, 78, 78, 2.4f,
                                      "Changes are visible immediately. ESC returns to the pause menu.");
  frame->AddView(hint);
  hint->Show();

  kitSelectionPulldown[0]->SetFocus();
  this->Show();
}

VisualOptionsPage::~VisualOptionsPage() {}

void VisualOptionsPage::OnRandomizeSun() {
  GetGameTask()->GetMatch()->SetRandomSunParams();
}

void VisualOptionsPage::OnChangeKit(Gui2Pulldown* pulldown) {
  int teamID = atoi(pulldown->GetSelected().substr(4, 1).c_str()) - 1;
  int kitNumber = atoi(pulldown->GetSelected().substr(8, 2).c_str());
  GetGameTask()->GetMatch()->GetTeam(teamID)->SetKitNumber(kitNumber);
}
