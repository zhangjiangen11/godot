/**************************************************************************/
/*  vector.h                                                              */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef VECTOR_H
#define VECTOR_H

/**
 * @class Vector
 * Vector container. Simple copy-on-write container.
 *
 * LocalVector is an alternative available for internal use when COW is not
 * required.
 */

#include "core/error/error_macros.h"
#include "core/os/memory.h"
#include "core/templates/cowdata.h"
#include "core/templates/search_array.h"
#include "core/templates/sort_array.h"

#include "core/math/math_funcs.h"

#include <climits>
#include <initializer_list>
#include <utility>


struct CodeHelp
{
	union MarshallFloat {
		uint32_t i; ///< int
		float f; ///< float
	};

	union MarshallDouble {
		uint64_t l; ///< long long
		double d; ///< double
	};

	// Behaves like one of the above, depending on compilation setting.
	union MarshallReal {
#ifdef REAL_T_IS_DOUBLE
		uint64_t i;
#else
		uint32_t i;
#endif
		real_t r;
	};
	static inline unsigned int encode_uint16(uint16_t p_uint, uint8_t* p_arr) {
		for (int i = 0; i < 2; i++) {
			*p_arr = p_uint & 0xFF;
			p_arr++;
			p_uint >>= 8;
		}

		return sizeof(uint16_t);
	}

	static inline unsigned int encode_uint32(uint32_t p_uint, uint8_t* p_arr) {
		for (int i = 0; i < 4; i++) {
			*p_arr = p_uint & 0xFF;
			p_arr++;
			p_uint >>= 8;
		}

		return sizeof(uint32_t);
	}

	static inline unsigned int encode_half(float p_float, uint8_t* p_arr) {
		encode_uint16(Math::make_half_float(p_float), p_arr);

		return sizeof(uint16_t);
	}

	static inline unsigned int encode_float(float p_float, uint8_t* p_arr) {
		MarshallFloat mf;
		mf.f = p_float;
		encode_uint32(mf.i, p_arr);

		return sizeof(uint32_t);
	}

	static inline unsigned int encode_uint64(uint64_t p_uint, uint8_t* p_arr) {
		for (int i = 0; i < 8; i++) {
			*p_arr = p_uint & 0xFF;
			p_arr++;
			p_uint >>= 8;
		}

		return sizeof(uint64_t);
	}

	static inline unsigned int encode_double(double p_double, uint8_t* p_arr) {
		MarshallDouble md;
		md.d = p_double;
		encode_uint64(md.l, p_arr);

		return sizeof(uint64_t);
	}



	static inline uint16_t decode_uint16(const uint8_t* p_arr) {
		uint16_t u = 0;

		for (int i = 0; i < 2; i++) {
			uint16_t b = *p_arr;
			b <<= (i * 8);
			u |= b;
			p_arr++;
		}

		return u;
	}
	static inline uint32_t decode_uint32(const uint8_t* p_arr) {
		uint32_t u = 0;

		for (int i = 0; i < 4; i++) {
			uint32_t b = *p_arr;
			b <<= (i * 8);
			u |= b;
			p_arr++;
		}

		return u;
	}
	static inline float decode_half(const uint8_t* p_arr) {
		return Math::half_to_float(decode_uint16(p_arr));
	}
	static inline float decode_float(const uint8_t* p_arr) {
		MarshallFloat mf;
		mf.i = decode_uint32(p_arr);
		return mf.f;
	}
	static inline uint64_t decode_uint64(const uint8_t* p_arr) {
		uint64_t u = 0;

		for (int i = 0; i < 8; i++) {
			uint64_t b = (*p_arr) & 0xFF;
			b <<= (i * 8);
			u |= b;
			p_arr++;
		}

		return u;
	}
	static inline double decode_double(const uint8_t* p_arr) {
		MarshallDouble md;
		md.l = decode_uint64(p_arr);
		return md.d;
	}
};




template <typename T>
class VectorWriteProxy {
public:
	_FORCE_INLINE_ T &operator[](typename CowData<T>::Size p_index) {
		CRASH_BAD_INDEX(p_index, ((Vector<T> *)(this))->_cowdata.size());

		return ((Vector<T> *)(this))->_cowdata.ptrw()[p_index];
	}
};

