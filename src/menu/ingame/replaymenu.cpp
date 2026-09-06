#include "replaymenu.hpp"

#include "../../hid/gamepad.hpp"
#include "../../hid/keyboard.hpp"
#include "framework/scheduler.hpp"
#include "main.hpp"
#include "managers/environmentmanager.hpp"
#include "utils/gui2/widgets/caption.hpp"
#include "utils/gui2/widgets/frame.hpp"
#include "utils/localization.hpp"

using namespace blunted;

ReplayPage::ReplayPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  match = GetGameTask()->GetMatch();
  if (!match) {
    return;
  }

  this->SetFocus();
  this->Show();

  signed long tmp = match->GetActualTime_ms() - match->GetReplaySize_ms();
  minTime_ms = std::max((signed long)10, tmp);
  signed long tmp2 = static_cast<signed long>(match->GetActualTime_ms()) - 10;
  maxTime_ms = static_cast<unsigned long>(std::max(10L, tmp2));
  actualTime_ms = clamp(maxTime_ms - 3000, minTime_ms, maxTime_ms);
  replayCamCount = match->GetReplayCamCount();

  cam = 0;
  modifierValue = 0.0f;
  autoRun = false;
  slowMotion = false;
  stayInReplay = true;
  closeWhenAutorunCompletes = false;

  // Broadcast-style top strap.
  Gui2Frame* header = new Gui2Frame(windowManager, "frame_replay_header", 5, 3, 90, 11, true);
  this->AddView(header);
  header->Show();

  Gui2Caption* kicker =
      new Gui2Caption(windowManager, "caption_replay_kicker", 2, 1, 84, 2.2f, "MATCH MEDIA  ·  REPLAY");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  header->AddView(kicker);
  kicker->Show();

  Gui2Caption* title = new Gui2Caption(windowManager, "caption_replay_title", 2, 4, 84, 4,
                                       Localization::GetInstance().Translate("ingame_replay_title"));
  header->AddView(title);
  title->Show();

  Gui2Frame* sidePanel = new Gui2Frame(windowManager, "frame_replay_controls", 72, 18, 23, 55, true);
  this->AddView(sidePanel);
  sidePanel->Show();

  Gui2Caption* controlsKicker =
      new Gui2Caption(windowManager, "caption_replay_controls_kicker", 2, 2, 19, 2.2f, "CONTROLS");
  controlsKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  sidePanel->AddView(controlsKicker);
  controlsKicker->Show();

  Gui2Caption* controlsTitle =
      new Gui2Caption(windowManager, "caption_replay_controls_title", 2, 6, 19, 3, "BROADCAST");
  sidePanel->AddView(controlsTitle);
  controlsTitle->Show();

  Gui2Caption* controls = new Gui2Caption(
      windowManager, "caption_replay_controls_copy", 2, 12, 19, 29,
      "LEFT / RIGHT\nScrub timeline\n\nUP / DOWN\nOrbit replay camera\n\nPASS\nChange camera\n\nSHOOT / HIGH PASS\nPlay / pause\n\nSPRINT\nHold for slow motion");
  sidePanel->AddView(controls);
  controls->Show();

  // Bottom timeline bar. The replay itself remains visible behind these panels.
  Gui2Frame* footer = new Gui2Frame(windowManager, "frame_replay_footer", 5, 80, 90, 16, true);
  this->AddView(footer);
  footer->Show();

  Gui2Caption* timelineKicker =
      new Gui2Caption(windowManager, "caption_replay_timeline_kicker", 2, 1, 84, 2.2f, "TIMELINE");
  timelineKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  footer->AddView(timelineKicker);
  timelineKicker->Show();

  timeLabel = new Gui2Caption(windowManager, "caption_replay_time", 2, 5, 84, 3, "");
  footer->AddView(timeLabel);
  timeLabel->Show();

  Gui2Caption* help = new Gui2Caption(windowManager, "caption_replay_help", 2, 10, 84, 2.2f,
                                      "ESC Back to match  ·  Camera and time controls stay live");
  footer->AddView(help);
  help->Show();
  UpdateTimeLabel();

  sig_OnClose.connect([this](...) { OnClose(); });
  match->SetAutoUpdateIngameCamera(false);

  match->replayState.Lock();
  match->replayState->viewTime_ms = actualTime_ms;
  match->replayState->cam = cam;
  match->replayState->modifierValue = 0.0f;
  match->replayState->dirty = true;
  match->replayState.Unlock();
}

