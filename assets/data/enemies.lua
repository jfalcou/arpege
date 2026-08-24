-- SPDX-License-Identifier: BSL-1.0
--
-- What a room can field. Reloaded from disk without rebuilding, so these
-- figures are meant to be argued with.
--
-- cost    what a point of the room budget buys; a heavy costs many parasites
-- sight   how far it notices the player; never below the player's own range,
--         or it dies without ever waking up
-- reach   how close it must be to hurt, which for a shooter is where it holds

return {
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
    fire_interval = 1.4,
    shot_speed = 78,
    shot_radius = 2,
    shot_damage = 1,
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
