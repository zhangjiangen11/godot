/**
 * object.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_OBJECT_H
#define COMPAT_OBJECT_H

#include "core/object/object.h"

#define OBJECT_DB_GET_INSTANCE(m_id) ObjectDB::get_instance(ObjectID(m_id))
#define IS_CLASS(m_obj, m_class) (m_obj->is_class_ptr(m_class::get_class_ptr_static()))
#define GET_SCRIPT(m_obj) (m_obj->get_script_instance() ? m_obj->get_script_instance()->get_script() : nullptr)

_FORCE_INLINE_ bool OBJECT_HAS_PROPERTY(Object *p_obj, const StringName &p_prop) {
	bool r_valid;
	return Variant(p_obj).has_key(p_prop, r_valid);
}

#endif // COMPAT_OBJECT_H