ReplayPage::~ReplayPage() {}

void ReplayPage::OnClose() {
  match->replayState.Lock();
  match->replayState->viewTime_ms = maxTime_ms;
  match->replayState->cam = cam;
  match->replayState->modifierValue = 0.0f;
  match->replayState->dirty = true;
  match->replayState.Unlock();

  GetScheduler()->ResetTaskSequenceTime("game");
  match->SetAutoUpdateIngameCamera(true);
  if (stayInReplay) match->Pause(false);
}

void ReplayPage::Autorun(int replayHistoryOffset_ms, bool stayInReplay) {
  autoRun = true;
  closeWhenAutorunCompletes = true;
  cam = 1;
  modifierValue = 0.0;
  signed long tmp = maxTime_ms - replayHistoryOffset_ms;
  actualTime_ms = clamp(tmp, minTime_ms, maxTime_ms);
  this->stayInReplay = stayInReplay;
}

void ReplayPage::UpdateTimeLabel() {
  unsigned long replaySize_ms = maxTime_ms - minTime_ms;
  unsigned long elapsed_ms = actualTime_ms - minTime_ms;
  float positionPct = replaySize_ms > 0 ? elapsed_ms * 100.0f / replaySize_ms : 0.0f;
  unsigned long secsAgo = (maxTime_ms - actualTime_ms) / 1000;
  std::string label = std::string(slowMotion ? "SLOW MOTION  0.5x   ·   " : "PLAYBACK  1.0x   ·   ") +
                      int_to_str(elapsed_ms / 1000) + "s / " + int_to_str(replaySize_ms / 1000) +
                      "s   ·   " + int_to_str(static_cast<int>(round(positionPct))) + "%   ·   -" +
                      int_to_str(secsAgo) + "s   ·   CAMERA " + int_to_str(cam + 1);
  timeLabel->SetCaption(label);
}

void ReplayPage::Process() {
  if (autoRun) {
    Vector3 direction;
    direction.coords[0] = slowMotion ? 0.25f : 0.5f;
    ProcessInput(direction, false, false, false);
  }
}

void ReplayPage::ProcessKeyboardEvent(KeyboardEvent* event) {
  const std::vector<IHIDevice*>& controllers = GetControllers();
  HIDKeyboard* keyboard = nullptr;
  for (IHIDevice* c : controllers) {
    if (c && c->GetDeviceType() == e_HIDeviceType_Keyboard) {
      keyboard = static_cast<HIDKeyboard*>(c);
      break;
    }
  }
  if (!keyboard) return;

  bool button1 = event->GetKeyOnce(keyboard->GetFunctionMapping(e_ButtonFunction_ShortPass));
  bool button2 = event->GetKeyOnce(keyboard->GetFunctionMapping(e_ButtonFunction_HighPass));
  bool slowMo = event->GetKeyContinuous(keyboard->GetFunctionMapping(e_ButtonFunction_Sprint));

  Vector3 direction;
  if (event->GetKeyContinuous(keyboard->GetFunctionMapping(e_ButtonFunction_Left))) direction.coords[0] -= 0.5f;
  if (event->GetKeyContinuous(keyboard->GetFunctionMapping(e_ButtonFunction_Right))) direction.coords[0] += 0.5f;
  if (event->GetKeyContinuous(keyboard->GetFunctionMapping(e_ButtonFunction_Up))) direction.coords[1] -= 0.5f;
  if (event->GetKeyContinuous(keyboard->GetFunctionMapping(e_ButtonFunction_Down))) direction.coords[1] += 0.5f;
  ProcessInput(direction, button1, button2, slowMo);
}

