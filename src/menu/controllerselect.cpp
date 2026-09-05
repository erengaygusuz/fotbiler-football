#include "controllerselect.hpp"

#include "../main.hpp"
#include "hid/gamepad.hpp"
#include "hid/keyboard.hpp"
#include "mainmenu.hpp"
#include "managers/usereventmanager.hpp"
#include "pagefactory.hpp"
#include "startmatch/teamselect.hpp"
#include "utils/localization.hpp"

using namespace blunted;

namespace {

constexpr float kControllerThumbnailAspectRatio = 300.0f / 200.0f;
constexpr float kControllerThumbnailHeight = 10.0f;
constexpr unsigned long kMenuSmokeAdvanceDelay_ms = 250;

bool MenuSmokeQuickMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_quick_match", false);
}

bool MenuSmokeFullMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_full_match", false);
}

bool MenuSmokeGamepadMatchEnabled() {
  return GetConfiguration()->GetBool("menu_smoke_test_gamepad_match", false);
}

bool MenuSmokeAutoQuickMatchEnabled() {
  return MenuSmokeQuickMatchEnabled() || MenuSmokeFullMatchEnabled();
}

Gui2Caption* AddControllerSelectNotice(Gui2Page* page, Gui2WindowManager* windowManager,
                                       const std::string& name, float yPercent,
                                       const std::string& caption) {
  Gui2Caption* notice = new Gui2Caption(windowManager, name, 10, yPercent, 80, 3, caption);
  page->AddView(notice);
  notice->SetPosition(50 - notice->GetTextWidthPercent() * 0.5, yPercent);
  notice->Show();
  return notice;
}

Gui2Button* AddControllerSelectBackButton(Gui2Page* page, Gui2WindowManager* windowManager,
                                          const std::string& name, float yPercent = 86.0f) {
  Gui2Button* backButton = new Gui2Button(windowManager, name, 38, yPercent, 24, 3.8f,
                                          Localization::GetInstance().Translate("action_back"));
  backButton->sig_OnClick.connect([page](...) { page->GoBack(); });
  page->AddView(backButton);
  backButton->Show();
  return backButton;
}

}  // namespace

