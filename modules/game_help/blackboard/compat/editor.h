/**
 * editor.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_EDITOR_H
#define COMPAT_EDITOR_H

#ifdef TOOLS_ENABLED

#include "editor/file_system/editor_file_system.h"
#include "editor/editor_interface.h"
#define EDITOR_FILE_SYSTEM() (EditorFileSystem::get_singleton())

// Shared.
void SHOW_BUILTIN_DOC(const String &p_topic);
void EDIT_SCRIPT(const String &p_path);

#endif // TOOLS_ENABLED

#endif // COMPAT_EDITOR_H
