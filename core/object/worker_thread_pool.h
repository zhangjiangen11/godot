/**************************************************************************/
/*  worker_thread_pool.h                                                  */
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

#pragma once

#include "core/os/condition_variable.h"
#include "core/os/memory.h"
#include "core/os/os.h"
#include "core/os/semaphore.h"
#include "core/os/thread.h"
#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"
#include "core/templates/rid.h"
#include "core/templates/safe_refcount.h"

class WorkerThreadPool : public Object {
	GDCLASS(WorkerThreadPool, Object)
public:
	enum {
		INVALID_TASK_ID = -1
	};

	typedef int64_t TaskID;
	typedef int64_t GroupID;

private:
	struct Task;

	struct BaseTemplateUserdata {
		virtual void callback() {}
		virtual void callback_indexed(uint32_t p_index) {}
		virtual ~BaseTemplateUserdata() {}
	};

	struct Group {
		GroupID self = -1;
		SafeNumeric<uint32_t> index;
		SafeNumeric<uint32_t> completed_index;
		uint32_t max = 0;
		Semaphore done_semaphore;
		SafeFlag completed;
		SafeNumeric<uint32_t> finished;
		uint32_t tasks_used = 0;
	};

	struct Task {
		TaskID self = -1;
		Callable callable;
		void (*native_func)(void *) = nullptr;
		void (*native_group_func)(void *, uint32_t) = nullptr;
		void *native_func_userdata = nullptr;
		String description;
		Semaphore done_semaphore; // For user threads awaiting.
		bool completed : 1;
		bool pending_notify_yield_over : 1;
		Group *group = nullptr;
		SelfList<Task> task_elem;
		uint32_t waiting_pool = 0;
		uint32_t waiting_user = 0;
		bool low_priority = false;
		BaseTemplateUserdata *template_userdata = nullptr;
		int pool_thread_index = -1;

		void free_template_userdata();
		Task() :
				completed(false),
				pending_notify_yield_over(false),
				task_elem(this) {}
	};

	static const uint32_t TASKS_PAGE_SIZE = 1024;
	static const uint32_t GROUPS_PAGE_SIZE = 256;
	String thread_name;
	PagedAllocator<Task, false, TASKS_PAGE_SIZE> task_allocator;
	PagedAllocator<Group, false, GROUPS_PAGE_SIZE> group_allocator;

	SelfList<Task>::List low_priority_task_queue;
	SelfList<Task>::List task_queue;

	BinaryMutex task_mutex;

	struct ThreadData {
		static Task *const YIELDING; // Too bad constexpr doesn't work here.

		uint32_t index = 0;
		Thread thread;
		bool signaled : 1;
		bool yield_is_over : 1;
		bool pre_exited_languages : 1;
		bool exited_languages : 1;
		Task *current_task = nullptr;
		Task *awaited_task = nullptr; // Null if not awaiting the condition variable, or special value (YIELDING).
		ConditionVariable cond_var;
		WorkerThreadPool *pool = nullptr;

		ThreadData() :
				signaled(false),
				yield_is_over(false),
				pre_exited_languages(false),
				exited_languages(false) {}
	};

	TightLocalVector<ThreadData> threads;
	enum Runlevel {
		RUNLEVEL_NORMAL,
		RUNLEVEL_PRE_EXIT_LANGUAGES, // Block adding new tasks
		RUNLEVEL_EXIT_LANGUAGES, // All threads detach from scripting threads.
		RUNLEVEL_EXIT,
	} runlevel = RUNLEVEL_NORMAL;
	union { // Cleared on every runlevel change.
		struct {
			uint32_t num_idle_threads;
		} pre_exit_languages;
		struct {
			uint32_t num_exited_threads;
		} exit_languages;
	} runlevel_data;
	ConditionVariable control_cond_var;

	HashMap<Thread::ID, int> thread_ids;
	HashMap<
			TaskID,
			Task *,
			HashMapHasherDefault,
			HashMapComparatorDefault<TaskID>,
			PagedAllocator<HashMapElement<TaskID, Task *>, false, TASKS_PAGE_SIZE>>
			tasks;
	HashMap<
			GroupID,
			Group *,
			HashMapHasherDefault,
			HashMapComparatorDefault<GroupID>,
			PagedAllocator<HashMapElement<GroupID, Group *>, false, GROUPS_PAGE_SIZE>>
			groups;