ControllerSelectPage::ControllerSelectPage(Gui2WindowManager* windowManager,
                                           const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData),
      pageCreatedTime_ms(EnvironmentManager::GetInstance().GetTime_ms()),
      autoAdvanceTriggered(false) {
  inGame = pageData.properties->GetBool("isInGame");

  Gui2Frame* ctrlFrame =
      new Gui2Frame(windowManager, "frame_controllerselect", 6, 5, 88, 88, true);
  this->AddView(ctrlFrame);
  ctrlFrame->Show();

  Gui2Caption* kicker = new Gui2Caption(windowManager, "caption_controllerselect_kicker", 3, 2, 82,
                                        2.3f, inGame ? "MATCHDAY  ·  CONTROLS" : "KICK OFF  ·  CONTROLS");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  ctrlFrame->AddView(kicker);
  kicker->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_controllerselect_title", 3, 5, 82,
                                       4, "SELECT SIDES");
  ctrlFrame->AddView(title);
  title->Show();

  Gui2Caption* subtitle = new Gui2Caption(windowManager, "caption_controllerselect_subtitle", 3, 9,
                                          82, 2.4f,
                                          "Move each input device between Home, Neutral and Away.");
  ctrlFrame->AddView(subtitle);
  subtitle->Show();

  Gui2Frame* sidesPanel = new Gui2Frame(windowManager, "frame_controllerselect_sides", 3, 15, 82, 58,
                                        true);
  ctrlFrame->AddView(sidesPanel);
  sidesPanel->Show();

  Gui2Caption* t1 =
      new Gui2Caption(windowManager, "caption_controllerselect_t1", 2, 3, 24, 3, "HOME");
  Gui2Caption* tNeutral =
      new Gui2Caption(windowManager, "caption_controllerselect_tn", 29, 3, 24, 3, "NEUTRAL");
  Gui2Caption* t2 =
      new Gui2Caption(windowManager, "caption_controllerselect_t2", 56, 3, 24, 3, "AWAY");
  t1->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  t2->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  tNeutral->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Dark2));
  sidesPanel->AddView(t1);
  sidesPanel->AddView(tNeutral);
  sidesPanel->AddView(t2);
  t1->Show();
  tNeutral->Show();
  t2->Show();

  this->SetFocus();

  const std::vector<IHIDevice*>& controllers = GetControllers();
  if (controllers.empty()) {
    AddControllerSelectNotice(this, windowManager, "caption_controllerselect_noinput", 44,
                              TR("ctrl_noinput"));
    AddControllerSelectNotice(this, windowManager, "caption_controllerselect_noinput_hint", 49,
                              TR("ctrl_noinput_hint"));
    Gui2Button* backButton =
        AddControllerSelectBackButton(this, windowManager, "button_controllerselect_back");
    backButton->SetFocus();
    this->Show();
    return;
  }

  if (inGame) sides = GetMenuTask()->GetControllerSetup();
  for (unsigned int i = 0; i < controllers.size(); i++) {
    SideSelection side;
    side.controllerID = i;
    if (inGame && i < sides.size()) {
      side.side = sides.at(i).side;
    } else {
      side.side = 0;
      if (!autoAssignedPlayerOne && controllers.at(i)->GetDeviceType() == e_HIDeviceType_Gamepad) {
        side.side = -1;
        autoAssignedPlayerOne = true;
      }
    }
    const float controllerImageWidth = windowManager->GetWidthPercentForHeight(
        kControllerThumbnailHeight, kControllerThumbnailAspectRatio);
    side.controllerImage = new Gui2Image(windowManager, "image_controller" + int_to_str(i), 0, 0,
                                         controllerImageWidth, kControllerThumbnailHeight);
    this->AddView(side.controllerImage);
    if (controllers.at(i)->GetDeviceType() == e_HIDeviceType_Gamepad) {
      side.controllerImage->LoadImage("media/menu/controller/controller_small.png");
    } else {
      side.controllerImage->LoadImage("media/menu/controller/keyboard_small.png");
    }
    side.controllerImage->Show();
    if (!inGame || i >= sides.size()) sides.push_back(side);
    else sides.at(i) = side;
    delay.push_back(0);
  }

  if (sides.size() > controllers.size()) sides.resize(controllers.size());
  SetImagePositions();

  Gui2Caption* hintGamepad = new Gui2Caption(
      windowManager, "caption_ctrl_hint_gp", 3, 77.0f, 80, 2.0f, TR("ctrl_hint_gamepad"));
  hintGamepad->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  ctrlFrame->AddView(hintGamepad);
  hintGamepad->Show();

  Gui2Caption* hintKbd = new Gui2Caption(
      windowManager, "caption_ctrl_hint_kbd", 3, 80.0f, 80, 2.0f, TR("ctrl_hint_keyboard"));
  ctrlFrame->AddView(hintKbd);
  hintKbd->Show();

  Gui2Button* backButton =
      AddControllerSelectBackButton(this, windowManager, "button_controllerselect_back", 90.0f);
  (void)backButton;
  this->Show();
}

ControllerSelectPage::~ControllerSelectPage() {}

void ControllerSelectPage::ConfirmSelection() {
  if (!inGame) {
    if (sides.empty()) {
      printf("[menu-smoke] Controller select has no available input devices\n");
      return;
    }

    if (MenuSmokeFullMatchEnabled()) {
      if (MenuSmokeGamepadMatchEnabled()) {
        const std::vector<IHIDevice*>& controllers = GetControllers();
        for (auto& side : sides) {
          bool scripted = side.controllerID >= 0 &&
                          side.controllerID < static_cast<int>(controllers.size()) &&
                          controllers.at(side.controllerID)->IsScriptedInput();
          side.side = scripted ? -1 : 0;
        }
        SetImagePositions();
        printf("[menu-smoke] Controller select: scripted gamepad drives Player 1, other team CPU\n");
      } else {
        for (auto& side : sides) side.side = 0;
        SetImagePositions();
        printf("[menu-smoke] Controller select switched to CPU vs CPU for full-match verification\n");
      }
    }

    GetMenuTask()->SetControllerSetup(sides);
    printf("[menu-smoke] Controller select confirmed, opening team selection\n");
    CreatePage(e_PageID_TeamSelect);
  }
}

