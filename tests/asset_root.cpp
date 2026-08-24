// SPDX-License-Identifier: BSL-1.0

#define TTS_MAIN
#include <tts/tts.hpp>

#include <data/asset_root.hpp>

TTS_CASE("Without a choice the assets sit next to the executable")
{
  TTS_EQUAL(arpg::asset_root("", "", "/opt/arpg/bin"), std::filesystem::path{"/opt/arpg/bin"} / "assets");
};

TTS_CASE("A chosen root is taken as the directory itself")
{
  // Named rather than appended to, so it can point at a working copy that is
  // not called assets.
  TTS_EQUAL(arpg::asset_root("", "/home/dev/arpg/assets", "/opt/arpg/bin"),
            std::filesystem::path{"/home/dev/arpg/assets"});
};

TTS_CASE("What was typed for this run beats what a shell was left set to")
{
  TTS_EQUAL(arpg::asset_root("/tmp/try", "/home/dev/arpg/assets", "/opt/arpg/bin"),
            std::filesystem::path{"/tmp/try"});
};
