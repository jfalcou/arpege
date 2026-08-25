// SPDX-License-Identifier: BSL-1.0

#include <screens/bureau_screen.hpp>

#include <core/pixel_canvas.hpp>
#include <core/raylib_input.hpp>
#include <core/screen_manager.hpp>
#include <screens/dungeon_screen.hpp>

#include <raylib.h>

#include <memory>

namespace arpg
{

namespace
{

constexpr const char* player_file = "data/player.lua";

/// What a posting is signed under. Drawn from nothing yet: the seed of a run
/// belongs to the contract, and there is no contract to draw it from.
constexpr std::uint64_t opening_seed = 20260825;

void line(const pixel_canvas& canvas, const char* text, int y, Color tint)
{
  DrawText(text, (canvas.width() - MeasureText(text, 10)) / 2, y, 10, tint);
}

} // namespace

void bureau_screen::on_enter()
{
  m_bindings = menu_bindings(raylib_input::codes());

  const loaded_player hero = load_player_from(m_scripts, *ctx().assets / player_file);

  m_data_error = hero.error;
  m_profile = hero.value;

  // Signed once, and kept for the whole session: a posting outlives the
  // employees sent out under it.
  m_run = begin_posting(opening_seed, m_profile.health);
}

void bureau_screen::update(float)
{
  m_actions.advance(m_bindings.resolve(*ctx().input));

  // The screen does not run while the Failles are on top of it, so this is the
  // first step after coming back. A press let go of down there is still in
  // flight, and would dismiss a notice nobody has read.
  if (m_post_was_filled && !employee_alive(m_run))
  {
    m_actions.flush();
  }

  m_post_was_filled = employee_alive(m_run);

  if (m_actions.consume(action::cancel))
  {
    ctx().screens->pop();
    return;
  }

  if (!m_actions.consume(action::confirm) || !m_data_error.empty())
  {
    return;
  }

  // An assignment cannot be handed to a corpse. The Bureau puts someone else
  // on the post rather than ending it: the post outlives the person.
  if (!employee_alive(m_run))
  {
    assign_employee(m_run, m_profile.health);
    return;
  }

  ctx().screens->push(std::make_unique<dungeon_screen>(m_run));
}

void bureau_screen::render(float)
{
  const pixel_canvas& canvas = *ctx().canvas;
  ClearBackground(Color{16, 15, 20, 255});

  // A vacancy says so outright. A line changing colour among five others is
  // not an announcement, and someone died.
  const bool vacant = employee_alive(m_run) == false;
  const char* title = vacant ? "VACANCY" : "BUREAU";

  DrawText(title, (canvas.width() - MeasureText(title, 20)) / 2, 18, 20,
           vacant ? Color{198, 88, 78, 255} : Color{226, 205, 154, 255});

  if (!m_data_error.empty())
  {
    line(canvas, m_data_error.c_str(), 60, Color{214, 118, 168, 255});
    return;
  }

  line(canvas, TextFormat("assignment %d", m_run.depth + 1), 48, Color{160, 150, 170, 255});
  line(canvas, TextFormat("filed %d   -   carried %d", m_run.banked, m_run.carried), 62, Color{176, 128, 214, 255});

  if (employee_alive(m_run))
  {
    line(canvas, TextFormat("employee   %d / %d", m_run.health, m_run.health_max), 76, Color{140, 200, 150, 255});
  }
  else
  {
    line(canvas, "the previous holder did not report back", 76, Color{198, 88, 78, 255});
  }

  // The only figure the Bureau keeps of the people themselves.
  line(canvas, TextFormat("write-offs %d", m_run.lost), 90, vacant ? Color{198, 88, 78, 255} : Color{92, 84, 104, 255});

  const bool on_pad = ctx().input->device == input_device::gamepad;

  if (employee_alive(m_run))
  {
    line(canvas, on_pad ? "A  report to the Failles" : "ENTER  report to the Failles", 118, Color{226, 205, 154, 255});
  }
  else
  {
    line(canvas, on_pad ? "A  requisition an employee" : "ENTER  requisition an employee", 118,
         Color{226, 205, 154, 255});
  }

  line(canvas, on_pad ? "B  clock off" : "ESC  clock off", 132, Color{120, 110, 130, 255});
}

} // namespace arpg
