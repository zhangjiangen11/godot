#pragma once
#include "core/os/memory.h"
#include "core/typedefs.h"

template <typename T>
class Stack {
	struct StackNode {
		StackNode *next = nullptr;
		T data;
	};

public:
	_FORCE_INLINE_ bool top(T &v) const {
		StackNode *n = nullptr;
		if (first != nullptr) {
			v = first->data;
			return true;
		}
		return false;
	}
	_FORCE_INLINE_ bool empty() {
		return node_count == 0U;
	}
	_FORCE_INLINE_ uint32_t size() const {
		return node_count;
	}
	_FORCE_INLINE_ void push(const T &v) {
		StackNode *n = nullptr;
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
		++node_count;
	}
	_FORCE_INLINE_ bool pop(T &v) {
		bool r = false;
		if (first != nullptr) {
			v = first->data;
			first = first->next;
			if (first == nullptr) {
				last = nullptr;
			}
			r = true;
		}
		--node_count;
		return r;
	}
	void clear() {
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
		node_count = 0;
	}
	Stack() {
	}
	Stack(const SafeStack &) = delete;
	bool operator==((const Stack &) const = delete;
	bool operator!=((const Stack &) const = delete;

	~Stack() {
		clear();
	}

	StackNode *first = nullptr;
	StackNode *last = nullptr;
	StackNode *free_list = nullptr;
	uint32_t node_count = 0;
};
