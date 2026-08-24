// SPDX-License-Identifier: BSL-1.0

// Compile test: nothing here is executed. Building and linking it is the whole
// assertion, which is how the GUI layer gets checked at all.

#include <core/screen.hpp>
#include <core/screen_manager.hpp>
#include <screens/main_menu_screen.hpp>

#include <memory>
#include <type_traits>

static_assert(std::is_base_of_v<arpg::screen, arpg::main_menu_screen>);
static_assert(std::has_virtual_destructor_v<arpg::screen>);

// A screen owns its world, so copying one would duplicate that world.
static_assert(!std::is_copy_constructible_v<arpg::screen>);
static_assert(!std::is_copy_assignable_v<arpg::screen>);

static_assert(std::is_default_constructible_v<arpg::screen_manager>);

[[maybe_unused]] static void usage()
{
  arpg::screen_manager screens;

  screens.push(std::make_unique<arpg::main_menu_screen>());
  screens.replace(std::make_unique<arpg::main_menu_screen>());
  screens.pop();
  screens.clear();
  screens.apply_pending();

  screens.update(1.0f / 60.0f);
  screens.render(0.5f);
  screens.shutdown();
}

int main()
{
  return 0;
}
