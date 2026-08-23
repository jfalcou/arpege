// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/action.hpp"
#include "core/input_snapshot.hpp"

#include <bitset>
#include <vector>

namespace arpg
{

using action_set = std::bitset<action_count>;

// Bindings for one context. A screen owns the map it needs: the dungeon maps
// movement and shooting, a menu maps navigation, and the same physical key can
// mean different things in each without either knowing about the other.
class action_map
{
public:
  void bind(binding control, action target);

  // Removes every binding pointing at that action.
  void unbind(action target);
  void clear();

  // Which actions the given controls are currently holding down.
  action_set resolve(const input_snapshot& snapshot) const;

  // Controls bound to an action, in binding order. Lets the UI show the right
  // prompt for the device in use.
  std::vector<binding> controls_for(action target) const;

  // Every bound control, without duplicates, for the platform layer to poll.
  // Polling only what is bound avoids walking the hundreds of key codes the
  // backend knows about.
  std::vector<binding> controls() const;

  std::size_t size() const { return m_entries.size(); }

private:
  struct entry
  {
    binding control;
    action target;
  };

  std::vector<entry> m_entries;
};

} // namespace arpg
