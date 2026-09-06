#include "cameramenu.hpp"

#include "../main.hpp"
#include "utils/localization.hpp"

using namespace blunted;

CameraPage::CameraPage(Gui2WindowManager* windowManager, const Gui2PageData& pageData)
    : Gui2Page(windowManager, pageData) {
  Gui2Frame* frame = new Gui2Frame(windowManager, "camframe", 8, 6, 84, 88, true);
  this->AddView(frame);
  frame->Show();

  Gui2Caption* kicker =
      new Gui2Caption(windowManager, "camera_kicker", 3, 2, 78, 2.3f, "MATCH PRESENTATION");
  kicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  frame->AddView(kicker);
  kicker->Show();

  Gui2Caption* title =
      new Gui2Caption(windowManager, "caption_camera", 3, 5, 78, 4, "CAMERA SETTINGS");
  frame->AddView(title);
  title->Show();

  Gui2Caption* subtitle = new Gui2Caption(windowManager, "camera_subtitle", 3, 9, 78, 2.5f,
                                          "Tune match framing and apply changes live.");
  frame->AddView(subtitle);
  subtitle->Show();

  Gui2Frame* controlsPanel = new Gui2Frame(windowManager, "camera_controls", 3, 15, 49, 61, true);
  frame->AddView(controlsPanel);
  controlsPanel->Show();

  sliderZoom = new Gui2Slider(windowManager, "camzoomslider", 0, 0, 42, 6, TR("camera_zoom"));
  sliderHeight = new Gui2Slider(windowManager, "camheightslider", 0, 0, 42, 6, TR("camera_height"));
  sliderFOV = new Gui2Slider(windowManager, "camfovslider", 0, 0, 42, 6, TR("camera_fov"));
  sliderAngleFactor =
      new Gui2Slider(windowManager, "camangleslider", 0, 0, 42, 6, TR("camera_angle"));

  sliderZoom->AddHelperValue(Vector3(80, 80, 250), TR("settings_factory_default"), _default_CameraZoom);
  sliderHeight->AddHelperValue(Vector3(80, 80, 250), TR("settings_factory_default"), _default_CameraHeight);
  sliderFOV->AddHelperValue(Vector3(80, 80, 250), TR("settings_factory_default"), _default_CameraFOV);
  sliderAngleFactor->AddHelperValue(Vector3(80, 80, 250), TR("settings_factory_default"), _default_CameraAngleFactor);

  Gui2Caption* controlsTitle =
      new Gui2Caption(windowManager, "camera_controls_title", 2, 2, 45, 3, "LIVE CAMERA");
  controlsPanel->AddView(controlsTitle);
  controlsTitle->Show();

  Gui2Grid* grid = new Gui2Grid(windowManager, "camgrid", 2, 8, 45, 40);
  grid->AddView(sliderZoom, 0, 0);
  grid->AddView(sliderHeight, 1, 0);
  grid->AddView(sliderFOV, 2, 0);
  grid->AddView(sliderAngleFactor, 3, 0);
  grid->UpdateLayout(0.6f);
  controlsPanel->AddView(grid);
  grid->Show();

  sliderZoom->sig_OnChange.connect([this](...) { UpdateCamera(); });
  sliderHeight->sig_OnChange.connect([this](...) { UpdateCamera(); });
  sliderFOV->sig_OnChange.connect([this](...) { UpdateCamera(); });
  sliderAngleFactor->sig_OnChange.connect([this](...) { UpdateCamera(); });
  this->sig_OnClose.connect([this](...) { OnClose(); });

  Gui2Frame* presetsPanel = new Gui2Frame(windowManager, "camera_presets", 54, 15, 27, 61, true);
  frame->AddView(presetsPanel);
  presetsPanel->Show();

  Gui2Caption* presetKicker =
      new Gui2Caption(windowManager, "camera_preset_kicker", 2, 2, 23, 2.3f, "PRESETS");
  presetKicker->SetColor(windowManager->GetStyle()->GetColor(e_DecorationType_Bright2));
  presetsPanel->AddView(presetKicker);
  presetKicker->Show();

  Gui2Caption* presetTitle =
      new Gui2Caption(windowManager, "camera_preset_title", 2, 6, 23, 3, "BROADCAST STYLE");
  presetsPanel->AddView(presetTitle);
  presetTitle->Show();

  Gui2Button* buttonPresetStandard =
      new Gui2Button(windowManager, "cam_preset_standard", 0, 0, 23, 4, TR("camera_preset_standard"));
  Gui2Button* buttonPresetWidescreen =
      new Gui2Button(windowManager, "cam_preset_widescreen", 0, 0, 23, 4, TR("camera_preset_widescreen"));
  Gui2Button* buttonPresetUltrawide =
      new Gui2Button(windowManager, "cam_preset_ultrawide", 0, 0, 23, 4, TR("camera_preset_ultrawide"));
  Gui2Button* backButton = new Gui2Button(windowManager, "cam_button_back", 0, 0, 23, 4,
                                          Localization::GetInstance().Translate("action_back"));

  buttonPresetStandard->sig_OnClick.connect([this](...) { ApplyPreset(0.5f, 0.3f, 0.4f, 0.0f); });
  buttonPresetWidescreen->sig_OnClick.connect([this](...) { ApplyPreset(0.6f, 0.2f, 0.5f, 0.1f); });
  buttonPresetUltrawide->sig_OnClick.connect([this](...) { ApplyPreset(0.7f, 0.15f, 0.6f, 0.2f); });
  backButton->sig_OnClick.connect([this](...) { GoBack(); });

  Gui2Grid* presetGrid = new Gui2Grid(windowManager, "cam_presetgrid", 2, 12, 23, 34);
  presetGrid->AddView(buttonPresetStandard, 0, 0);
  presetGrid->AddView(buttonPresetWidescreen, 1, 0);
  presetGrid->AddView(buttonPresetUltrawide, 2, 0);
  presetGrid->AddView(backButton, 3, 0);
  presetGrid->UpdateLayout(0.7f);
  presetsPanel->AddView(presetGrid);
  presetGrid->Show();

  Gui2Caption* hint = new Gui2Caption(windowManager, "camera_hint", 3, 81, 78, 2.3f,
                                      "Changes are applied immediately and saved when you leave this screen.");
  frame->AddView(hint);
  hint->Show();

  float zoom, height, fov, angleFactor;
  GetGameTask()->GetMatch()->GetCameraParams(zoom, height, fov, angleFactor);
  sliderZoom->SetValue(zoom);
  sliderHeight->SetValue(height);
  sliderFOV->SetValue(fov);
  sliderAngleFactor->SetValue(angleFactor);
  sliderZoom->SetFocus();
  this->Show();
}

CameraPage::~CameraPage() {}

void CameraPage::ApplyPreset(float zoom, float height, float fov, float angleFactor) {
  sliderZoom->SetValue(zoom);
  sliderHeight->SetValue(height);
  sliderFOV->SetValue(fov);
  sliderAngleFactor->SetValue(angleFactor);
  UpdateCamera();
}

void CameraPage::OnClose() {
  if (Verbose()) printf("saving camera settings\n");
  GetConfiguration()->SaveFile(GetConfigFilename());
}

void CameraPage::UpdateCamera() {
  GetConfiguration()->Set("camera_zoom", sliderZoom->GetValue());
  GetConfiguration()->Set("camera_height", sliderHeight->GetValue());
  GetConfiguration()->Set("camera_fov", sliderFOV->GetValue());
  GetConfiguration()->Set("camera_anglefactor", sliderAngleFactor->GetValue());
  GetGameTask()->GetMatch()->SetCameraParams(sliderZoom->GetValue(), sliderHeight->GetValue(),
                                             sliderFOV->GetValue(), sliderAngleFactor->GetValue());
}
