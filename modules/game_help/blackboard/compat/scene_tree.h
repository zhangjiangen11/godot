/**
 * scene_tree.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_SCENE_TREE_H
#define COMPAT_SCENE_TREE_H

#include "scene/main/scene_tree.h"
#define SCENE_TREE() (SceneTree::get_singleton())

#endif // COMPAT_SCENE_TREE_H
