/**
 * resource_loader.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#ifndef COMPAT_RESOURCE_LOADER_H
#define COMPAT_RESOURCE_LOADER_H

#include "core/io/resource_loader.h"

#define RESOURCE_LOAD(m_path, m_hint) ResourceLoader::load(m_path, m_hint)
#define RESOURCE_LOAD_NO_CACHE(m_path, m_hint) ResourceLoader::load(m_path, m_hint, ResourceFormatLoader::CACHE_MODE_IGNORE)
#define RESOURCE_EXISTS(m_path, m_type_hint) (ResourceLoader::exists(m_path, m_type_hint))
#define RESOURCE_IS_CACHED(m_path) (ResourceCache::has(m_path))
#define RESOURCE_IS_SCENE_FILE(m_path) (ResourceLoader::get_resource_type(m_path) == "PackedScene")

#endif // COMPAT_RESOURCE_LOADER_H