	uint32_t max_low_priority_threads = 0;
	uint32_t low_priority_threads_used = 0;
	uint32_t notify_index = 0; // For rotating across threads, no help distributing load.

	uint64_t last_task = 1;

	static HashMap<StringName, WorkerThreadPool *> named_pools;

	static void _thread_function(void *p_user);

	void _process_task(Task *task);

	void _post_tasks(Task **p_tasks, uint32_t p_count, bool p_high_priority, MutexLock<BinaryMutex> &p_lock);
	void _notify_threads(const ThreadData *p_current_thread_data, uint32_t p_process_count, uint32_t p_promote_count);

	bool _try_promote_low_priority_task();

	static WorkerThreadPool *singleton;

#ifdef THREADS_ENABLED
	static const uint32_t MAX_UNLOCKABLE_LOCKS = 2;
	struct UnlockableLocks {
		THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> *ulock = nullptr;
		uint32_t rc = 0;
	};
	static thread_local UnlockableLocks unlockable_locks[MAX_UNLOCKABLE_LOCKS];
#endif

	TaskID _add_task(const Callable &p_callable, void (*p_func)(void *), void *p_userdata, BaseTemplateUserdata *p_template_userdata, bool p_high_priority, const String &p_description);
	GroupID _add_group_task(const Callable &p_callable, void (*p_func)(void *, uint32_t), void *p_userdata, BaseTemplateUserdata *p_template_userdata, int p_elements, int p_tasks, bool p_high_priority, const String &p_description);

	template <typename C, typename M, typename U>
	struct TaskUserData : public BaseTemplateUserdata {
		C *instance;
		M method;
		U userdata;
		virtual void callback() override {
			(instance->*method)(userdata);
		}
	};

	template <typename C, typename M, typename U>
	struct GroupUserData : public BaseTemplateUserdata {
		C *instance;
		M method;
		U userdata;
		virtual void callback_indexed(uint32_t p_index) override {
			(instance->*method)(p_index, userdata);
		}
	};

	void _wait_collaboratively(ThreadData *p_caller_pool_thread, Task *p_task);

	void _switch_runlevel(Runlevel p_runlevel);
	bool _handle_runlevel(ThreadData *p_thread_data, MutexLock<BinaryMutex> &p_lock);

#ifdef THREADS_ENABLED
	static uint32_t _thread_enter_unlock_allowance_zone(THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> &p_ulock);
#endif

	void _lock_unlockable_mutexes();
	void _unlock_unlockable_mutexes();

protected:
	static void _bind_methods();

public:
	template <typename C, typename M, typename U>
	TaskID add_template_task(C *p_instance, M p_method, U p_userdata, bool p_high_priority = false, const String &p_description = String()) {
		typedef TaskUserData<C, M, U> TUD;
		TUD *ud = memnew(TUD);
		ud->instance = p_instance;
		ud->method = p_method;
		ud->userdata = p_userdata;
		return _add_task(Callable(), nullptr, nullptr, ud, p_high_priority, p_description);
	}
	TaskID add_native_task(void (*p_func)(void *), void *p_userdata, bool p_high_priority = false, const String &p_description = String());
	TaskID add_task(const Callable &p_action, bool p_high_priority = false, const String &p_description = String());

	bool is_task_completed(TaskID p_task_id) const;
	Error wait_for_task_completion(TaskID p_task_id);

	void yield();
	void notify_yield_over(TaskID p_task_id);

