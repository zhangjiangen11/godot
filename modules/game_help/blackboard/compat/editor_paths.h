/**
 * editor_paths.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#ifndef COMPAT_EDITOR_PATHS_H
#define COMPAT_EDITOR_PATHS_H

#include <editor/editor_interface.h>
#include <editor/editor_paths.h>

#define PROJECT_SETTINGS_DIR() EditorInterface::get_singleton()->get_editor_paths()->get_project_settings_dir()
#define LAYOUT_CONFIG_FILE() PROJECT_SETTINGS_DIR().path_join("editor_layout.cfg")

#endif // COMPAT_EDITOR_PATHS_H
