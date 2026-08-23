// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/vec2.hpp"

namespace arpg
{

// Radial deadzone: the magnitude of the stick is filtered, not each axis
// separately. Filtering per axis would let a diagonal push through while the
// same magnitude on a single axis is rejected, and would clip the reachable
// directions into a square.
//
// Below inner the result is zero; above outer it is a unit vector; in between
// the magnitude is rescaled linearly so it starts from zero at the edge of the
// deadzone rather than jumping to inner.
vec2 apply_radial_deadzone(vec2 stick, float inner, float outer = 1.0f);

} // namespace arpg
