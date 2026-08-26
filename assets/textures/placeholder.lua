-- SPDX-License-Identifier: BSL-1.0
--
-- Written by the sprite editor. Editing it by hand is fine; the editor
-- reads back what it wrote.

return {
  version = 1,
  image = "placeholder.png",

  frames = {
    { name = "frame_0", rect = { 0, 0, 16, 16 }, origin = { 8, 16 } },
    { name = "frame_1", rect = { 16, 0, 16, 16 }, origin = { 8, 16 } },
    { name = "frame_2", rect = { 32, 0, 16, 16 }, origin = { 8, 16 } },
    { name = "frame_3", rect = { 48, 0, 16, 16 }, origin = { 8, 16 } },
    { name = "frame_4", rect = { 0, 16, 16, 16 }, origin = { 8, 16 } },
    { name = "frame_5", rect = { 16, 16, 16, 16 }, origin = { 8, 16 } },
    { name = "frame_6", rect = { 32, 16, 16, 16 }, origin = { 8, 16 } },
    { name = "frame_7", rect = { 48, 16, 16, 16 }, origin = { 8, 16 } },
  },

  animations = {
    {
      name = "crawl",
      loops = true,
      frames = {
        { "frame_0", 0.1 },
        { "frame_1", 0.1 },
        { "frame_2", 0.1 },
        { "frame_3", 0.1 },
        { "frame_2", 0.1 },
        { "frame_1", 0.1 },
        { "frame_0", 0.1 },
      },
    },
    {
      name = "sway",
      loops = true,
      frames = {
        { "frame_4", 0.14 },
        { "frame_5", 0.14 },
        { "frame_6", 0.14 },
        { "frame_7", 0.14 },
      },
    },
  },
}
