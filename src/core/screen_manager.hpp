#pragma once

#include "core/app_context.hpp"
#include "core/screen.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace arpg
{

// Stack of screens. Change requests are queued and applied at the end of the
// frame: applying them during an update would destroy a running screen.
class screen_manager
{
public:
  void set_context(const app_context& context) { m_context = context; }

  void push(std::unique_ptr<screen> value);
  void pop();
  void replace(std::unique_ptr<screen> value);
  void clear();

  // Applies the queued requests. Call once per frame, after update and render.
  void apply_pending();

  // Updates screens from the top down to the first blocking one.
  void update(float dt);

  // Draws from the topmost opaque screen up to the top of the stack.
  void render(float alpha);

  // Empties the stack immediately, calling on_exit on every screen.
  void shutdown();

  bool empty() const { return m_stack.empty(); }
  std::size_t size() const { return m_stack.size(); }
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
