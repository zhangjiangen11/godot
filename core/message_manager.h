#pragma once
#include "core/object/ref_counted.h"
#include "core/templates/safe_stack.h"
#ifndef MessageManager
#define MessageManager _ID678DPYU4872
#endif

#define register_message _id3766592
#define unregister_message _id376rghj592

#define register_enum_message _id3df766595g
#define unregister_enum_message _id3oi76j5dv

class MessageManager : public Object {
	GDCLASS(MessageManager, Object);
	static void _bind_methods();
	struct DeferredMessage {
		StringName message;
		int64_t id = -1;
		Array args;
		bool is_enum = false;
	};

public:
	static MessageManager *singleton;

	MessageManager();
	~MessageManager();
	static MessageManager *get_singleton() {
		return singleton;
	}

	void emit(const StringName &p_message, Array p_args);
	void emit_deferred(const StringName &p_message, Array p_args);

	void emit_enum(int64_t p_message, Array p_args);
	void emit_enum_deferred(int64_t p_message, Array p_args);

	void process(int p_max_count = 10);
	void register_message(const StringName &p_message, const Callable &p_callable);
	void unregister_message(const StringName &p_message, const Callable &p_callable);

	void register_enum_message(int64_t p_message, const Callable &p_callable);
	void unregister_enum_message(int64_t p_message, const Callable &p_callable);

	void clear_messages();
	void clear();

	HashMap<StringName, List<Callable>> messages;
	HashMap<int64_t, List<Callable>> enum_messages;

	SafeStack<DeferredMessage> deferred_messages_list;
};
