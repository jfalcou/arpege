-- SPDX-License-Identifier: BSL-1.0

return {
  version = 1,

  name = "the Tallow Reef",

  -- Nothing here was built. Rooms hang off one another as they grew.
  shape = "organic",

  rooms = { 6, 8 },

  room = {
    min = { 0.7, 0.75 },
    max = { 1.3, 1.25 },
  },

  spacing = 0.35,

  -- No brutes: the reef swarms rather than blocks.
  fauna = {
    { "parasite", 6 },
    { "cultist", 2 },
    { "acolyte", 2 },
  },
}
