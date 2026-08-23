// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/action_map.hpp"
#include "core/action_state.hpp"
#include "core/screen.hpp"
#include "core/vec2.hpp"

namespace arpg
{

/// Title screen, and the only screen there is for now.
class main_menu_screen : public screen
{
public:
  void on_enter() override;
  void update(float dt) override;
  void render(float alpha) override;

private:
  // The screen owns its bindings: the same key means something else in a
  // dungeon, and neither map needs to know about the other.
  action_map m_bindings;
  action_state m_actions;

  // Two positions per entity: rendering interpolates between the previous and
  // the current simulation step.
  vec2 m_previous{};
  vec2 m_current{};
  vec2 m_velocity{};
  float m_radius = 4.0f;
};

} // namespace arpg