	template <typename C, typename M, typename U>
	GroupID add_template_group_task(C *p_instance, M p_method, U p_userdata, int p_elements, int p_tasks = -1, bool p_high_priority = false, const String &p_description = String()) {
		typedef GroupUserData<C, M, U> GroupUD;
		GroupUD *ud = memnew(GroupUD);
		ud->instance = p_instance;
		ud->method = p_method;
		ud->userdata = p_userdata;
		return _add_group_task(Callable(), nullptr, nullptr, ud, p_elements, p_tasks, p_high_priority, p_description);
	}
	GroupID add_native_group_task(void (*p_func)(void *, uint32_t), void *p_userdata, int p_elements, int p_tasks = -1, bool p_high_priority = false, const String &p_description = String());
	GroupID add_group_task(const Callable &p_action, int p_elements, int p_tasks = -1, bool p_high_priority = false, const String &p_description = String());
	uint32_t get_group_processed_element_count(GroupID p_group) const;
	bool is_group_task_completed(GroupID p_group) const;
	void wait_for_group_task_completion(GroupID p_group);

	_FORCE_INLINE_ int get_thread_count() const {
#ifdef THREADS_ENABLED
		return threads.size();
#else
		return 1;
#endif
	}

	// Note: Do not use this unless you know what you are doing, and it is absolutely necessary. Main thread pool (`get_singleton()`) should be preferred instead.
	static WorkerThreadPool *get_named_pool(const StringName &p_name);

	static WorkerThreadPool *get_singleton() { return singleton; }
	int get_thread_index() const;
	TaskID get_caller_task_id() const;

#ifdef THREADS_ENABLED
	_ALWAYS_INLINE_ static uint32_t thread_enter_unlock_allowance_zone(const MutexLock<BinaryMutex> &p_lock) { return _thread_enter_unlock_allowance_zone(p_lock._get_lock()); }
	template <int Tag>
	_ALWAYS_INLINE_ static uint32_t thread_enter_unlock_allowance_zone(const SafeBinaryMutex<Tag> &p_mutex) { return _thread_enter_unlock_allowance_zone(p_mutex._get_lock()); }
	static void thread_exit_unlock_allowance_zone(uint32_t p_zone_id);
#else
	static uint32_t thread_enter_unlock_allowance_zone(const MutexLock<BinaryMutex> &p_lock) { return UINT32_MAX; }
	template <int Tag>
	static uint32_t thread_enter_unlock_allowance_zone(const SafeBinaryMutex<Tag> &p_mutex) { return UINT32_MAX; }
	static void thread_exit_unlock_allowance_zone(uint32_t p_zone_id) {}
#endif

	void init(int p_thread_count = -1, float p_low_priority_task_ratio = 0.3);
	void exit_languages_threads();
	void finish();
	WorkerThreadPool(const String & name,bool p_singleton = true);
	~WorkerThreadPool();
};

/*=================================================================WorkerTaskPool==================================================================*/
class TaskJobHandle;
// 多任务管理器
class WorkerTaskPool : public Object {
	GDCLASS(WorkerTaskPool, Object)
	static void _bind_methods();
	static WorkerTaskPool *singleton;
public:

	struct ThreadRunStack {
		uint32_t thread_index;
		StringName task_name;
		uint32_t task_start;
		uint32_t task_end;
		// 毫秒ms
		double tast_start_time;
		double task_end_time;
		ThreadRunStack* next = nullptr;
	};
protected:
	bool exit_threads = false;
	class ThreadTaskGroup* task_queue = nullptr;
	// 释放的列队
	class ThreadTaskGroup* free_queue = nullptr;
	Mutex task_mutex;
	Mutex free_mutex;
	Semaphore task_available_semaphore;

	struct ThreadData {
		uint32_t index;
		Thread thread;
	};
	TightLocalVector<ThreadData> threads;


	Mutex stack_mutex;
	LocalVector<ThreadRunStack> task_run_stack_pool;
	ThreadRunStack* first_stack = nullptr;
	ThreadRunStack* last_stack = nullptr;
	int stack_use_count = 0;
	bool capture_stack = false;

