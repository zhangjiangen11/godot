#pragma once
#include "core/object/ref_counted.h"
#include <atomic>
template <typename T>
struct SafeArray : public RefCounted {
	GDSOFTCLASS(SafeArray, RefCounted)
public:
	T *data = nullptr;
	std::atomic<int64_t> listCount;

	int64_t memorySize = 0;
	bool isSafeAlocal = false;
	int64_t size() const {
		return MIN(listCount.load(std::memory_order_acquire), memorySize);
	}
	SafeArray(int64_t memSize = 0) {
		data = nullptr;
		listCount.store(0, std::memory_order_release);
		memorySize = 0;
		isSafeAlocal = false;
		if (memSize > 0) {
			auto_resize(memSize, memSize);
		}
	}
	~SafeArray() {
		dispose();
	}
	void set_memory(T *memory, int64_t offset, int64_t list_count, int64_t memory_size) {
		data = memory + offset;
		listCount.store(list_count, std::memory_order_release);
		memorySize = memory_size;
		isSafeAlocal = false;
	}
	void init(int64_t memSize = 0) {
		data = nullptr;
		listCount.store(0, std::memory_order_release);
		memorySize = 0;
		isSafeAlocal = false;
		if (memSize > 0) {
			auto_resize(memSize, memSize);
		}
	}
	/// <summary>
	/// 自动重置大小
	/// </summary>
	void auto_resize(int64_t length, int64_t next_length) {
		if (memorySize < length) {
			if (next_length < length) {
				//Debug.LogError("参数设置错误，next_length 要大于length ！");
				next_length = length;
			}

			long newsize = sizeof(T) * (long)next_length;
			auto new_data = memalloc(newsize);
			if (new_data == nullptr) {
				return;
			}
			memset(new_data, 0, newsize);
			if (data != nullptr) {
				memcpy(new_data, get_unsafe_ptr(), sizeof(T) * listCount.load(std::memory_order_acquire));
				memfree(data);
			}
			memorySize = next_length;
			data = (T *)new_data;
			isSafeAlocal = true;
		}
	}
	T *get_unsafe_ptr(int64_t index = 0) {
		if (index > listCount.load(std::memory_order_acquire)) {
			//Debug.LogError($"参数获取错误，index[{index}] 要小于listCount[{listCount}]！");
			return nullptr;
		}
		return &data[index];
	}
	T &operator[](int64_t index) {
		if (index >= listCount.load(std::memory_order_acquire)) {
			//Debug.LogError($"参数获取错误，index[{index}] 要小于listCount[{listCount}]");
			return data[0];
		}
		return data[index];
	}
	const T &operator[](int64_t index) const {
		if (index >= listCount.load(std::memory_order_acquire)) {
			//Debug.LogError($"参数获取错误，index[{index}] 要小于listCount[{listCount}]");
			return data[0];
		}
		return data[index];
	}
	void set_value(int64_t index, const T &value) {
		if (index > listCount.load(std::memory_order_acquire)) {
			//Debug.LogError($"参数设置错误，index[{index}] 要小于listCount[{listCount}]！");
			return;
		}
		data[index] = value;
	}
	void remove_range(int64_t start, int count) {
		if (count <= 0 || start < 0) {
			//Debug.LogError($"小贼别跑，这里有毒：{Length} start:{start} count:{count}！");
			return;
		}
		if (start + count > size()) {
			//Debug.LogError($"移除的数据太多了，当前长度：{Length} start:{start} count:{count}！");
			listCount.store(start, std::memory_order_release);
			return;
		} else if (start + count == size()) {
			listCount.store(start, std::memory_order_release);
			return;
		}
		//int bs = start + count;
		//int index = 0;
		//for(int i = bs; i < listCount; ++i,++index)
		//{
		//	UnsafeUtility.MemCpy(&data[start + index], &data[bs + index], UnsafeUtility.SizeOf<T>());
		//}
		int64_t last_index = start + count;
		int64_t copy_count = listCount.load(std::memory_order_acquire) - last_index;
		T *temp = (T *)alloca(sizeof(T) * copy_count);
		memcpy(temp, &data[last_index], sizeof(T) * copy_count);
		memcpy(&data[start], temp, sizeof(T) * copy_count);
	}
	/// <summary>
	/// 获取指针地址
	/// </summary>
	/// <returns></returns>
	void *get_unsafe_readonly_ptr(int64_t index = 0) {
		return get_unsafe_ptr(index);
	}
	void zero_memory_value() {
		auto ptr = get_unsafe_ptr();
		if (ptr != nullptr) {
			memset(ptr, 0, sizeof(T) * listCount.load(std::memory_order_acquire));
		}
	}
	void memset_value(uint8_t value) {
		auto ptr = get_unsafe_ptr();
		if (ptr != nullptr) {
			memset(ptr, 0, sizeof(T) * listCount.load(std::memory_order_acquire));
		}
	}
	void add_ref(T &_data) {
		int64_t last_count = listCount.load(std::memory_order_acquire);
		auto_resize(last_count + 1, last_count + 1 + 200);

		memcpy(&data[last_count], &_data, sizeof(T));
		listCount.fetch_add(1, std::memory_order_release);
	}
	void thread_add(T &_data) {
		auto idx = listCount.fetch_add(1, std::memory_order_acq_rel) - 1;
		if (idx < memorySize) {
			memcpy(&data[idx], &_data, sizeof(T));
		}
	}
	void thread_add_range(T *_data, int64_t count) {
		auto idx = listCount.fetch_add(count, std::memory_order_acq_rel) - count;
		if (idx + count <= memorySize) {
			memcpy(&data[idx], _data, sizeof(T) * count);
		}
	}
	T *thread_add_range_empty(int64_t count, uint64_t *p_old_index = nullptr) {
		auto idx = listCount.fetch_add(count, std::memory_order_acq_rel) - count;
		if (p_old_index != nullptr) {
			*p_old_index = idx;
		}
		if (idx + count <= memorySize) {
			return &data[idx];
		}
		return nullptr;
	}

	void add(T _data) {
		int last_count = listCount.load(std::memory_order_acquire);
		auto_resize(last_count + 1, last_count + 1 + 200);

		memcpy(&data[last_count], &_data, sizeof(T));
		listCount.fetch_add(1, std::memory_order_release);
	}
	void add_range(T *_data, int64_t count) {
		int last_count = listCount.load(std::memory_order_acquire);
		auto_resize(last_count + count, last_count + count + 200);

		memcpy(&data[last_count], _data, sizeof(T) * count);
		listCount.fetch_add(count, std::memory_order_release);
	}
	/// <summary>
	/// Sets the length of this list, increasing the capacity if necessary.
	/// </summary>
	/// <remarks>Does not clear newly allocated bytes.</remarks>
	/// <param name="length">The new length of this list.</param>
	void resize_uninitialized(int64_t length, int64_t addCount = 0) {
		auto_resize(length, length + addCount);
		listCount.store(length, std::memory_order_release);
	}
	void compact_memory() {
		if (data != nullptr) {
			if (isSafeAlocal) {
				memfree(data);
			}
			data = nullptr;
			listCount.store(0, std::memory_order_release);
			memorySize = 0;
		}
	}
	void clear() {
		listCount.store(0, std::memory_order_release);
	}
	void dispose() {
		if (data != nullptr) {
			if (isSafeAlocal) {
				memfree(data);
			}
			data = nullptr;
			listCount.store(0, std::memory_order_release);
			memorySize = 0;
		}
	}
};
