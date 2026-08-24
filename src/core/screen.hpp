// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/app_context.hpp>

namespace arpg
{

/// A game mode: menu, roguelike map, dungeon, pause.
///
/// Screens are stacked rather than swapped, so a pause can freeze the dungeon
/// underneath without hiding it.
class screen
{
public:
  virtual ~screen() = default;

  screen() = default;
  screen(const screen&) = delete;
  screen& operator=(const screen&) = delete;

  /// Called when the screen enters the stack; the place to load what it needs.
  virtual void on_enter() {}

  /// Called when the screen leaves the stack; the place to release it.
  virtual void on_exit() {}

  /// One step of simulation. @p dt is always application::fixed_dt.
  virtual void update(float dt) = 0;

  /// Draws the screen.
  ///
  /// @param alpha leftover of the accumulator, in [0, 1[. Interpolate between
  ///        the previous and the current simulation state with it, so movement
  ///        stays smooth whatever the refresh rate.
  virtual void render(float alpha) = 0;

  /// When false, the screen underneath keeps being updated. A pause returns
  /// true here and false below, freezing the dungeon while still showing it.
  virtual bool blocks_update() const { return true; }

  /// When false, the screen underneath keeps being drawn.
  virtual bool blocks_render() const { return true; }

  /// Hands the screen its services. Called before on_enter().
  void attach(const app_context& context) { m_context = context; }

protected:
  /// Services the screen may reach outside of its own world.
  const app_context& ctx() const { return m_context; }

private:
  app_context m_context{};
};

} // namespace arpg
