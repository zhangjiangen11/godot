/**
 * print.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_PRINT_STRING_H
#define COMPAT_PRINT_STRING_H

#include <core/string/print_string.h>
#define PRINT_LINE(...) (print_line(__VA_ARGS__))

#endif // COMPAT_PRINT_STRING_H
