#pragma once

#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
// 内存分配器
class MemoryPoolAllocal : public RefCounted {
	GDCLASS(MemoryPoolAllocal, RefCounted);
	static void _bind_methods();

public:
	// 内存块结构体
	class Block {
	public:
		uint32_t offset = 0;
		uint32_t size = 0; // 内存块大小
		bool available = false; // 内存块是否可用（true：可用，false：不可用）
		Block *next = nullptr; // 指向链表中下一个内存块的指针
		static Block *s_freeBlock;
		static Mutex mutex;
		int start() {
			return offset;
		}
		int end() {
			return offset + size;
		}
		static Block *allocate() {
			mutex.lock();
			if (s_freeBlock != nullptr) {
				Block *r = s_freeBlock;
				s_freeBlock = s_freeBlock->next;
				mutex.unlock();
				r->next = nullptr;
				return r;
			}
			mutex.unlock();
			return memnew(Block);
		}
		static void free(Block *_block) {
			mutex.lock();
			_block->next = s_freeBlock;
			s_freeBlock = _block;
			mutex.unlock();
		}
	};

public:
	MemoryPoolAllocal() {
		m_totalMemorySize = 0;
		add_free_block(2048);
	}
	// 构造函数，初始化内存池
	MemoryPoolAllocal(int total_size) {
		m_totalMemorySize = 0;
		add_free_block(total_size);
	}

	// 析构函数，释放内存池和Block信息
	void release() {
		for (auto &entry : m_block_map) {
			Block::free(entry.value);
		}
		m_block_map.clear();
		m_total_Blocks.clear();
	}
	Block *add_free_block(int count = 256) {
		Block *first_block = Block::allocate();
		first_block->available = true;
		first_block->offset = m_totalMemorySize;
		first_block->size = count;
		m_block_map.insert(first_block->offset, first_block);
		m_total_Blocks.insert(first_block);
		m_totalMemorySize += count;

		return first_block;
	}

	// Best Fit 分配算法
	// 注意如果_auto_addcount 不为0，返回的区间有可能自动分配，超出当前的区间
	Block *allocate(uint32_t requested_size, uint32_t _auto_addcount = 0) {
		if (requested_size <= 0) {
			//Debug.LogError($"无法分配{requested_size}内存，数量非法！");
			return nullptr;
		}
		//Block previous = null;
		Block *best_fit_block = nullptr;
		Block *current_block = nullptr;

		uint32_t min_gap = 2147483647;

		// 遍历已释放内存块列表
		uint32_t current_gap;
		uint32_t remaining_size = 0;
		for (auto &entry : m_block_map) {
			current_block = entry.value;
			if (current_block->available && current_block->size >= requested_size) {
				current_gap = current_block->size - requested_size;

				if (current_gap < min_gap) {
					min_gap = current_gap;
					best_fit_block = current_block;
					remaining_size = current_gap;
				}
				// 找到了最佳大小
				if (current_gap == 0) {
					break;
				}
			}
		}

		// 若找到合适的内存块
		if (best_fit_block != nullptr) {
			// 若分配后剩余大小足够容纳一个新的内存块
			if (remaining_size > 0) {
				// 更新当前最佳匹配内存块的信息
				best_fit_block->size = requested_size;
				best_fit_block->available = false;

				// 在空闲链表中添加一个新的内存块
				Block *remaining_block = Block::allocate();
				remaining_block->offset = best_fit_block->offset + requested_size;
				// 设置大小为剩余大小
				remaining_block->size = remaining_size;
				remaining_block->available = true;
				remaining_block->next = nullptr;

				m_block_map.insert(remaining_block->offset, remaining_block);
				m_total_Blocks.insert(remaining_block);

			} else {
				// 标记已分配内存块为不可用
				best_fit_block->available = false;
			}
			return best_fit_block;
		}
		if (_auto_addcount > 0) {
			// 还没有找到，并且允许自动分配就新增一个内存块
			_auto_addcount = MAX(requested_size, _auto_addcount);
			add_free_block(_auto_addcount);
			return allocate(requested_size);
		}
		// 如果没有找到合适的内存块，返回nullptr
		return nullptr;
	}
	// 减少内存
	Block *reduction_memory(Block *block, uint32_t reduction_count) {
		if (reduction_count == 0) {
			return block;
		}
		if (reduction_count > block->size) {
			//Debug.LogError("你这是要掘地三尺的减少呀！");
			return block;
		}
		if (!m_block_map.has(block->start())) {
			//Debug.LogError("发现外来物种入侵，请弄死他！");
			return block;
		}

		Block *free = Block::allocate();
		block->size -= reduction_count;
		free->offset = block->end();
		free->size = reduction_count;
		free->available = false;
		m_block_map.insert(free->start(), free);
		m_total_Blocks.insert(free);
		return block;
	}
	void free_block(int block_offset) {
		auto it = m_block_map.find(block_offset);
		if (it != m_block_map.end()) {
			free_block(it->value);

		} else {
			//Debug.LogError("非法索引！block_offset 不存在！");
		}
	}

