#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "presentation/ui/rmlui/frontend_runtime_bridge.hpp"
#include "presentation/ui/rmlui/screen_router.hpp"

namespace {

using blunted::ui::ScreenId;
using blunted::ui::ScreenRouter;
namespace frontend = blunted::ui::frontend;

TEST(ModernMainMenuParity, ExposesLegacyPlayerFacingEntryRoutes) {
  const auto* quickMatch = ScreenRouter::FindRoute("match-setup");
  const auto* league = ScreenRouter::FindRoute("league-start");
  const auto* career = ScreenRouter::FindRoute("career-mode-select");
  const auto* settings = ScreenRouter::FindRoute("runtime-settings");
  const auto* credits = ScreenRouter::FindRoute("credits");
  const auto* mainMenu = ScreenRouter::FindRoute("main-menu");

  ASSERT_NE(quickMatch, nullptr);
  ASSERT_NE(league, nullptr);
  ASSERT_NE(career, nullptr);
  ASSERT_NE(settings, nullptr);
  ASSERT_NE(credits, nullptr);
  ASSERT_NE(mainMenu, nullptr);

  EXPECT_EQ(quickMatch->id, ScreenId::MatchSetup);
  EXPECT_EQ(league->id, ScreenId::LeagueStart);
  EXPECT_EQ(career->id, ScreenId::CareerModeSelect);
  EXPECT_EQ(settings->id, ScreenId::RuntimeSettings);
  EXPECT_EQ(credits->id, ScreenId::Credits);
  EXPECT_EQ(mainMenu->id, ScreenId::MainMenu);
}

TEST(ModernMainMenuParity, LeagueAndCreditsUseModernDocumentsAndNavigationHistory) {
  std::vector<std::string> loadedDocuments;
  ScreenRouter router([&loadedDocuments](const std::string& path) {
    loadedDocuments.push_back(path);
    return true;
  });

  ASSERT_TRUE(router.Navigate(ScreenId::MainMenu));
  ASSERT_TRUE(router.NavigateByName("league-start"));
  ASSERT_EQ(router.Current(), ScreenId::LeagueStart);
  ASSERT_TRUE(router.CanGoBack());
  ASSERT_EQ(loadedDocuments.back(), "media/ui/fotbiler/league_start.rml");

  ASSERT_TRUE(router.Back());
  ASSERT_EQ(router.Current(), ScreenId::MainMenu);

  ASSERT_TRUE(router.NavigateByName("credits"));
  ASSERT_EQ(router.Current(), ScreenId::Credits);
  ASSERT_EQ(loadedDocuments.back(), "media/ui/fotbiler/credits.rml");
}

TEST(ModernMainMenuParity, QuickMatchPublishesPlayableRuntimeLaunch) {
  frontend::Reset();

  frontend::PublishQuickMatchLaunch(3, 8, 12, 0.5f, 1);

  EXPECT_EQ(frontend::GetAppMode(), frontend::AppMode::Loading);
  EXPECT_EQ(frontend::GetSessionKind(), frontend::SessionKind::QuickMatch);
  EXPECT_EQ(frontend::GetReturnTarget(), frontend::ReturnTarget::MatchSetup);

  frontend::LaunchRequest request;
  ASSERT_TRUE(frontend::ConsumeLaunchRequest(request));
  EXPECT_EQ(request.kind, frontend::LaunchKind::QuickMatch);
  EXPECT_EQ(request.homeTeamId, 3);
  EXPECT_EQ(request.awayTeamId, 8);
  EXPECT_EQ(request.matchDurationMinutes, 12);
  EXPECT_FLOAT_EQ(request.difficulty, 0.5f);
  EXPECT_EQ(request.controlSide, 1);
  EXPECT_FALSE(frontend::ConsumeLaunchRequest(request));

  frontend::Reset();
}

TEST(ModernMainMenuParity, QuitRequestIsOneShot) {
  frontend::Reset();

  frontend::RequestQuit();

  EXPECT_TRUE(frontend::ConsumeQuitRequest());
  EXPECT_FALSE(frontend::ConsumeQuitRequest());

  frontend::Reset();
}

}  // namespace
