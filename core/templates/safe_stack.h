#pragma once
#include "core/os/memory.h"
#include "core/typedefs.h"

//

#include "core/os/mutex.h"
template <typename T>
class SafeStack {
	struct StackNode {
		StackNode *next = nullptr;
		T data;
	};

public:
	_FORCE_INLINE_ void push(const T &v) {
		StackNode *n = nullptr;
		mutex.lock();
		if (free_list == nullptr) {
			n = memnew(StackNode);
		} else {
			n = free_list;
			free_list = free_list->next;
		}
		n->data = v;
		if (first == nullptr) {
			first = n;
			last = n;
		} else {
			last->next = n;
			last = n;
		}
		mutex.unlock();
	}
	_FORCE_INLINE_ bool pop(T &v) {
		bool r = false;
		mutex.lock();
		if (first != nullptr) {
			v = first->data;
			first = first->next;
			if (first == nullptr) {
				last = nullptr;
			}
			r = true;
		}
		mutex.unlock();
		return r;
	}
	void clear() {
		mutex.lock();
		while (first != nullptr) {
			StackNode *n = first;
			first = first->next;
			memdelete(n);
		}
		while (free_list != nullptr) {
			StackNode *n = free_list;
			free_list = free_list->next;
			memdelete(n);
		}
		first = nullptr;
		last = nullptr;
		free_list = nullptr;
		mutex.unlock();
	}
	~SafeStack() {
		clear();
	}

	Mutex mutex;
	StackNode *first = nullptr;
	StackNode *last = nullptr;
	StackNode *free_list = nullptr;
};