	// 释放内存块
	void free_block(Block *freed_block) {
		if (!m_total_Blocks.has(freed_block)) {
			return;
		}
		if (freed_block->available == true) {
			return;
		}
		freed_block->available = true;

		//Block current_block = freed_block;
		// 查找是否存在可以链接的下一个空闲区域
		//int next_memory = freed_block.end;
		Block *next_block_entry = nullptr;
		int nextKey = freed_block->end();
		auto it = m_block_map.find(nextKey);
		if (it != m_block_map.end()) {
			next_block_entry = it->value;
			Block *next_block = next_block_entry;
			if (next_block->available) {
				// 合并相邻的内存块
				freed_block->size += next_block->size;

				// 从映射表和堆内存中删除被合并的相邻内存块
				m_block_map.erase(nextKey);
				m_total_Blocks.erase(next_block);
				Block::free(next_block);
			}
		}

		Block *current_block = nullptr;
		// 合并上一个空白区域
		// 如果可用就自动合并
		for (auto &entry : m_block_map) {
			current_block = entry.value;
			// 判断是否是当前块的下一个节点是否为当前需要释放的节点
			if (current_block->end() == freed_block->start()) {
				if (!current_block->available) {
					// 不可用直接返回
					break;
				}

				current_block->size += freed_block->size;
				m_total_Blocks.erase(freed_block);
				m_block_map.erase(freed_block->offset);
				Block::free(freed_block);
				break;
			}
		}
	}
	uint64_t get_total_memory_size() {
		return m_totalMemorySize;
	}

public:
	uint64_t allocate_imp(uint32_t p_size) {
		Block *block = allocate(p_size);
		if (block == nullptr) {
			return 0;
		}
		return (uint64_t)block;
	}

	void free_imp(uint64_t p_block) {
		if (p_block == 0) {
			return;
		}
		Block *block = (Block *)p_block;
		free_block(block);
	}
	int find_block_size(uint64_t p_block) {
		auto it = m_block_map.find(p_block);
		if (it != m_block_map.end()) {
			return it->value->size;
		}
		return 0;
	}

public:
	static int get_block_size(uint64_t p_block) {
		if (p_block < 200) {
			// 非法值，不可能存在这么小的指针
			return 0;
		}
		Block *block = (Block *)p_block;
		return block->size;
	}
	static int get_block_offset(uint64_t p_block) {
		if (p_block < 200) {
			// 非法值，不可能存在这么小的指针
			return 0;
		}
		Block *block = (Block *)p_block;
		return block->offset;
	}

	static int get_block_end(uint64_t p_block) {
		if (p_block < 200) {
			// 非法值，不可能存在这么小的指针
			return 0;
		}
		Block *block = (Block *)p_block;
		return block->end();
	}

private:
	int m_totalMemorySize;
	HashMap<int, Block *> m_block_map; // 保存内存地址和Block信息的映射表
	HashSet<Block *> m_total_Blocks;
};
