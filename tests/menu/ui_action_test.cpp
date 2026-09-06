#include <gtest/gtest.h>

#include "presentation/ui/rmlui/ui_action.hpp"

namespace blunted::ui {
namespace {

TEST(ModernUiAction, PreservesParameterlessLegacyActions) {
  const UiAction action = MakeUiAction("start-quick-match");

  EXPECT_EQ(action.name, "start-quick-match");
  EXPECT_TRUE(action.arguments.empty());
  EXPECT_TRUE(static_cast<bool>(action));
}

TEST(ModernUiAction, ParsesFeatureParityPayload) {
  const UiAction action =
      MakeUiAction("place-transfer-bid", "player-id=42;amount=12500000;strategy=Attacking");

  EXPECT_EQ(action.Argument("player-id"), "42");
  EXPECT_EQ(action.IntArgument("player-id"), 42);
  EXPECT_EQ(action.LongLongArgument("amount"), 12500000);
  EXPECT_EQ(action.Argument("strategy"), "Attacking");
}

TEST(ModernUiAction, SupportsEscapedSeparatorsAndWhitespace) {
  const UiAction action = MakeUiAction(
      "rename-stadium", R"( value = Fotbiler\; Arena ; note = A\=B\\C )");

  EXPECT_EQ(action.Argument("value"), "Fotbiler; Arena");
  EXPECT_EQ(action.Argument("note"), R"(A=B\C)");
}

TEST(ModernUiAction, IgnoresMalformedArgumentsAndRejectsInvalidNumbers) {
  const UiAction action =
      MakeUiAction("release-player", "broken;=no-key;player-id=abc;slot=3x;valid=7");

  EXPECT_FALSE(action.HasArgument("broken"));
  EXPECT_FALSE(action.HasArgument(""));
  EXPECT_FALSE(action.IntArgument("player-id").has_value());
  EXPECT_FALSE(action.IntArgument("slot").has_value());
  ASSERT_TRUE(action.IntArgument("valid").has_value());
  EXPECT_EQ(*action.IntArgument("valid"), 7);
}

TEST(ModernUiAction, LastDuplicateArgumentWins) {
  const UiAction action = MakeUiAction("set-strategy", "value=Balanced;value=Possession");
  EXPECT_EQ(action.Argument("value"), "Possession");
}

}  // namespace
}  // namespace blunted::ui
