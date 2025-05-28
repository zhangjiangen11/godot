/**
 * math.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_MATH_H
#define COMPAT_MATH_H

#include "core/math/math_funcs.h"
#define RAND_RANGE(m_from, m_to) (Math::random(m_from, m_to))
#define RANDF() (Math::randf())

#endif // COMPAT_MATH_H
