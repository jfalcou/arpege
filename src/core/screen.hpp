#pragma once

#include "core/app_context.hpp"

namespace arpg
{

// A game mode: menu, roguelike map, dungeon, pause.
class screen
{
public:
  virtual ~screen() = default;

  screen() = default;
  screen(const screen&) = delete;
  screen& operator=(const screen&) = delete;

  // Called by screen_manager when the screen enters or leaves the stack.
  virtual void on_enter() {}
  virtual void on_exit() {}

  // Fixed timestep update: dt is always application::fixed_dt.
  virtual void update(float dt) = 0;

  // alpha is the accumulator leftover in [0, 1[: interpolate between the
  // previous and the current simulation state.
  virtual void render(float alpha) = 0;

  // When false, the screen underneath keeps being updated or drawn.
  virtual bool blocks_update() const { return true; }
  virtual bool blocks_render() const { return true; }

  void attach(const app_context& context) { m_context = context; }

protected:
  const app_context& ctx() const { return m_context; }

private:
  app_context m_context{};
};

} // namespace arpg
