#define TTS_MAIN
#include <tts/tts.hpp>

#include "core/ScreenManager.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{

// Stand-in for a real screen: it draws nothing and only records the calls it
// receives, which is what makes the stack observable without a window.
class FakeScreen : public arpg::Screen
{
public:
  FakeScreen(std::vector<std::string>& trace, std::string name, bool blocksUpdate = true, bool blocksRender = true)
    : m_trace(&trace)
    , m_name(std::move(name))
    , m_blocksUpdate(blocksUpdate)
    , m_blocksRender(blocksRender)
  {
  }

  void onEnter() override { m_trace->push_back(m_name + ":enter"); }
  void onExit() override { m_trace->push_back(m_name + ":exit"); }
  void update(float) override { m_trace->push_back(m_name + ":update"); }
  void render(float) override { m_trace->push_back(m_name + ":render"); }
  bool blocksUpdate() const override { return m_blocksUpdate; }
  bool blocksRender() const override { return m_blocksRender; }

private:
  std::vector<std::string>* m_trace;
  std::string m_name;
  bool m_blocksUpdate;
  bool m_blocksRender;
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

std::unique_ptr<arpg::Screen> makeScreen(std::vector<std::string>& trace, std::string name, bool blocksUpdate = true,
                                         bool blocksRender = true)
{
  return std::make_unique<FakeScreen>(trace, std::move(name), blocksUpdate, blocksRender);
}

} // namespace

TTS_CASE("A push only takes effect once the frame ends")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "menu"));
  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{});

  screens.applyPending();
  TTS_EQUAL(screens.size(), 1U);
  TTS_EQUAL(joined(trace), std::string{"menu:enter"});
};

TTS_CASE("A blocking screen hides the one below it")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "map"));
  screens.push(makeScreen(trace, "pause"));
  screens.applyPending();
  trace.clear();

  screens.update(1.0f / 60.0f);
  screens.render(0.0f);

  TTS_EQUAL(joined(trace), std::string{"pause:update pause:render"});
};

TTS_CASE("A screen that does not block lets the one below run")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "map"));
  screens.push(makeScreen(trace, "overlay", false, false));
  screens.applyPending();
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
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "dungeon"));
  screens.push(makeScreen(trace, "pause", true, false));
  screens.applyPending();
  trace.clear();

  screens.update(1.0f / 60.0f);
  screens.render(0.0f);

  TTS_EQUAL(joined(trace), std::string{"pause:update dungeon:render pause:render"});
};

TTS_CASE("Replace leaves the old screen before entering the new one")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "menu"));
  screens.applyPending();
  trace.clear();

  screens.replace(makeScreen(trace, "map"));
  screens.applyPending();

  TTS_EQUAL(screens.size(), 1U);
  TTS_EQUAL(joined(trace), std::string{"menu:exit map:enter"});
};

TTS_CASE("Popping the last screen empties the stack")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "menu"));
  screens.applyPending();
  screens.pop();
  screens.applyPending();

  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{"menu:enter menu:exit"});
};

TTS_CASE("Popping an empty stack is harmless")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.pop();
  screens.applyPending();

  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{});
};

TTS_CASE("Clear unwinds the whole stack from the top")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "menu"));
  screens.push(makeScreen(trace, "map"));
  screens.push(makeScreen(trace, "dungeon"));
  screens.applyPending();
  trace.clear();

  screens.clear();
  screens.applyPending();

  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{"dungeon:exit map:exit menu:exit"});
};

TTS_CASE("Several requests queued in one frame are applied in order")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "menu"));
  screens.push(makeScreen(trace, "map"));
  screens.pop();
  screens.applyPending();

  TTS_EQUAL(screens.size(), 1U);
  TTS_EQUAL(joined(trace), std::string{"menu:enter map:enter map:exit"});
};

TTS_CASE("Shutdown leaves every screen and drops what was queued")
{
  std::vector<std::string> trace;
  arpg::ScreenManager screens;

  screens.push(makeScreen(trace, "menu"));
  screens.push(makeScreen(trace, "map"));
  screens.applyPending();
  trace.clear();

  screens.push(makeScreen(trace, "never"));
  screens.shutdown();

  TTS_EXPECT(screens.empty());
  TTS_EQUAL(joined(trace), std::string{"map:exit menu:exit"});
};
