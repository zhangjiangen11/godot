/**
 * resource.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_RESOURCE_H
#define COMPAT_RESOURCE_H

#include "core/io/resource.h"

#define IS_RESOURCE_FILE(m_path) (m_path.begins_with("res://") && m_path.find("::") == -1)
#define RESOURCE_TYPE_HINT(m_type) vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, m_type)
#define RESOURCE_IS_BUILT_IN(m_res) (m_res->get_path().is_empty() || m_res->get_path().contains("::"))
#define RESOURCE_IS_EXTERNAL(m_res) (!RESOURCE_IS_BUILT_IN(m_res))
#define RESOURCE_PATH_IS_BUILT_IN(m_path) (m_path.is_empty() || m_path.contains("::"))
#define RESOURCE_PATH_IS_EXTERNAL(m_path) (!RESOURCE_PATH_IS_BUILT_IN(m_path))

#endif // COMPAT_RESOURCE_H
