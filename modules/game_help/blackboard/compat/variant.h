/**
 * variant.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#ifndef COMPAT_VARIANT_H
#define COMPAT_VARIANT_H

#include "core/variant/variant.h"
#define VARIANT_EVALUATE(m_op, m_lvalue, m_rvalue, r_ret) r_ret = Variant::evaluate(m_op, m_lvalue, m_rvalue)

// *** SHARED ***

#define VARIANT_IS_ARRAY(m_variant) (m_variant.get_type() >= Variant::ARRAY)
#define VARIANT_IS_NUM(m_variant) (m_variant.get_type() == Variant::INT || m_variant.get_type() == Variant::FLOAT)

void VARIANT_DELETE_IF_OBJECT(const Variant &p_variant);

Variant VARIANT_DEFAULT(Variant::Type p_type);

#endif // COMPAT_VARIANT_H
