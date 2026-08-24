// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <core/command_line.hpp>

#include <vector>

namespace
{

arpg::launch_options parse(std::vector<std::string_view> arguments)
{
  return arpg::parse_command_line(arguments);
}

} // namespace

TTS_CASE("An empty line asks for nothing")
{
  const arpg::launch_options options = parse({});

  TTS_EXPECT(options.valid());
  TTS_EXPECT(options.assets.empty());
  TTS_EXPECT_NOT(options.help);
};

TTS_CASE("The value may be glued to the option")
{
  const arpg::launch_options options = parse({"--assets=/home/dev/arpg/assets"});

  TTS_EXPECT(options.valid()) << options.error;
  TTS_EQUAL(options.assets, std::string{"/home/dev/arpg/assets"});
};

TTS_CASE("The value may be the next argument")
{
  const arpg::launch_options options = parse({"--assets", "/home/dev/arpg/assets"});

  TTS_EXPECT(options.valid()) << options.error;
  TTS_EQUAL(options.assets, std::string{"/home/dev/arpg/assets"});
};

TTS_CASE("A path holding spaces survives, since the shell already split it")
{
  const arpg::launch_options options = parse({"--assets", "C:/Program Files/arpg/assets"});

  TTS_EXPECT(options.valid()) << options.error;
  TTS_EQUAL(options.assets, std::string{"C:/Program Files/arpg/assets"});
};

TTS_CASE("An option with nothing after it is refused")
{
  const arpg::launch_options options = parse({"--assets"});

  TTS_EXPECT_NOT(options.valid());
};

TTS_CASE("An empty value is refused rather than taken for the current directory")
{
  TTS_EXPECT_NOT(parse({"--assets="}).valid());
  TTS_EXPECT_NOT(parse({"--assets", ""}).valid());
};

TTS_CASE("A misspelt option is refused rather than ignored")
{
  // Ignoring it would look like it worked, and the game would quietly read
  // the wrong directory.
  TTS_EXPECT_NOT(parse({"--asset=/tmp"}).valid());
  TTS_EXPECT_NOT(parse({"--assetsss"}).valid());
  TTS_EXPECT_NOT(parse({"-x"}).valid());
};

TTS_CASE("A stray word is not a directory")
{
  TTS_EXPECT_NOT(parse({"/home/dev/arpg/assets"}).valid());
};

TTS_CASE("Help stops the line where it is")
{
  const arpg::launch_options options = parse({"--help", "--assets=/tmp"});

  TTS_EXPECT(options.help);
  TTS_EXPECT(options.valid());
};

TTS_CASE("The usage names the option it documents")
{
  TTS_EXPECT(arpg::usage().find("--assets") != std::string_view::npos);
};
