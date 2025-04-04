// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include <Jolt/Core/Array.h>
#include <Jolt/Core/Mutex.h>
#include <Jolt/Core/NonCopyable.h>

JPH_NAMESPACE_BEGIN

// 自定义栈类
template <typename T>
class Stack {
private:
	Array<T *> data;

public:
	// 判断栈是否为空
	bool empty() const {
		return data.empty();
	}

	// 获取栈顶元素
	T *top() const {
		if (empty()) {
			return nullptr;
		}
		return data.back();
	}

	// 弹出栈顶元素
	void pop() {
		if (empty()) {
			return;
		}
		data.pop_back();
	}

	// 压入元素到栈顶
	void push(T *value) {
		data.push_back(value);
	}
	size_t size() {
		return data.size();
	}
};

// 定义对象池类
template <typename T, size_t MaxCacheCount>
class ObjectPool {
private:
	Stack<T> availableObjects;
	Mutex mutex;

public:
	// 构造函数，初始化对象池
	ObjectPool() {
	}

	// 析构函数，释放对象池中的所有对象
	~ObjectPool() {
		mutex.lock();
		while (!availableObjects.empty()) {
			T *obj = availableObjects.top();
			availableObjects.pop();
			delete obj;
		}
		mutex.unlock();
	}

	// 从对象池中获取一个对象
	T *acquireObject() {
		mutex.lock();
		if (availableObjects.empty()) {
			mutex.unlock();
			// 如果对象池为空，创建一个新对象
			T *obj = new T();
			return obj;
		} else {
			// 从对象池中取出一个可用对象
			T *obj = availableObjects.top();
			availableObjects.pop();
			mutex.unlock();
			return obj;
		}
	}

	// 将对象放回对象池
	void releaseObject(T *obj) {
		mutex.lock();
		if (availableObjects.size() >= MaxCacheCount) {
			mutex.unlock();
			delete obj;
			return;
		}
		availableObjects.push(obj);
		mutex.unlock();
	}
};

JPH_NAMESPACE_END
