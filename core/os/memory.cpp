/**************************************************************************/
/*  memory.cpp                                                            */
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

#include "memory.h"
#include "mutex.h"

#include "core/templates/safe_refcount.h"

#include <stdlib.h>
#include <string.h>

void *operator new(size_t p_size, const char *p_description) {
	return Memory::alloc_static(p_size, false);
}

void *operator new(size_t p_size, void *(*p_allocfunc)(size_t p_size)) {
	return p_allocfunc(p_size);
}

#ifdef _MSC_VER
void operator delete(void *p_mem, const char *p_description) {
	CRASH_NOW_MSG("Call to placement delete should not happen.");
}

void operator delete(void *p_mem, void *(*p_allocfunc)(size_t p_size)) {
	CRASH_NOW_MSG("Call to placement delete should not happen.");
}

void operator delete(void *p_mem, void *p_pointer, size_t check, const char *p_description) {
	CRASH_NOW_MSG("Call to placement delete should not happen.");
}
#endif

#ifdef DEBUG_ENABLED
SafeNumeric<uint64_t> Memory::mem_usage;
SafeNumeric<uint64_t> Memory::max_usage;
#endif

#define SAMLL_MEMORY_MANAGER 0

#if SAMLL_MEMORY_MANAGER
template <int SIZE_COUNT>
struct SmallMemoryBuffer {
	uint64_t size_or_next = 0;
	uint8_t data[SIZE_COUNT];
};
template <int SIZE_COUNT>
struct SmallMemory {
	_FORCE_INLINE_ void *alloc_mem(int p_count) {
		{
			void *ret = nullptr;
			mutex.lock();
			if (buffers != nullptr) {
				SmallMemoryBuffer<SIZE_COUNT> *b = buffers;
				buffers = (SmallMemoryBuffer<SIZE_COUNT> *)b->size_or_next;
				ret = b->data;
				b->size_or_next = p_count;
			}
			mutex.unlock();
			if (ret != nullptr) {
				return ret;
			}
		}
		SmallMemoryBuffer<SIZE_COUNT> *b = (SmallMemoryBuffer<SIZE_COUNT> *)malloc(sizeof(SmallMemoryBuffer<SIZE_COUNT>));
		if (b == nullptr) {
			return nullptr;
		}
		b->size_or_next = p_count;
		return b->data;
	}
	_FORCE_INLINE_ void free_mem(void *p_ptr) {
		if (p_ptr == nullptr) {
			return;
		}

		uint8_t *mem = (uint8_t *)p_ptr;
		mem -= sizeof(uint64_t);
		if (free_count.get() > 500) {
			free(mem);
			return;
		}
		SmallMemoryBuffer<SIZE_COUNT> *b = (SmallMemoryBuffer<SIZE_COUNT> *)mem;
		mutex.lock();
		b->size_or_next = (uint64_t)buffers;
		buffers = b;
		mutex.unlock();
	}

	SystemMutex mutex;
	SmallMemoryBuffer<SIZE_COUNT> *buffers = nullptr;
	SafeNumeric<uint64_t> free_count;
};

struct SmallMemoryManager {
#define SAMLL_MEMORY_MEMBER(count) SmallMemory<count> small_memory_##count
#define SAMLL_MEMORY_MEMBER_BREAH(value, count)       \
	if (value <= count) {                             \
		return small_memory_##count.alloc_mem(count); \
	}
#define SAMLL_MEMORY_MEMBER_BREAH_FREE(ptr, value, count) \
	if (value <= count) {                                 \
		small_memory_##count.free_mem(ptr);               \
	}
#define SAMLL_MEMORY_MEMBER_BREAH_INDEX(value, count, index) \
	if (value <= count) {                                    \
		return index;                                        \
	}

