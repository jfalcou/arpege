#pragma once

#include "core/AppContext.hpp"

namespace arpg
{

// A game mode: menu, roguelike map, dungeon, pause.
class Screen
{
public:
  virtual ~Screen() = default;

  Screen() = default;
  Screen(const Screen&) = delete;
  Screen& operator=(const Screen&) = delete;

  // Called by ScreenManager when the screen enters or leaves the stack.
  virtual void onEnter() {}
  virtual void onExit() {}

  // Fixed timestep update: dt is always Application::kFixedDt.
  virtual void update(float dt) = 0;

  // alpha is the accumulator leftover in [0, 1[: interpolate between the
  // previous and the current simulation state.
  virtual void render(float alpha) = 0;

  // When false, the screen underneath keeps being updated or drawn.
  virtual bool blocksUpdate() const { return true; }
  virtual bool blocksRender() const { return true; }

  void attach(const AppContext& context) { m_context = context; }

protected:
  const AppContext& ctx() const { return m_context; }

private:
  AppContext m_context{};
};

} // namespace arpg