	void push_task_stack(int thread_index,const StringName& task_name,uint32_t task_start,uint32_t task_end, double task_start_time,double task_end_time);
	void reset_task_stack();
	void get_task_stack_data(LocalVector<ThreadRunStack> & stack);

public:
	template <typename... VarArgs>
	_FORCE_INLINE_ Ref<TaskJobHandle> add_group_task_bind(const StringName& _task_name,const Callable &p_action, int p_elements_count, int _batch_count,TaskJobHandle* depend_task,VarArgs... p_args) {
		return add_group_task(p_action.bind(p_args...), p_elements_count, _batch_count, depend_task);
	}
	template <typename... VarArgs>
	_FORCE_INLINE_ Ref<TaskJobHandle> add_task_bind(const StringName& _task_name,const Callable &p_action, TaskJobHandle* depend_task,VarArgs... p_args) {
		return add_group_task(p_action.bind(p_args...), 1, 1, depend_task);
	}

public:
	Ref<TaskJobHandle> add_native_group_task(const StringName& _task_name,void (*p_func)(void *, uint32_t), void *p_userdata, int p_elements_count,int _batch_count,TaskJobHandle* depend_task);
	Ref<TaskJobHandle> add_group_task(const StringName& _task_name,const Callable &p_action, int p_elements, int _batch_count,TaskJobHandle* depend_task);

public:
	Ref<TaskJobHandle> combined_job_handle(TypedArray<TaskJobHandle> _handles );
public:
	
	static WorkerTaskPool *get_singleton() { return singleton; }

	void init();
	void finish();


	WorkerTaskPool();
	~WorkerTaskPool();
protected:
	static void _thread_task_function(void *p_user);
	void _process_task_queue(int thread_id) ;
	class ThreadTaskGroup * allocal_task();
	void free_task(class ThreadTaskGroup * task);
	void add_task(class ThreadTaskGroup * task);
	friend class ThreadTaskGroup;
private:

};
/*=================================================================TaskJobHandle==================================================================*/
// 任务Job句柄
class TaskJobHandle : public RefCounted
{
	GDCLASS(TaskJobHandle, RefCounted);
	static void _bind_methods();
	friend class WorkerTaskPool;
	friend class ThreadTaskGroup;
 public:
 	// 新建下一个组任务
	template <typename... VarArgs>
	_FORCE_INLINE_ Ref<TaskJobHandle> new_next_group_t(const StringName& _task_name,const Callable &p_action, int p_elements_count, int _batch_count,VarArgs... p_args) {
		return add_group_task(_task_name,p_action.bind(p_args...), p_elements_count, _batch_count, this);
	}

	// 新建下一个任务
	template <typename... VarArgs>
	_FORCE_INLINE_ Ref<TaskJobHandle> new_next_t(const StringName& _task_name,const Callable &p_action, VarArgs... p_args) {
		return add_group_task(_task_name,p_action.bind(p_args...), 1, 1, this);
	}

public:
	// 新建下一个组任务
   Ref<TaskJobHandle> new_next_group(const StringName& _task_name,const Callable &p_action, int p_elements_count, int _batch_count) {
		return WorkerTaskPool::get_singleton()->add_group_task(_task_name,p_action, p_elements_count, _batch_count, this);
   }

   // 新建下一个任务
   Ref<TaskJobHandle> new_next(const StringName& _task_name,const Callable &p_action) {
	   return WorkerTaskPool::get_singleton()->add_group_task(_task_name,p_action, 1, 1, this);
   }

	// 添加依赖，这个句柄,必须是自己心分配的,不能复用
	void push_depend(TaskJobHandle* depend_task) {
		ERR_FAIL_COND(is_init);
		dependJob.push_back(depend_task);
	}
	bool is_completed() ;
	// 等待信号完成
	void wait_completion();

 protected:
 	void init(){
 		is_init = true;
 	}
	void set_task_completed(int count);
	void set_completed();
	// 等待所有依赖信号完成
	void wait_depend_completion();

	
 protected:
	// 任务名称
	StringName task_name;
	Mutex depend_mutex;
	// 依赖的句柄
	LocalVector<Ref<TaskJobHandle>> dependJob;
	// 完成标志
	mutable THREADING_NAMESPACE::mutex done_mutex;
	std::condition_variable cv;
	SafeFlag completed;
	// 完成数量
	SafeNumeric<uint32_t> completed_index;
	// 最大任务数量
	uint32_t taskMax = 0;
	bool is_init = false;
	bool is_error = false;
	bool is_job = false;
	String error_string;
};
