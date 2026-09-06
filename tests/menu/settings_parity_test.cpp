#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include "hid/ihidevice.hpp"
#include "presentation/ui/rmlui/frontend_runtime_bridge.hpp"
#include "presentation/ui/rmlui/input_settings.hpp"
#include "presentation/ui/rmlui/runtime_settings.hpp"
#include "presentation/ui/rmlui/screen_router.hpp"

namespace {

using blunted::ui::ParseKeyboardBindingAction;
using blunted::ui::RuntimeVolumeToLegacyAudio;
using blunted::ui::ScreenId;
using blunted::ui::ScreenRouter;
using blunted::ui::kKeyboardBindingLabels;
namespace frontend = blunted::ui::frontend;

std::string ReadTextFile(const std::string& path) {
  std::ifstream file(path);
  std::ostringstream contents;
  contents << file.rdbuf();
  return contents.str();
}

TEST(ModernSettingsParity, KeyboardModelMatchesLegacyFunctionCount) {
  static_assert(e_ButtonFunction_Size == 18, "legacy input function count changed");
  EXPECT_EQ(kKeyboardBindingLabels.size(), static_cast<std::size_t>(e_ButtonFunction_Size));
  EXPECT_EQ(kKeyboardBindingLabels[0], "MOVE UP");
  EXPECT_EQ(kKeyboardBindingLabels[4], "THROUGH PASS");
  EXPECT_EQ(kKeyboardBindingLabels[17], "START");
}

TEST(ModernSettingsParity, KeyboardBindingActionsAreStrictlyParsed) {
  ASSERT_EQ(ParseKeyboardBindingAction("bind-key-0"), 0u);
  ASSERT_EQ(ParseKeyboardBindingAction("bind-key-9"), 9u);
  ASSERT_EQ(ParseKeyboardBindingAction("bind-key-17"), 17u);
  EXPECT_FALSE(ParseKeyboardBindingAction("bind-key-18"));
  EXPECT_FALSE(ParseKeyboardBindingAction("bind-key--1"));
  EXPECT_FALSE(ParseKeyboardBindingAction("bind-key-x"));
  EXPECT_FALSE(ParseKeyboardBindingAction("reset-keyboard-bindings"));
}

TEST(ModernSettingsParity, ControlsSettingsHasDedicatedModernRoute) {
  const auto* route = ScreenRouter::FindRoute("controls-settings");
  ASSERT_NE(route, nullptr);
  EXPECT_EQ(route->id, ScreenId::ControlsSettings);
  EXPECT_EQ(route->documentPath, "media/ui/fotbiler/controls_settings.rml");
}

TEST(ModernSettingsParity, RuntimeVolumeMapsToLegacyAudioRange) {
  EXPECT_FLOAT_EQ(RuntimeVolumeToLegacyAudio(-20), 0.0f);
  EXPECT_FLOAT_EQ(RuntimeVolumeToLegacyAudio(0), 0.0f);
  EXPECT_FLOAT_EQ(RuntimeVolumeToLegacyAudio(80), 0.8f);
  EXPECT_FLOAT_EQ(RuntimeVolumeToLegacyAudio(100), 1.0f);
  EXPECT_FLOAT_EQ(RuntimeVolumeToLegacyAudio(140), 1.0f);
}

TEST(ModernSettingsParity, DisplayBridgePreservesSettingsRequest) {
  frontend::Reset();
  frontend::DisplaySettingsRequest request;
  request.width = 2560;
  request.height = 1440;
  request.fullscreen = false;
  request.vsync = false;
  request.difficultyStep = 2;
  request.gameSpeedStep = 0;
  request.volume = 60;
  frontend::PublishDisplaySettings(request);

  frontend::DisplaySettingsRequest consumed;
  ASSERT_TRUE(frontend::ConsumeDisplaySettings(consumed));
  EXPECT_EQ(consumed.width, 2560);
  EXPECT_EQ(consumed.height, 1440);
  EXPECT_FALSE(consumed.fullscreen);
  EXPECT_FALSE(consumed.vsync);
  EXPECT_EQ(consumed.difficultyStep, 2);
  EXPECT_EQ(consumed.gameSpeedStep, 0);
  EXPECT_EQ(consumed.volume, 60);
  EXPECT_FALSE(frontend::ConsumeDisplaySettings(consumed));
  frontend::Reset();
}

TEST(ModernSettingsParity, SettingsScreenDoesNotExposeDeadMatchOnlyLinks) {
  const std::string settings = ReadTextFile("data/media/ui/fotbiler/runtime_settings.rml");
  ASSERT_FALSE(settings.empty());
  EXPECT_NE(settings.find("data-route=\"controls-settings\""), std::string::npos);
  EXPECT_NE(settings.find("data-action=\"cycle-language\""), std::string::npos);
  EXPECT_EQ(settings.find("data-route=\"camera-settings\""), std::string::npos);
  EXPECT_EQ(settings.find("data-route=\"visual-settings\""), std::string::npos);
  EXPECT_EQ(settings.find("data-route=\"controller-select-modern\""), std::string::npos);

  const std::string controls = ReadTextFile("data/media/ui/fotbiler/controls_settings.rml");
  ASSERT_FALSE(controls.empty());
  for (std::size_t i = 0; i < kKeyboardBindingLabels.size(); ++i) {
    EXPECT_NE(controls.find("data-action=\"bind-key-" + std::to_string(i) + "\""),
              std::string::npos)
        << "missing keyboard binding action " << i;
  }
}

}  // namespace
