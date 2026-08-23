// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include "core/screen_manager.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

// Stand-in for a real screen: it draws nothing and only records the calls it
// receives, which is what makes the stack observable without a window.
class fake_screen : public arpg::screen
{
public:
  fake_screen(std::vector<std::string>& trace, std::string name, bool blocks_update = true, bool blocks_render = true)
    : m_trace(&trace)
    , m_name(std::move(name))
    , m_blocks_update(blocks_update)
    , m_blocks_render(blocks_render)
  {
  }

  void on_enter() override { m_trace->push_back(m_name + ":enter"); }
  void on_exit() override { m_trace->push_back(m_name + ":exit"); }
  void update(float) override { m_trace->push_back(m_name + ":update"); }
  void render(float) override { m_trace->push_back(m_name + ":render"); }
  bool blocks_update() const override { return m_blocks_update; }
  bool blocks_render() const override { return m_blocks_render; }

private:
  std::vector<std::string>* m_trace;
  std::string m_name;
  bool m_blocks_update;
  bool m_blocks_render;
};

std::string joined(const std::vector<std::string>& trace)
{
  std::string out;
  for (const std::string& entry : trace)
  {
    if (!out.empty())
    {
      out += ' ';
    }
    out += entry;
  }
  return out;
}

std::unique_ptr<arpg::screen> make_screen(std::vector<std::string>& trace, std::string name, bool blocks_update = true,
                                         bool blocks_render = true)
{
  return std::make_unique<fake_screen>(trace, std::move(name), blocks_update, blocks_render);
}

} // namespace

TTS_CASE("A push only takes effect once the frame ends")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "menu"));
  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{});

  screens.apply_pending();
  TTS_EQUAL(screens.size(), 1U);
  TTS_EQUAL(joined(trace), std::string{"menu:enter"});
};

TTS_CASE("A blocking screen hides the one below it")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "map"));
  screens.push(make_screen(trace, "pause"));
  screens.apply_pending();
  trace.clear();

  screens.update(1.0f / 60.0f);
  screens.render(0.0f);

  TTS_EQUAL(joined(trace), std::string{"pause:update pause:render"});
};

TTS_CASE("A screen that does not block lets the one below run")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "map"));
  screens.push(make_screen(trace, "overlay", false, false));
  screens.apply_pending();
  trace.clear();

  screens.update(1.0f / 60.0f);
  TTS_EQUAL(joined(trace), std::string{"map:update overlay:update"});

  trace.clear();
  screens.render(0.0f);
  TTS_EQUAL(joined(trace), std::string{"map:render overlay:render"});
};

TTS_CASE("A pause blocks the update but not the rendering")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "dungeon"));
  screens.push(make_screen(trace, "pause", true, false));
  screens.apply_pending();
  trace.clear();

  screens.update(1.0f / 60.0f);
  screens.render(0.0f);

  TTS_EQUAL(joined(trace), std::string{"pause:update dungeon:render pause:render"});
};

TTS_CASE("Replace leaves the old screen before entering the new one")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "menu"));
  screens.apply_pending();
  trace.clear();

  screens.replace(make_screen(trace, "map"));
  screens.apply_pending();

  TTS_EQUAL(screens.size(), 1U);
  TTS_EQUAL(joined(trace), std::string{"menu:exit map:enter"});
};

TTS_CASE("Popping the last screen empties the stack")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "menu"));
  screens.apply_pending();
  screens.pop();
  screens.apply_pending();

  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{"menu:enter menu:exit"});
};

TTS_CASE("Popping an empty stack is harmless")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.pop();
  screens.apply_pending();

  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{});
};

TTS_CASE("Clear unwinds the whole stack from the top")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "menu"));
  screens.push(make_screen(trace, "map"));
  screens.push(make_screen(trace, "dungeon"));
  screens.apply_pending();
  trace.clear();

  screens.clear();
  screens.apply_pending();

  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{"dungeon:exit map:exit menu:exit"});
};

TTS_CASE("Several requests queued in one frame are applied in order")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "menu"));
  screens.push(make_screen(trace, "map"));
  screens.pop();
  screens.apply_pending();

  TTS_EQUAL(screens.size(), 1U);
  TTS_EQUAL(joined(trace), std::string{"menu:enter map:enter map:exit"});
};

TTS_CASE("Shutdown leaves every screen and drops what was queued")
{
  std::vector<std::string> trace;
  arpg::screen_manager screens;

  screens.push(make_screen(trace, "menu"));
  screens.push(make_screen(trace, "map"));
  screens.apply_pending();
  trace.clear();

  screens.push(make_screen(trace, "never"));
  screens.shutdown();

  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{"map:exit menu:exit"});
};
