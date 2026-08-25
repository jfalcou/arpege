// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/action_map.hpp>
#include <core/action_state.hpp>
#include <core/screen.hpp>
#include <data/player_data.hpp>
#include <data/script_host.hpp>
#include <world/run_state.hpp>

namespace arpg
{

/// Between assignments.
///
/// Owns the posting, and hands it to the dungeon by reference: the dungeon
/// changes it and hands it back, which is what makes a run something other
/// than a level played twice. It stays on the stack underneath, so returning
/// from the Failles is a pop rather than a rebuild.
class bureau_screen : public screen
{
public:
  void on_enter() override;
  void update(float dt) override;
  void render(float alpha) override;

private:
  script_host m_scripts;
  player_profile m_profile;

  /// Why the last read failed, shown in place rather than left to a log.
  std::string m_data_error;

  run_state m_run;

  /// Whether the post was filled last time this screen ran. A keystroke let go
  /// of while someone was dying must not be what dismisses the notice of their
  /// death.
  bool m_post_was_filled = true;

  action_map m_bindings;
  action_state m_actions;
};

} // namespace arpg