template <typename T>
class Vector {
	friend class VectorWriteProxy<T>;

public:
	VectorWriteProxy<T> write;
	typedef typename CowData<T>::Size Size;

private:
	CowData<T> _cowdata;

public:
	// Must take a copy instead of a reference (see GH-31736).
	bool push_back(T p_elem);
	_FORCE_INLINE_ bool append(const T &p_elem) { return push_back(p_elem); } //alias
	void fill(T p_elem);

	void remove_at(Size p_index) { _cowdata.remove_at(p_index); }
	_FORCE_INLINE_ bool erase(const T &p_val) {
		Size idx = find(p_val);
		if (idx >= 0) {
			remove_at(idx);
			return true;
		}
		return false;
	}

	void reverse();

	_FORCE_INLINE_ T *ptrw() { return _cowdata.ptrw(); }
	_FORCE_INLINE_ const T *ptr() const { return _cowdata.ptr(); }
	_FORCE_INLINE_ void clear() { resize(0); }
	_FORCE_INLINE_ bool is_empty() const { return _cowdata.is_empty(); }

	_FORCE_INLINE_ T get(Size p_index) { return _cowdata.get(p_index); }
	_FORCE_INLINE_ const T &get(Size p_index) const { return _cowdata.get(p_index); }
	_FORCE_INLINE_ void set(Size p_index, const T &p_elem) { _cowdata.set(p_index, p_elem); }
	_FORCE_INLINE_ Size size() const { return _cowdata.size(); }
	_FORCE_INLINE_ Error resize(Size p_size) { return _cowdata.resize(p_size); }
	_FORCE_INLINE_ Error resize_zeroed(Size p_size) { return _cowdata.template resize<true>(p_size); }
	_FORCE_INLINE_ const T &operator[](Size p_index) const { return _cowdata.get(p_index); }
	// Must take a copy instead of a reference (see GH-31736).
	Error insert(Size p_pos, T p_val) { return _cowdata.insert(p_pos, p_val); }
	Size find(const T &p_val, Size p_from = 0) const { return _cowdata.find(p_val, p_from); }
	Size rfind(const T &p_val, Size p_from = -1) const { return _cowdata.rfind(p_val, p_from); }
	Size count(const T &p_val) const { return _cowdata.count(p_val); }

	// Must take a copy instead of a reference (see GH-31736).
	void append_array(Vector<T> p_other);

	_FORCE_INLINE_ bool has(const T &p_val) const { return find(p_val) != -1; }

	_FORCE_INLINE_ void sort() {
		sort_custom<_DefaultComparator<T>>();
	}

	template <typename Comparator, bool Validate = SORT_ARRAY_VALIDATE_ENABLED, typename... Args>
	void sort_custom(Args &&...args) {
		Size len = _cowdata.size();
		if (len == 0) {
			return;
		}

		T *data = ptrw();
		SortArray<T, Comparator, Validate> sorter{ args... };
		sorter.sort(data, len);
	}

	_FORCE_INLINE_ Size bsearch(const T &p_value, bool p_before) {
		return bsearch_custom<_DefaultComparator<T>>(p_value, p_before);
	}

	template <typename Comparator, typename Value, typename... Args>
	_FORCE_INLINE_ Size bsearch_custom(const Value &p_value, bool p_before, Args &&...args) {
		SearchArray<T, Comparator> search{ args... };
		return search.bisect(ptrw(), size(), p_value, p_before);
	}

	_FORCE_INLINE_ Vector<T> duplicate() {
		return *this;
	}

	void ordered_insert(const T &p_val) {
		Size i;
		for (i = 0; i < _cowdata.size(); i++) {
			if (p_val < operator[](i)) {
				break;
			}
		}
		insert(i, p_val);
	}

	_FORCE_INLINE_ void operator=(const Vector &p_from) { _cowdata._ref(p_from._cowdata); }
	_FORCE_INLINE_ void operator=(Vector &&p_from) { _cowdata = std::move(p_from._cowdata); }

	_FORCE_INLINE_ Vector<uint8_t> to_byte_array() const {
		Vector<uint8_t> ret;
		if (is_empty()) {
			return ret;
		}
		size_t alloc_size = size() * sizeof(T);
		ret.resize(alloc_size);
		if (alloc_size) {
			memcpy(ret.ptrw(), ptr(), alloc_size);
		}
		return ret;
	}

