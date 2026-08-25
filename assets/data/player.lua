-- SPDX-License-Identifier: BSL-1.0
--
-- What the player is worth. Reloaded from disk without rebuilding, and any
-- change restarts the room from the same seed so a figure is judged against
-- the room it was changed for.
--
-- hitbox  a few pixels, far smaller than what is drawn: a wall of bullets has
--         to be threadable where it looks solid
-- mercy   seconds of invulnerability, after a hit and during a dash
-- gun     life times speed is how far a shot carries, and no enemy may wake
--         closer than that or it dies without ever noticing

return {
  -- What this file is written for. A build that reads another version says so
  -- rather than complaining about a field.
  version = 1,

  health = 3,
  speed = 70,
  focus_speed = 30,
  hitbox = 2,
  mercy = 0.8,

  gun = {
    interval = 0.12,
    speed = 220,
    radius = 1.5,
    life = 1.2,
    damage = 1,
  },

  dash = {
    speed = 260,
    duration = 0.14,
    cooldown = 0.55,
    mercy = 0.14,
  },
}
