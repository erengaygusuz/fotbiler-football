#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

#include "hid/ihidevice.hpp"
#include "presentation/ui/rmlui/frontend_runtime_bridge.hpp"
#include "presentation/ui/rmlui/input_settings.hpp"
#include "presentation/ui/rmlui/runtime_settings.hpp"
#include "presentation/ui/rmlui/screen_router.hpp"
#include "utils/localization.hpp"

namespace {

using blunted::ui::DescribePhysicalGamepadBinding;
using blunted::ui::EncodeGamepadAxisDirection;
using blunted::ui::ParseGamepadFunctionAction;
using blunted::ui::ParseGamepadPhysicalBindingAction;
using blunted::ui::ParseGamepadSelectAction;
using blunted::ui::ParseKeyboardBindingAction;
using blunted::ui::RuntimeVolumeToLegacyAudio;
using blunted::ui::ScreenId;
using blunted::ui::ScreenRouter;
using blunted::ui::kControllerButtonLabels;
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

TEST(ModernSettingsParity, GamepadModelMatchesLegacyControllerButtonCount) {
  static_assert(e_ControllerButton_Size == 14, "legacy controller button count changed");
  EXPECT_EQ(kControllerButtonLabels.size(), static_cast<std::size_t>(e_ControllerButton_Size));
  EXPECT_EQ(kControllerButtonLabels[0], "DPAD UP");
  EXPECT_EQ(kControllerButtonLabels[6], "A / CROSS");
  EXPECT_EQ(kControllerButtonLabels[13], "START / OPTIONS");
}

TEST(ModernSettingsParity, GamepadActionsAreStrictlyParsed) {
  EXPECT_EQ(ParseGamepadSelectAction("select-gamepad-0"), 0u);
  EXPECT_EQ(ParseGamepadSelectAction("select-gamepad-7"), 7u);
  EXPECT_FALSE(ParseGamepadSelectAction("select-gamepad-8"));

  EXPECT_EQ(ParseGamepadPhysicalBindingAction("bind-gamepad-physical-0"), 0u);
  EXPECT_EQ(ParseGamepadPhysicalBindingAction("bind-gamepad-physical-13"), 13u);
  EXPECT_FALSE(ParseGamepadPhysicalBindingAction("bind-gamepad-physical-14"));

  EXPECT_EQ(ParseGamepadFunctionAction("cycle-gamepad-function-0"), 0u);
  EXPECT_EQ(ParseGamepadFunctionAction("cycle-gamepad-function-17"), 17u);
  EXPECT_FALSE(ParseGamepadFunctionAction("cycle-gamepad-function-18"));
}

TEST(ModernSettingsParity, LegacyAxisBindingEncodingRoundTripsToReadableLabels) {
  EXPECT_EQ(EncodeGamepadAxisDirection(0, false), -1);
  EXPECT_EQ(EncodeGamepadAxisDirection(0, true), -2);
  EXPECT_EQ(EncodeGamepadAxisDirection(1, false), -3);
  EXPECT_EQ(EncodeGamepadAxisDirection(1, true), -4);
  EXPECT_EQ(DescribePhysicalGamepadBinding(-1), "AXIS 0-");
  EXPECT_EQ(DescribePhysicalGamepadBinding(-2), "AXIS 0+");
  EXPECT_EQ(DescribePhysicalGamepadBinding(-3), "AXIS 1-");
  EXPECT_EQ(DescribePhysicalGamepadBinding(5), "BUTTON 5");
}

TEST(ModernSettingsParity, GamepadWorkflowHasDedicatedModernRoutes) {
  const struct {
    const char* name;
    ScreenId id;
    const char* document;
  } routes[] = {
      {"gamepad-list", ScreenId::GamepadList, "media/ui/fotbiler/gamepad_list.rml"},
      {"gamepad-setup", ScreenId::GamepadSetup, "media/ui/fotbiler/gamepad_setup.rml"},
      {"gamepad-calibration", ScreenId::GamepadCalibration,
       "media/ui/fotbiler/gamepad_calibration.rml"},
      {"gamepad-mapping", ScreenId::GamepadMapping, "media/ui/fotbiler/gamepad_mapping.rml"},
      {"gamepad-functions", ScreenId::GamepadFunctions,
       "media/ui/fotbiler/gamepad_functions.rml"},
  };

  for (const auto& expected : routes) {
    const auto* route = ScreenRouter::FindRoute(expected.name);
    ASSERT_NE(route, nullptr) << expected.name;
    EXPECT_EQ(route->id, expected.id);
    EXPECT_EQ(route->documentPath, expected.document);
  }
}

TEST(ModernSettingsParity, GamepadDocumentsExposeLegacyParityActions) {
  const std::string controls = ReadTextFile("data/media/ui/fotbiler/controls_settings.rml");
  const std::string list = ReadTextFile("data/media/ui/fotbiler/gamepad_list.rml");
  const std::string setup = ReadTextFile("data/media/ui/fotbiler/gamepad_setup.rml");
  const std::string mapping = ReadTextFile("data/media/ui/fotbiler/gamepad_mapping.rml");
  const std::string functions = ReadTextFile("data/media/ui/fotbiler/gamepad_functions.rml");
  const std::string calibration = ReadTextFile("data/media/ui/fotbiler/gamepad_calibration.rml");

  EXPECT_NE(controls.find("data-route=\"gamepad-list\""), std::string::npos);
  EXPECT_NE(setup.find("data-action=\"begin-gamepad-calibration\""), std::string::npos);
  EXPECT_NE(setup.find("data-route=\"gamepad-mapping\""), std::string::npos);
  EXPECT_NE(setup.find("data-route=\"gamepad-functions\""), std::string::npos);
  EXPECT_NE(calibration.find("data-action=\"save-gamepad-calibration\""), std::string::npos);
  EXPECT_NE(calibration.find("data-action=\"cancel-gamepad-calibration\""), std::string::npos);

  for (std::size_t i = 0; i < 8; ++i) {
    EXPECT_NE(list.find("data-action=\"select-gamepad-" + std::to_string(i) + "\""),
              std::string::npos);
  }
  for (std::size_t i = 0; i < kControllerButtonLabels.size(); ++i) {
    EXPECT_NE(mapping.find("data-action=\"bind-gamepad-physical-" + std::to_string(i) + "\""),
              std::string::npos);
  }
  for (std::size_t i = 0; i < kKeyboardBindingLabels.size(); ++i) {
    EXPECT_NE(functions.find("data-action=\"cycle-gamepad-function-" + std::to_string(i) + "\""),
              std::string::npos);
  }
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

TEST(ModernSettingsParity, MigratedScreensExposeLocalizationTargets) {
  const std::string settings = ReadTextFile("data/media/ui/fotbiler/runtime_settings.rml");
  const std::string mainMenu = ReadTextFile("data/media/ui/fotbiler/main_menu.rml");
  const std::string controls = ReadTextFile("data/media/ui/fotbiler/controls_settings.rml");
  const std::string credits = ReadTextFile("data/media/ui/fotbiler/credits.rml");
  const std::string league = ReadTextFile("data/media/ui/fotbiler/league_start.rml");

  EXPECT_NE(settings.find("id=\"settings-title\""), std::string::npos);
  EXPECT_NE(settings.find("id=\"settings-language-label\""), std::string::npos);
  EXPECT_NE(mainMenu.find("id=\"main-career-title\""), std::string::npos);
  EXPECT_NE(mainMenu.find("id=\"main-settings-title\""), std::string::npos);
  EXPECT_NE(controls.find("id=\"controls-title\""), std::string::npos);
  EXPECT_NE(credits.find("id=\"credits-back-title\""), std::string::npos);
  EXPECT_NE(league.find("id=\"league-back-title\""), std::string::npos);
}

TEST(ModernSettingsParity, ShippedLocalesProduceVisibleSettingsTranslations) {
  Localization& localization = Localization::GetInstance();
  ASSERT_TRUE(localization.Load("en"));
  const std::string englishSettings = localization.Translate("settings_title");
  const std::string englishControls = localization.Translate("settings_controller");

  ASSERT_TRUE(localization.Load("de"));
  EXPECT_EQ(localization.Translate("settings_title"), "Einstellungen");
  EXPECT_EQ(localization.Translate("settings_controller"), "Steuerung");
  EXPECT_NE(localization.Translate("settings_title"), englishSettings);
  EXPECT_NE(localization.Translate("settings_controller"), englishControls);

  ASSERT_TRUE(localization.Load("en"));
}

TEST(ModernSettingsParity, TurkishLocaleIsShippedAndVisibleInModernUi) {
  const auto languages = Localization::GetAvailableLanguages();
  ASSERT_GE(languages.size(), 2u);
  EXPECT_EQ(languages[0], "en");
  EXPECT_EQ(languages[1], "tr");
  EXPECT_NE(std::find(languages.begin(), languages.end(), "tr"), languages.end());
  EXPECT_EQ(Localization::GetLanguageDisplayName("tr"), "Türkçe");

  Localization& localization = Localization::GetInstance();
  ASSERT_TRUE(localization.Load("tr"));
  EXPECT_EQ(localization.GetCurrentLanguage(), "tr");
  EXPECT_EQ(localization.Translate("settings_title"), "Ayarlar");
  EXPECT_EQ(localization.Translate("menu_careermode"), "Kariyer Modu");
  EXPECT_EQ(localization.Translate("menu_leaguemode"), "Lig Modu");
  EXPECT_EQ(localization.Translate("graphics_resolution"), "Çözünürlük");
  EXPECT_EQ(localization.Translate("settings_controller"), "Kontroller");
  EXPECT_EQ(localization.Translate("league_return_main_menu"), "Ana Menüye Dön");

  ASSERT_TRUE(localization.Load("en"));
}

TEST(ModernSettingsParity, SingleUtilityNavigationTilesStayInsideUtilityRows) {
  const std::string credits = ReadTextFile("data/media/ui/fotbiler/credits.rml");
  const std::string league = ReadTextFile("data/media/ui/fotbiler/league_start.rml");
  ASSERT_FALSE(credits.empty());
  ASSERT_FALSE(league.empty());

  EXPECT_NE(credits.find("class=\"row utility-row\""), std::string::npos);
  EXPECT_NE(league.find("class=\"row utility-row\""), std::string::npos);
  EXPECT_NE(credits.find("id=\"credits-back-title\""), std::string::npos);
  EXPECT_NE(league.find("id=\"league-back-title\""), std::string::npos);
}

}  // namespace
