-- SPDX-License-Identifier: BSL-1.0
--
-- One file per biome, so a mod adds one by dropping a file in rather than by
-- replacing a list of them all.
--
-- Every distance is a multiple of the view: "1.4" is a screen and a bit, and
-- stays right if the internal resolution changes. Pixels would not.

return {
  version = 1,

  name = "the Porcelain Lazaret",

  -- Built by people, however mad they were: rooms sit square and aligned.
  shape = "rigid",

  rooms = { 5, 7 },

  room = {
    min = { 0.8, 0.8 },
    max = { 1.2, 1.1 },
  },

  spacing = 0.3,

  -- A name and how often it is offered. Ordered rather than keyed by name:
  -- the iteration order of a Lua table is unspecified, and a level drawn from
  -- a seed cannot depend on it.
  fauna = {
    { "parasite", 5 },
    { "cultist", 3 },
    { "brute", 1 },
  },
}