	Vector<T> slice(Size p_begin, Size p_end = CowData<T>::MAX_INT) const {
		Vector<T> result;

		const Size s = size();

		Size begin = CLAMP(p_begin, -s, s);
		if (begin < 0) {
			begin += s;
		}
		Size end = CLAMP(p_end, -s, s);
		if (end < 0) {
			end += s;
		}

		ERR_FAIL_COND_V(begin > end, result);

		Size result_size = end - begin;
		result.resize(result_size);

		const T *const r = ptr();
		T *const w = result.ptrw();
		for (Size i = 0; i < result_size; ++i) {
			w[i] = r[begin + i];
		}

		return result;
	}

	bool operator==(const Vector<T> &p_arr) const {
		Size s = size();
		if (s != p_arr.size()) {
			return false;
		}
		for (Size i = 0; i < s; i++) {
			if (operator[](i) != p_arr[i]) {
				return false;
			}
		}
		return true;
	}
	bool operator<(const Vector<T> &p_arr) const {
		int s = size();
		if (s != p_arr.size()) {
			return false;
		}
		return ptr() < p_arr.ptr();
	}
	bool operator<=(const Vector<T> &p_arr) const {
		int s = size();
		if (s != p_arr.size()) {
			return false;
		}
		return ptr() <= p_arr.ptr();
	}

	bool operator!=(const Vector<T> &p_arr) const {
		Size s = size();
		if (s != p_arr.size()) {
			return true;
		}
		for (Size i = 0; i < s; i++) {
			if (operator[](i) != p_arr[i]) {
				return true;
			}
		}
		return false;
	}

	struct Iterator {
		_FORCE_INLINE_ T &operator*() const {
			return *elem_ptr;
		}
		_FORCE_INLINE_ T *operator->() const { return elem_ptr; }
		_FORCE_INLINE_ Iterator &operator++() {
			elem_ptr++;
			return *this;
		}
		_FORCE_INLINE_ Iterator &operator--() {
			elem_ptr--;
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator &b) const { return elem_ptr == b.elem_ptr; }
		_FORCE_INLINE_ bool operator!=(const Iterator &b) const { return elem_ptr != b.elem_ptr; }

		_FORCE_INLINE_ Iterator(T *p_ptr) { elem_ptr = p_ptr; }
		_FORCE_INLINE_ Iterator() {}
		_FORCE_INLINE_ Iterator(const Iterator &p_it) { elem_ptr = p_it.elem_ptr; }

	private:
		T *elem_ptr = nullptr;
	};

	struct ConstIterator {
		_FORCE_INLINE_ const T &operator*() const {
			return *elem_ptr;
		}
		_FORCE_INLINE_ const T *operator->() const { return elem_ptr; }
		_FORCE_INLINE_ ConstIterator &operator++() {
			elem_ptr++;
			return *this;
		}
		_FORCE_INLINE_ ConstIterator &operator--() {
			elem_ptr--;
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ConstIterator &b) const { return elem_ptr == b.elem_ptr; }
		_FORCE_INLINE_ bool operator!=(const ConstIterator &b) const { return elem_ptr != b.elem_ptr; }

		_FORCE_INLINE_ ConstIterator(const T *p_ptr) { elem_ptr = p_ptr; }
		_FORCE_INLINE_ ConstIterator() {}
		_FORCE_INLINE_ ConstIterator(const ConstIterator &p_it) { elem_ptr = p_it.elem_ptr; }

	private:
		const T *elem_ptr = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() {
		return Iterator(ptrw());
	}
	_FORCE_INLINE_ Iterator end() {
		return Iterator(ptrw() + size());
	}

	_FORCE_INLINE_ ConstIterator begin() const {
		return ConstIterator(ptr());
	}
	_FORCE_INLINE_ ConstIterator end() const {
		return ConstIterator(ptr() + size());
	}

	_FORCE_INLINE_ Vector() {}
	_FORCE_INLINE_ Vector(std::initializer_list<T> p_init) :
			_cowdata(p_init) {}
	_FORCE_INLINE_ Vector(const Vector &p_from) { _cowdata._ref(p_from._cowdata); }
	_FORCE_INLINE_ Vector(Vector &&p_from) :
			_cowdata(std::move(p_from._cowdata)) {}

