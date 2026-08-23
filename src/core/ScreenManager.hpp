#pragma once

#include "core/AppContext.hpp"
#include "core/Screen.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace arpg
{

// Stack of screens. Change requests are queued and applied at the end of the
// frame: applying them during an update would destroy a running screen.
class ScreenManager
{
public:
  void setContext(const AppContext& context) { m_context = context; }

  void push(std::unique_ptr<Screen> screen);
  void pop();
  void replace(std::unique_ptr<Screen> screen);
  void clear();

  // Applies the queued requests. Call once per frame, after update and render.
  void applyPending();

  // Updates screens from the top down to the first blocking one.
  void update(float dt);

  // Draws from the topmost opaque screen up to the top of the stack.
  void render(float alpha);

  // Empties the stack immediately, calling onExit on every screen.
  void shutdown();

  bool empty() const { return m_stack.empty(); }
  std::size_t size() const { return m_stack.size(); }
  Screen* top() { return m_stack.empty() ? nullptr : m_stack.back().get(); }

private:
  enum class CommandKind
  {
    Push,
    Pop,
    Replace,
    Clear
  };

  struct Command
  {
    CommandKind kind;
    std::unique_ptr<Screen> screen;
  };

  void enter(std::unique_ptr<Screen> screen);
  void leaveTop();

  std::vector<std::unique_ptr<Screen>> m_stack;
  std::vector<Command> m_pending;
  AppContext m_context{};
};

} // namespace arpg