void ReplayPage::ProcessJoystickEvent(JoystickEvent* event) {
  const std::vector<IHIDevice*>& controllers = GetControllers();
  HIDGamepad* gamepad = nullptr;
  for (IHIDevice* c : controllers) {
    if (c && c->GetDeviceType() == e_HIDeviceType_Gamepad) {
      gamepad = static_cast<HIDGamepad*>(c);
      break;
    }
  }
  if (!gamepad) return;

  bool button1 =
      event->GetButton(0, gamepad->GetControllerMapping(gamepad->GetFunctionMapping(e_ButtonFunction_LongPass))) ||
      event->GetButton(0, gamepad->GetControllerMapping(gamepad->GetFunctionMapping(e_ButtonFunction_ShortPass)));
  bool button2 =
      event->GetButton(0, gamepad->GetControllerMapping(gamepad->GetFunctionMapping(e_ButtonFunction_HighPass))) ||
      event->GetButton(0, gamepad->GetControllerMapping(gamepad->GetFunctionMapping(e_ButtonFunction_Shot)));
  bool slowMo = event->GetButton(
      0, gamepad->GetControllerMapping(gamepad->GetFunctionMapping(e_ButtonFunction_Sprint)));

  Vector3 direction;
  direction.coords[0] = event->GetAxis(0, 0);
  direction.coords[1] = event->GetAxis(0, 1);

  float deadzone = 0.2f;
  if (fabs(direction.coords[0]) < deadzone) {
    direction.coords[0] = 0.0f;
  } else {
    direction.coords[0] = pow((fabs(direction.coords[0]) - deadzone) * (1.0f / (1.0f - deadzone)), 2.0f) *
                          signSide(direction.coords[0]);
  }
  deadzone = 0.4f;
  if (fabs(direction.coords[1]) < deadzone) {
    direction.coords[1] = 0.0f;
  } else {
    direction.coords[1] = pow((fabs(direction.coords[1]) - deadzone) * (1.0f / (1.0f - deadzone)), 4.0f) *
                          signSide(direction.coords[1]);
  }
  ProcessInput(direction, button1, button2, slowMo);
}

void ReplayPage::ProcessInput(const Vector3& direction, bool button1, bool button2,
                              bool slowMoInput) {
  slowMotion = slowMoInput;

  if (button2 && autoRun == false) {
    actualTime_ms = minTime_ms;
    autoRun = true;
    closeWhenAutorunCompletes = false;
  } else if (button2) {
    autoRun = false;
    closeWhenAutorunCompletes = false;
  }
  if (button1 && autoRun == true) {
    autoRun = false;
    closeWhenAutorunCompletes = false;
  } else if (button1) {
    cam++;
    if (cam == replayCamCount) cam = 0;
  }

  if (!autoRun) modifierValue += direction.coords[1] * 0.05f;

  if (cam == 2) {
    if (modifierValue < -1.0f) modifierValue += 2.0f;
    if (modifierValue > 1.0f) modifierValue -= 2.0f;
  } else {
    modifierValue = clamp(modifierValue, -1.0f, 1.0f);
  }

  float speedMultiplier = slowMotion ? 0.5f : 1.0f;
  float timeMovement = direction.coords[0] * 2.0f * speedMultiplier;
  actualTime_ms += int(round(timeMovement * 10.0f));

  if (autoRun && actualTime_ms >= (signed int)maxTime_ms) {
    autoRun = false;
    if (closeWhenAutorunCompletes) {
      closeWhenAutorunCompletes = false;
      GoBack();
      return;
    }
  }

  actualTime_ms = clamp(actualTime_ms, minTime_ms, maxTime_ms);
  UpdateTimeLabel();

  match->replayState.Lock();
  match->replayState->viewTime_ms = actualTime_ms;
  match->replayState->cam = cam;
  match->replayState->modifierValue = modifierValue;
  match->replayState->dirty = true;
  match->replayState.Unlock();
}
