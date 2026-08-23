#include "core/screen_manager.hpp"

#include <utility>

namespace arpg
{

void screen_manager::push(std::unique_ptr<screen> value)
{
  m_pending.push_back(command{command_kind::push, std::move(value)});
}

void screen_manager::pop()
{
  m_pending.push_back(command{command_kind::pop, nullptr});
}

void screen_manager::replace(std::unique_ptr<screen> value)
{
  m_pending.push_back(command{command_kind::replace, std::move(value)});
}

void screen_manager::clear()
{
  m_pending.push_back(command{command_kind::clear, nullptr});
}

void screen_manager::enter(std::unique_ptr<screen> value)
{
  if (value == nullptr)
  {
    return;
  }

  value->attach(m_context);
  m_stack.push_back(std::move(value));
  m_stack.back()->on_enter();
}

void screen_manager::leave_top()
{
  if (m_stack.empty())
  {
    return;
  }

  m_stack.back()->on_exit();
  m_stack.pop_back();
}

void screen_manager::apply_pending()
{
  if (m_pending.empty())
  {
    return;
  }

  // A command can queue more of them, for instance an on_enter pushing a
  // transition. Those are handled on the next frame.
  std::vector<command> commands;
  commands.swap(m_pending);

  for (command& entry : commands)
  {
    switch (entry.kind)
    {
    case command_kind::push:
      enter(std::move(entry.value));
      break;
    case command_kind::pop:
      leave_top();
      break;
    case command_kind::replace:
      leave_top();
      enter(std::move(entry.value));
      break;
    case command_kind::clear:
      while (!m_stack.empty())
      {
        leave_top();
      }
      break;
    }
  }
}

void screen_manager::update(float dt)
{
  if (m_stack.empty())
  {
    return;
  }

  std::size_t first = m_stack.size() - 1;
  while (first > 0 && !m_stack[first]->blocks_update())
  {
    --first;
  }

  for (std::size_t i = first; i < m_stack.size(); ++i)
  {
    m_stack[i]->update(dt);
  }
}

void screen_manager::render(float alpha)
{
  if (m_stack.empty())
  {
    return;
  }

  std::size_t first = m_stack.size() - 1;
  while (first > 0 && !m_stack[first]->blocks_render())
  {
    --first;
  }

  for (std::size_t i = first; i < m_stack.size(); ++i)
  {
    m_stack[i]->render(alpha);
  }
}

void screen_manager::shutdown()
{
  m_pending.clear();
  while (!m_stack.empty())
  {
    leave_top();
  }
}

} // namespace arpg