	void *alloc_mem(int p_count) {
		if (p_count <= 0) {
			return nullptr;
		}
		SAMLL_MEMORY_MEMBER_BREAH(p_count, 20)
		else SAMLL_MEMORY_MEMBER_BREAH(p_count, 40) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 80) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 120) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 160) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 180) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 256) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 320) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 480) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 512) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 800) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 1024) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 1500) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 1800) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 2048) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 3000) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 5000) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 7000) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 8000) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 12000) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 15000) else SAMLL_MEMORY_MEMBER_BREAH(p_count, 20000) else {
			uint8_t *mem = (uint8_t *)malloc(p_count + sizeof(uint64_t));
			if (mem == nullptr) {
				return nullptr;
			}
			*(uint64_t *)mem = p_count;
			return mem + sizeof(uint64_t);
		}
	}
	void free_mem(void *p_ptr) {
		if (p_ptr == nullptr) {
			return;
		}
		uint64_t p_count = get_buffer_count(p_ptr);
		SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 20)
		else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 40) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 80) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 120) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 160) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 180) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 256) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 320) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 480) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 512) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 800) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 1024) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 1500) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 1800) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 2048) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 3000) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 5000) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 7000) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 8000) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 12000) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 15000) else SAMLL_MEMORY_MEMBER_BREAH_FREE(p_ptr, p_count, 20000) else {
			uint8_t *mem = (uint8_t *)p_ptr - sizeof(uint64_t);
			free(mem);
		}
	}
	void *realloc_mem(void *p_memory, size_t p_new_bytes) {
		if (p_memory == nullptr) {
			return alloc_mem(p_new_bytes);
		}
		//uint8_t *mem = (uint8_t *)p_memory;
		if (p_new_bytes == 0) {
			free_mem(p_memory);
			return nullptr;
		} else {
			auto old_count = get_buffer_count(p_memory);

			int index = get_buffer_type_index(old_count);
			if (index >= 0 && index == get_buffer_type_index(p_new_bytes)) {
				return p_memory;
			}
			void *new_mem = alloc_mem(p_new_bytes);
			memcpy(new_mem, p_memory, MIN(old_count, p_new_bytes));
			free_mem(p_memory);
			return new_mem;
		}
	}
	_FORCE_INLINE_ uint64_t get_buffer_count(void *p_ptr) {
		uint8_t *mem = (uint8_t *)p_ptr - sizeof(uint64_t);
		return *(uint64_t *)(mem);
	}
	_FORCE_INLINE_ int get_buffer_type_index(void *p_ptr) {
		uint8_t *mem = (uint8_t *)p_ptr - sizeof(uint64_t);
		uint64_t count = *(uint64_t *)(mem);
		return get_buffer_type_index(count);
	}

	int get_buffer_type_index(int p_count) {
		SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 20, 0)
		else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 40, 1) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 80, 2) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 120, 3) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 160, 4) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 180, 5) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 256, 6) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 320, 7) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 480, 8) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 512, 9) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 800, 10) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 1024, 11) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 1500, 12) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 1800, 13) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 2048, 14) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 3000, 15) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 5000, 16) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 7000, 17) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 8000, 18) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 12000, 19) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 15000, 20) else SAMLL_MEMORY_MEMBER_BREAH_INDEX(p_count, 20000, 21)

				return -1;
	}

	SAMLL_MEMORY_MEMBER(20);
	SAMLL_MEMORY_MEMBER(40);
	SAMLL_MEMORY_MEMBER(80);
	SAMLL_MEMORY_MEMBER(120);
	SAMLL_MEMORY_MEMBER(160);
	SAMLL_MEMORY_MEMBER(180);
	SAMLL_MEMORY_MEMBER(256);
	SAMLL_MEMORY_MEMBER(320);
	SAMLL_MEMORY_MEMBER(480);
	SAMLL_MEMORY_MEMBER(512);
	SAMLL_MEMORY_MEMBER(800);
	SAMLL_MEMORY_MEMBER(1024);
	SAMLL_MEMORY_MEMBER(1500);
	SAMLL_MEMORY_MEMBER(1800);
	SAMLL_MEMORY_MEMBER(2048);
	SAMLL_MEMORY_MEMBER(3000);
	SAMLL_MEMORY_MEMBER(5000);
	SAMLL_MEMORY_MEMBER(7000);
	SAMLL_MEMORY_MEMBER(8000);
	SAMLL_MEMORY_MEMBER(12000);
	SAMLL_MEMORY_MEMBER(15000);
	SAMLL_MEMORY_MEMBER(20000);

#undef SAMLL_MEMORY_MEMBER
#undef SAMLL_MEMORY_MEMBER_BREAH
#undef SAMLL_MEMORY_MEMBER_BREAH_FREE
#undef SAMLL_MEMORY_MEMBER_BREAH_INDEX
};

static _FORCE_INLINE_ SmallMemoryManager &get_small_memory_manager() {
	static SmallMemoryManager *s_manager = new (malloc(sizeof(SmallMemoryManager))) SmallMemoryManager();
	return *s_manager;
}
#endif

void *Memory::alloc_aligned_static(size_t p_bytes, size_t p_alignment) {
#if SAMLL_MEMORY_MANAGER
	return get_small_memory_manager().alloc_mem(p_bytes);
#endif
	DEV_ASSERT(is_power_of_2(p_alignment));

	void *p1, *p2;
	if ((p1 = (void *)malloc(p_bytes + p_alignment - 1 + sizeof(uint32_t))) == nullptr) {
		return nullptr;
	}

	p2 = (void *)(((uintptr_t)p1 + sizeof(uint32_t) + p_alignment - 1) & ~((p_alignment)-1));
	*((uint32_t *)p2 - 1) = (uint32_t)((uintptr_t)p2 - (uintptr_t)p1);
	return p2;
}

