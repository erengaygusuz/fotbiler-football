#include <gtest/gtest.h>

#include "presentation/ui/rmlui/runtime_ui_bridge.hpp"

namespace {

using blunted::ui::runtime::Command;
using blunted::ui::runtime::PublishReplaySnapshot;
using blunted::ui::runtime::ReadReplaySnapshot;
using blunted::ui::runtime::ReplaySnapshot;
using blunted::ui::runtime::Reset;
using blunted::ui::runtime::Screen;
using blunted::ui::runtime::SendCommand;
using blunted::ui::runtime::SetScreen;

TEST(RuntimeReplayBridge, ReplayCommandsRoundTripThroughRuntimeBridge) {
  Reset();

  SendCommand(Command::ReplayTogglePlayback);
  EXPECT_EQ(blunted::ui::runtime::ConsumeCommand(), Command::ReplayTogglePlayback);
  EXPECT_EQ(blunted::ui::runtime::ConsumeCommand(), Command::None);

  SendCommand(Command::ReplaySeekBackward);
  EXPECT_EQ(blunted::ui::runtime::ConsumeCommand(), Command::ReplaySeekBackward);

  SendCommand(Command::ReplayCycleCamera);
  EXPECT_EQ(blunted::ui::runtime::ConsumeCommand(), Command::ReplayCycleCamera);

  SendCommand(Command::ReplayExit);
  EXPECT_EQ(blunted::ui::runtime::ConsumeCommand(), Command::ReplayExit);
}

TEST(RuntimeReplayBridge, ReplaySnapshotPublishesLivePlaybackState) {
  Reset();
  SetScreen(Screen::Replay);

  ReplaySnapshot snapshot;
  snapshot.active = true;
  snapshot.playing = true;
  snapshot.speed = 0.5f;
  snapshot.camera = 3;
  snapshot.cameraCount = 4;
  snapshot.elapsed_ms = 4200;
  snapshot.duration_ms = 10000;
  snapshot.secondsAgo = 5;
  snapshot.progressPercent = 42;
  PublishReplaySnapshot(snapshot);

  const ReplaySnapshot actual = ReadReplaySnapshot();
  EXPECT_TRUE(actual.active);
  EXPECT_TRUE(actual.playing);
  EXPECT_FLOAT_EQ(actual.speed, 0.5f);
  EXPECT_EQ(actual.camera, 3);
  EXPECT_EQ(actual.cameraCount, 4);
  EXPECT_EQ(actual.elapsed_ms, 4200UL);
  EXPECT_EQ(actual.duration_ms, 10000UL);
  EXPECT_EQ(actual.secondsAgo, 5UL);
  EXPECT_EQ(actual.progressPercent, 42);
}

TEST(RuntimeReplayBridge, ResetClearsReplayStateAndPendingCommand) {
  ReplaySnapshot snapshot;
  snapshot.active = true;
  snapshot.playing = true;
  snapshot.progressPercent = 80;
  PublishReplaySnapshot(snapshot);
  SetScreen(Screen::Replay);
  SendCommand(Command::ReplayCycleSpeed);

  Reset();

  EXPECT_EQ(blunted::ui::runtime::GetScreen(), Screen::None);
  EXPECT_EQ(blunted::ui::runtime::ConsumeCommand(), Command::None);
  const ReplaySnapshot actual = ReadReplaySnapshot();
  EXPECT_FALSE(actual.active);
  EXPECT_FALSE(actual.playing);
  EXPECT_EQ(actual.progressPercent, 0);
}

}  // namespace
