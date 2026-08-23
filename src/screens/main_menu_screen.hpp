// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/screen.hpp"

#include <raylib.h>

namespace arpg
{

class main_menu_screen : public screen
{
public:
  void on_enter() override;
  void update(float dt) override;
  void render(float alpha) override;

private:
  // Two positions per entity: rendering interpolates between the previous and
  // the current simulation step.
  Vector2 m_previous{};
  Vector2 m_current{};
  Vector2 m_velocity{};
  float m_radius = 4.0f;
};

} // namespace arpg
