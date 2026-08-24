// SPDX-License-Identifier: BSL-1.0

#include <data/script_host.hpp>

#include <fstream>
#include <sstream>

namespace arpg
{

script_host::script_host()
{
  // Only what describing content needs. A data file has no business opening
  // files, reading the clock or loading libraries of its own: those would be
  // ways for an edit meant to tune a number to reach outside the game.
  m_lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
}

script_result script_host::run(std::string_view source, std::string_view name)
{
  script_result out;

  const sol::protected_function_result outcome =
      m_lua.safe_script(std::string{source}, sol::script_pass_on_error, std::string{name});

  if (!outcome.valid())
  {
    const sol::error failure = outcome;
    out.error = failure.what();
    return out;
  }

  const sol::optional<sol::table> table = outcome;

  if (!table)
  {
    out.error = std::string{name} + " must return a table";
    return out;
  }

  out.value = *table;
  return out;
}

script_result script_host::run_file(const std::filesystem::path& path)
{
  std::ifstream file(path);

  if (!file)
  {
    script_result out;
    out.error = "cannot open " + path.string();
    return out;
  }

  std::ostringstream text;
  text << file.rdbuf();

  // Read here rather than left to Lua, so a missing file and a broken one are
  // reported the same way.
  return run(text.str(), path.filename().string());
}

} // namespace arpg