void *Memory::realloc_aligned_static(void *p_memory, size_t p_bytes, size_t p_prev_bytes, size_t p_alignment) {
#if SAMLL_MEMORY_MANAGER
	return get_small_memory_manager().realloc_mem(p_memory, p_bytes);
#endif

	if (p_memory == nullptr) {
		return alloc_aligned_static(p_bytes, p_alignment);
	}

	void *ret = alloc_aligned_static(p_bytes, p_alignment);
	if (ret) {
		memcpy(ret, p_memory, p_prev_bytes);
	}
	free_aligned_static(p_memory);
	return ret;
}

void Memory::free_aligned_static(void *p_memory) {
#if SAMLL_MEMORY_MANAGER
	get_small_memory_manager().free_mem(p_memory);
	return;
#endif
	uint32_t offset = *((uint32_t *)p_memory - 1);
	void *p = (void *)((uint8_t *)p_memory - offset);
	free(p);
}

void *Memory::alloc_static(size_t p_bytes, bool p_pad_align) {
#if SAMLL_MEMORY_MANAGER
	return get_small_memory_manager().alloc_mem(p_bytes);
#endif
#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	void *mem = malloc(p_bytes + (prepad ? DATA_OFFSET : 0));

	ERR_FAIL_NULL_V(mem, nullptr);

	if (prepad) {
		uint8_t *s8 = (uint8_t *)mem;

		uint64_t *s = (uint64_t *)(s8 + SIZE_OFFSET);
		*s = p_bytes;

#ifdef DEBUG_ENABLED
		uint64_t new_mem_usage = mem_usage.add(p_bytes);
		max_usage.exchange_if_greater(new_mem_usage);
#endif
		return s8 + DATA_OFFSET;
	} else {
		return mem;
	}
}

void *Memory::realloc_static(void *p_memory, size_t p_bytes, bool p_pad_align) {
#if SAMLL_MEMORY_MANAGER
	return get_small_memory_manager().realloc_mem(p_memory, p_bytes);
#endif
	if (p_memory == nullptr) {
		return alloc_static(p_bytes, p_pad_align);
	}

	uint8_t *mem = (uint8_t *)p_memory;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= DATA_OFFSET;
		uint64_t *s = (uint64_t *)(mem + SIZE_OFFSET);

#ifdef DEBUG_ENABLED
		if (p_bytes > *s) {
			uint64_t new_mem_usage = mem_usage.add(p_bytes - *s);
			max_usage.exchange_if_greater(new_mem_usage);
		} else {
			mem_usage.sub(*s - p_bytes);
		}
#endif

		if (p_bytes == 0) {
			free(mem);
			return nullptr;
		} else {
			*s = p_bytes;

			mem = (uint8_t *)realloc(mem, p_bytes + DATA_OFFSET);
			ERR_FAIL_NULL_V(mem, nullptr);

			s = (uint64_t *)(mem + SIZE_OFFSET);

			*s = p_bytes;

			return mem + DATA_OFFSET;
		}
	} else {
		mem = (uint8_t *)realloc(mem, p_bytes);

		ERR_FAIL_COND_V(mem == nullptr && p_bytes > 0, nullptr);

		return mem;
	}
}

void Memory::free_static(void *p_ptr, bool p_pad_align) {
#if SAMLL_MEMORY_MANAGER
	get_small_memory_manager().free_mem(p_ptr);
	return;
#endif
	ERR_FAIL_NULL(p_ptr);

	uint8_t *mem = (uint8_t *)p_ptr;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= DATA_OFFSET;

#ifdef DEBUG_ENABLED
		uint64_t *s = (uint64_t *)(mem + SIZE_OFFSET);
		mem_usage.sub(*s);
#endif

		free(mem);
	} else {
		free(mem);
	}
}

uint64_t Memory::get_mem_available() {
	return -1; // 0xFFFF...
}

uint64_t Memory::get_mem_usage() {
#ifdef DEBUG_ENABLED
	return mem_usage.get();
#else
	return 0;
#endif
}

uint64_t Memory::get_mem_max_usage() {
#ifdef DEBUG_ENABLED
	return max_usage.get();
#else
	return 0;
#endif
}

_GlobalNil::_GlobalNil() {
	left = this;
	right = this;
	parent = this;
}

_GlobalNil _GlobalNilClass::_nil;
