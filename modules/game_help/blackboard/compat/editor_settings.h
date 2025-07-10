/**
 * editor_settings.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#ifndef COMPAT_EDITOR_SETTINGS_H
#define COMPAT_EDITOR_SETTINGS_H

#include "editor/settings/editor_settings.h"

#define EDITOR_SETTINGS() (EditorSettings::get_singleton())

#endif // COMPAT_EDITOR_SETTINGS_H
