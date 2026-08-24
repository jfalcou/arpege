// SPDX-License-Identifier: BSL-1.0

#include <core/action_map.hpp>

#include <algorithm>

namespace arpg
{

void action_map::bind(binding control, action target)
{
  m_entries.push_back(entry{control, target});
}

void action_map::unbind(action target)
{
  const auto removed = std::remove_if(m_entries.begin(), m_entries.end(),
                                      [target](const entry& value) { return value.target == target; });
  m_entries.erase(removed, m_entries.end());
}

void action_map::clear()
{
  m_entries.clear();
}

action_set action_map::resolve(const input_snapshot& snapshot) const
{
  action_set result;

  for (const entry& value : m_entries)
  {
    if (snapshot.holds(value.control))
    {
      result.set(index_of(value.target));
    }
  }

  return result;
}

std::vector<binding> action_map::controls_for(action target) const
{
  std::vector<binding> result;

  for (const entry& value : m_entries)
  {
    if (value.target == target)
    {
      result.push_back(value.control);
    }
  }

  return result;
}

std::vector<binding> action_map::controls() const
{
  std::vector<binding> result;

  for (const entry& value : m_entries)
  {
    if (std::find(result.begin(), result.end(), value.control) == result.end())
    {
      result.push_back(value.control);
    }
  }

  return result;
}

} // namespace arpg
