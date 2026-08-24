// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <core/app_context.hpp>
#include <core/screen.hpp>

#include <cstddef>
#include <memory>
#include <vector>

namespace arpg
{

/// Stack of screens.
///
/// Change requests are queued and applied at the end of the frame: applying one
/// during an update would destroy a screen while it is running.
class screen_manager
{
public:
  /// Services handed to every screen entering the stack.
  void set_context(const app_context& context) { m_context = context; }

  /// Queues @p value to be stacked on top, the one below staying alive.
  void push(std::unique_ptr<screen> value);

  /// Queues the removal of the topmost screen.
  void pop();

  /// Queues the topmost screen to be swapped for @p value.
  void replace(std::unique_ptr<screen> value);

  /// Queues the removal of every screen.
  void clear();

  /// Applies the queued requests. Call once per frame, after update and render.
  ///
  /// A request queued from within a request, such as an on_enter() pushing a
  /// transition, is handled on the next frame rather than in this pass.
  void apply_pending();

  /// Updates the screens, from the top down to the first blocking one.
  void update(float dt);

  /// Draws from the topmost opaque screen up to the top of the stack.
  void render(float alpha);

  /// Empties the stack at once, calling screen::on_exit() on every screen and
  /// dropping what was queued. For shutting the game down.
  void shutdown();

  /// Whether the stack holds no screen, which ends the game loop.
  bool empty() const { return m_stack.empty(); }

  /// Number of stacked screens.
  std::size_t size() const { return m_stack.size(); }

  /// Topmost screen, or nullptr when the stack is empty.
  screen* top() { return m_stack.empty() ? nullptr : m_stack.back().get(); }

private:
  enum class command_kind
  {
    push,
    pop,
    replace,
    clear
  };

  struct command
  {
    command_kind kind;
    std::unique_ptr<screen> value;
  };

  void enter(std::unique_ptr<screen> value);
  void leave_top();

  std::vector<std::unique_ptr<screen>> m_stack;
  std::vector<command> m_pending;
  app_context m_context{};
};

} // namespace arpg
