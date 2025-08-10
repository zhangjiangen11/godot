#include "memory_pool_allocal.h"
MemoryPoolAllocal::Block *MemoryPoolAllocal::Block::s_freeBlock = nullptr;
Mutex MemoryPoolAllocal::Block::mutex;

void MemoryPoolAllocal::_bind_methods() {
	ClassDB::bind_method(D_METHOD("allocate", "count"), &MemoryPoolAllocal::allocate_imp);
	ClassDB::bind_method(D_METHOD("free_block", "block"), &MemoryPoolAllocal::free_imp);
	ClassDB::bind_method(D_METHOD("get_total_memory_size"), &MemoryPoolAllocal::get_total_memory_size);

	ClassDB::bind_static_method("MemoryPoolAllocal", D_METHOD("get_block_offset", "block"), &MemoryPoolAllocal::get_block_offset);
	ClassDB::bind_static_method("MemoryPoolAllocal", D_METHOD("get_block_end", "block"), &MemoryPoolAllocal::get_block_end);
	ClassDB::bind_static_method("MemoryPoolAllocal", D_METHOD("get_block_size", "block"), &MemoryPoolAllocal::get_block_size);
}