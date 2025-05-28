/**
 * performance.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_PERFORMANCE_H
#define COMPAT_PERFORMANCE_H

#include "main/performance.h"
#define PERFORMANCE_ADD_CUSTOM_MONITOR(m_id, m_callable) (Performance::get_singleton()->add_custom_monitor(m_id, m_callable, Variant()))

#endif // COMPAT_PERFORMANCE_H
