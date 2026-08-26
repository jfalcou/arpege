-- SPDX-License-Identifier: BSL-1.0
--
-- What a room can field. Reloaded from disk without rebuilding, so these
-- figures are meant to be argued with.
--
-- cost    what a point of the room budget buys; a heavy costs many parasites
-- sight   how far it notices the player; never below the player's own range,
--         or it dies without ever waking up
-- reach   how close it must be to hurt, which for a shooter is where it holds
--
-- shots   a volley is a count, an arc and a rotation, not a named shape:
--         bullets = 1, arc = 0                aimed shot
--         bullets = 5, arc = 40               fan
--         bullets = 8, arc = 360, aim fixed   circle
--         bullets = 1, spin = 23, aim fixed   spiral

return {
  version = 1,

  {
    name = "parasite",
    cost = 5,
    health = 2,
    speed = 58,
    radius = 3,
    touch = 1,
    sight = 288,
    reach = 10,
    style = "melee",

    -- One animation per state: what it does while it waits, while it charges,
    -- and while it strikes. A state left out falls back on the first named.
    look = { atlas = "placeholder", idle = "crawl", chase = "crawl" },
  },
  {
    name = "cultist",
    cost = 10,
    health = 5,
    speed = 40,
    radius = 6,
    touch = 1,
    sight = 288,
    reach = 120,
    style = "ranged",

    -- Sidles while it fires, as a share of its walking speed. A shooter that
    -- holds perfectly still is easy to aim at and easy to ignore.
    strafe = 0.7,
    shots = {
      aim = "aimed",
      bullets = 3,
      arc = 24,
      interval = 1.6,
      speed = 78,
      radius = 2,
      damage = 1,
    },

    look = { atlas = "placeholder", idle = "sway", chase = "sway" },
  },
  {
    name = "acolyte",
    cost = 25,
    health = 8,
    speed = 22,
    radius = 6,
    touch = 1,
    sight = 288,
    reach = 150,
    style = "ranged",
    strafe = 0.45,
    shots = {
      aim = "fixed",
      bullets = 2,
      arc = 180,
      spin = 29,
      interval = 0.5,
      speed = 46,
      radius = 2,
      damage = 1,
      life = 6,
    },
  },
  {
    name = "brute",
    cost = 40,
    health = 20,
    speed = 32,
    radius = 10,
    touch = 2,
    sight = 288,
    reach = 16,
    style = "melee",
  },
}
