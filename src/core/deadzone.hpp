// SPDX-License-Identifier: BSL-1.0

#pragma once

#include "core/vec2.hpp"

namespace arpg
{

/// Filters the resting noise out of an analog stick.
///
/// The magnitude of the stick is filtered, not each axis separately. Filtering
/// per axis would let a diagonal through while rejecting the same magnitude on
/// a single axis, and would clip the reachable directions into a square.
///
/// Below @p inner the result is zero; above @p outer it is a unit vector; in
/// between the magnitude is rescaled linearly, so it starts from zero at the
/// edge of the deadzone rather than jumping to @p inner.
///
/// @param stick raw stick position, as reported by the device
/// @param inner magnitude under which the stick counts as resting
/// @param outer magnitude at which the stick counts as fully pushed
vec2 apply_radial_deadzone(vec2 stick, float inner, float outer = 1.0f);

} // namespace arpg
