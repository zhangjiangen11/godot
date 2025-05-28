/**
 * debugger.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_DEBUGGER_H
#define COMPAT_DEBUGGER_H

#include <core/debugger/engine_debugger.h>
#define IS_DEBUGGER_ACTIVE() (EngineDebugger::is_active())

#endif // COMPAT_DEBUGGER_H