	_FORCE_INLINE_ ~Vector() {}

	private:
	public:

		uint8_t decode_u8(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return r[p_offset];
		}
		int8_t decode_s8(int64_t p_offset)  const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return *((const int8_t*)&r[p_offset]);
		}
		int16_t decode_u16(int64_t p_offset)  const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return CodeHelp::decode_uint16(&r[p_offset]);
		}
		int16_t decode_s16(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return (int16_t)CodeHelp::decode_uint16(&r[p_offset]);
		}
		int32_t decode_u32(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return CodeHelp::decode_uint32(&r[p_offset]);
		}
		int32_t  decode_s32(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return (int32_t)CodeHelp::decode_uint32(&r[p_offset]);
		}
		int64_t decode_u64(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return (int64_t)CodeHelp::decode_uint64(&r[p_offset]);
		}
		int64_t decode_s64(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return (int64_t)CodeHelp::decode_uint64(&r[p_offset]);
		}
		double decode_half(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return Math::half_to_float(CodeHelp::decode_uint16(&r[p_offset]));
		}
		double decode_float(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return CodeHelp::decode_float(&r[p_offset]);
		}

		double decode_double(int64_t p_offset) const {
			if (sizeof(T) != 1) {
				return 0;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset >(int64_t(_size) - 4)) return 0;
			const uint8_t* r = (const uint8_t*)ptr();
			return CodeHelp::decode_double(&r[p_offset]);
		}

	public:
		void encode_u8(int64_t p_offset, uint8_t p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			*((uint8_t*)&w[p_offset]) = p_value;
		}
		void encode_s8(int64_t p_offset, int8_t p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			*((int8_t*)&w[p_offset]) = p_value;
		}

		void encode_u16(int64_t p_offset, uint16_t p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_uint16((uint16_t)p_value, &w[p_offset]);
		}
		void encode_s16(int64_t p_offset, int16_t p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_uint16((int16_t)p_value, &w[p_offset]);
		}

		void encode_u32(int64_t p_offset, uint32_t p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_uint32((uint32_t)p_value, &w[p_offset]);
		}
		void encode_s32(int64_t p_offset, int32_t p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_uint32((int32_t)p_value, &w[p_offset]);
		}

		void encode_u64(int64_t p_offset, uint64_t p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_uint64((uint64_t)p_value, &w[p_offset]);
		}
		void encode_s64(int64_t p_offset, int64_t p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_uint64((int64_t)p_value, &w[p_offset]);
		}

		void encode_half(int64_t p_offset, double p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_uint16(Math::make_half_float(p_value), &w[p_offset]);
		}
		void encode_float(int64_t p_offset, double p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_float(p_value, &w[p_offset]);
		}
		void encode_double(int64_t p_offset, double p_value) {
			if (sizeof(T) != 1) {
				return;
			}
			uint64_t _size = size();
			if (p_offset < 0 || p_offset > int64_t(_size) - 1) return;
			uint8_t* w = (uint8_t*)ptrw();
			CodeHelp::encode_double(p_value, &w[p_offset]);
		}


};

template <typename T>
void Vector<T>::reverse() {
	for (Size i = 0; i < size() / 2; i++) {
		T *p = ptrw();
		SWAP(p[i], p[size() - i - 1]);
	}
}

template <typename T>
void Vector<T>::append_array(Vector<T> p_other) {
	const Size ds = p_other.size();
	if (ds == 0) {
		return;
	}
	const Size bs = size();
	resize(bs + ds);
	for (Size i = 0; i < ds; ++i) {
		ptrw()[bs + i] = p_other[i];
	}
}

template <typename T>
bool Vector<T>::push_back(T p_elem) {
	Error err = resize(size() + 1);
	ERR_FAIL_COND_V(err, true);
	set(size() - 1, p_elem);

	return false;
}

template <typename T>
void Vector<T>::fill(T p_elem) {
	T *p = ptrw();
	for (Size i = 0; i < size(); i++) {
		p[i] = p_elem;
	}
}

#endif // VECTOR_H