void ControllerSelectPage::SetImagePositions() {
  for (unsigned int i = 0; i < sides.size(); i++) {
    const float controllerImageWidth = windowManager->GetWidthPercentForHeight(
        kControllerThumbnailHeight, kControllerThumbnailAspectRatio);
    const float centerX = 50.0f + sides.at(i).side * 25.0f;
    sides.at(i).controllerImage->SetPosition(centerX - controllerImageWidth * 0.5f, 28 + i * 14);
  }
}

void ControllerSelectPage::Process() {
  Gui2Page::Process();

  if (!autoAdvanceTriggered && MenuSmokeAutoQuickMatchEnabled() && !inGame &&
      EnvironmentManager::GetInstance().GetTime_ms() >= pageCreatedTime_ms + kMenuSmokeAdvanceDelay_ms) {
    autoAdvanceTriggered = true;
    ConfirmSelection();
    return;
  }

  const std::vector<IHIDevice*>& controllers = GetControllers();
  if (controllers.empty() || sides.empty() || delay.empty()) return;
  unsigned long now_ms = EnvironmentManager::GetInstance().GetTime_ms();
  for (unsigned int i = 0; i < controllers.size() && i < sides.size() && i < delay.size(); i++) {
    if (delay.at(i) >= now_ms - 250) continue;

    IHIDevice* controller = controllers.at(i);
    bool moved = false;
    if (controller->GetDeviceType() == e_HIDeviceType_Keyboard) {
      HIDKeyboard* keyboard = static_cast<HIDKeyboard*>(controller);
      if (keyboard->GetButtonValue(e_ButtonFunction_Left) > 0.5) {
        sides.at(i).side -= 1;
        moved = true;
      }
      if (keyboard->GetButtonValue(e_ButtonFunction_Right) > 0.5) {
        sides.at(i).side += 1;
        moved = true;
      }
      if (UserEventManager::GetInstance().GetKeyboardState(SDLK_1)) {
        sides.at(i).side = -1;
        moved = true;
      } else if (UserEventManager::GetInstance().GetKeyboardState(SDLK_2)) {
        sides.at(i).side = 1;
        moved = true;
      }
    } else if (controller->GetDeviceType() == e_HIDeviceType_Gamepad) {
      HIDGamepad* gamepad = static_cast<HIDGamepad*>(controller);
      if (gamepad->GetButtonValue(e_ButtonFunction_Left) > 0.5 ||
          gamepad->GetButtonValue(e_ButtonFunction_Switch) > 0.5) {
        sides.at(i).side -= 1;
        moved = true;
      }
      if (gamepad->GetButtonValue(e_ButtonFunction_Right) > 0.5 ||
          gamepad->GetButtonValue(e_ButtonFunction_Sprint) > 0.5) {
        sides.at(i).side += 1;
        moved = true;
      }
      if (gamepad->GetButtonValue(e_ButtonFunction_ShortPass) > 0.5 ||
          gamepad->GetButtonValue(e_ButtonFunction_Start) > 0.5) {
        ConfirmSelection();
        return;
      }
      if (gamepad->GetButtonValue(e_ButtonFunction_HighPass) > 0.5 ||
          gamepad->GetButtonValue(e_ButtonFunction_Select) > 0.5) {
        GoBack();
        return;
      }
    }

    if (moved) {
      sides.at(i).side = clamp(sides.at(i).side, -1, 1);
      delay.at(i) = now_ms;
    }
  }
  SetImagePositions();
}

void ControllerSelectPage::ProcessKeyboardEvent(KeyboardEvent* event) { (void)event; }
void ControllerSelectPage::ProcessJoystickEvent(JoystickEvent* event) { (void)event; }

void ControllerSelectPage::ProcessWindowingEvent(WindowingEvent* event) {
  if (event->IsActivate()) {
    if (!inGame) {
      if (sides.empty()) {
        event->Ignore();
        return;
      }
      GetMenuTask()->SetControllerSetup(sides);
      CreatePage(e_PageID_TeamSelect);
      return;
    }
    Gui2Page::ProcessWindowingEvent(event);
  }
  if (event->IsEscape()) {
    if (inGame) {
      GetMenuTask()->SetControllerSetup(sides);
      GetGameTask()->GetMatch()->UpdateControllerSetup();
    }
    Gui2Page::ProcessWindowingEvent(event);
  }
}
