#include "core/ScreenManager.hpp"

#include <utility>

namespace arpg
{

void ScreenManager::push(std::unique_ptr<Screen> screen)
{
  m_pending.push_back(Command{CommandKind::Push, std::move(screen)});
}

void ScreenManager::pop()
{
  m_pending.push_back(Command{CommandKind::Pop, nullptr});
}

void ScreenManager::replace(std::unique_ptr<Screen> screen)
{
  m_pending.push_back(Command{CommandKind::Replace, std::move(screen)});
}

void ScreenManager::clear()
{
  m_pending.push_back(Command{CommandKind::Clear, nullptr});
}

void ScreenManager::enter(std::unique_ptr<Screen> screen)
{
  if (screen == nullptr)
  {
    return;
  }

  screen->attach(m_context);
  m_stack.push_back(std::move(screen));
  m_stack.back()->onEnter();
}

void ScreenManager::leaveTop()
{
  if (m_stack.empty())
  {
    return;
  }

  m_stack.back()->onExit();
  m_stack.pop_back();
}

void ScreenManager::applyPending()
{
  if (m_pending.empty())
  {
    return;
  }

  // A command can queue more of them, for instance an onEnter pushing a
  // transition. Those are handled on the next frame.
  std::vector<Command> commands;
  commands.swap(m_pending);

  for (Command& command : commands)
  {
    switch (command.kind)
    {
    case CommandKind::Push:
      enter(std::move(command.screen));
      break;
    case CommandKind::Pop:
      leaveTop();
      break;
    case CommandKind::Replace:
      leaveTop();
      enter(std::move(command.screen));
      break;
    case CommandKind::Clear:
      while (!m_stack.empty())
      {
        leaveTop();
      }
      break;
    }
  }
}

void ScreenManager::update(float dt)
{
  if (m_stack.empty())
  {
    return;
  }

  std::size_t first = m_stack.size() - 1;
  while (first > 0 && !m_stack[first]->blocksUpdate())
  {
    --first;
  }

  for (std::size_t i = first; i < m_stack.size(); ++i)
  {
    m_stack[i]->update(dt);
  }
}

void ScreenManager::render(float alpha)
{
  if (m_stack.empty())
  {
    return;
  }

  std::size_t first = m_stack.size() - 1;
  while (first > 0 && !m_stack[first]->blocksRender())
  {
    --first;
  }

  for (std::size_t i = first; i < m_stack.size(); ++i)
  {
    m_stack[i]->render(alpha);
  }
}

void ScreenManager::shutdown()
{
  m_pending.clear();
  while (!m_stack.empty())
  {
    leaveTop();
  }
}

} // namespace arpg
