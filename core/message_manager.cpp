#include "message_manager.h"

MessageManager *MessageManager::singleton = nullptr;
void MessageManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("emit", "msg_name", "arg"), &MessageManager::emit);
	ClassDB::bind_method(D_METHOD("emit_deferred", "msg_name", "arg"), &MessageManager::emit_deferred);
	ClassDB::bind_method(D_METHOD("emit_enum", "msg_name", "arg"), &MessageManager::emit_enum);
	ClassDB::bind_method(D_METHOD("emit_enum_deferred", "msg_name", "arg"), &MessageManager::emit_enum_deferred);
	ClassDB::bind_method(D_METHOD("process", "max_process_count"), &MessageManager::process);
	ClassDB::bind_method(D_METHOD("register_message", "msg_name", "callable"), &MessageManager::register_message);
	ClassDB::bind_method(D_METHOD("unregister_message", "msg_name", "callable"), &MessageManager::unregister_message);
	ClassDB::bind_method(D_METHOD("register_enum_message", "msg_name", "callable"), &MessageManager::register_enum_message);
	ClassDB::bind_method(D_METHOD("unregister_enum_message", "msg_name", "callable"), &MessageManager::unregister_enum_message);
	ClassDB::bind_method(D_METHOD("clear_messages"), &MessageManager::clear_messages);
	ClassDB::bind_method(D_METHOD("clear"), &MessageManager::clear);
}

MessageManager::MessageManager() {
}
MessageManager::~MessageManager() {
}

void MessageManager::emit(const StringName &p_message, Array p_args) {
	if (!Thread::is_main_thread()) {
		ERR_PRINT("only call man thread,MessageManager::emit");
		return;
	}
	List<Callable> *callables = messages.getptr(p_message);
	if (callables == nullptr || callables->size() == 0) {
		return;
	}
	const Variant **p_args_ptr = (const Variant **)alloca(sizeof(Variant *) * p_args.size());
	for (int i = 0; i < p_args.size(); i++) {
		p_args_ptr[i] = &p_args[i];
	}
	Variant rs;
	Callable::CallError r_error;
	auto it = callables->begin();
	while (it != callables->end()) {
		if (!it->is_valid()) {
			it = callables->erase(it);
			continue;
		}

		it->callp(p_args_ptr, p_args.size(), rs, r_error);
		if (r_error.error != Callable::CallError::CALL_OK) {
			it = callables->erase(it);
		} else {
			++it;
		}
	}
}
void MessageManager::emit_deferred(const StringName &p_message, Array p_args) {
	DeferredMessage msg;
	msg.message = p_message;
	msg.args = p_args;
	deferred_messages_list.push(msg);
}

void MessageManager::emit_enum(int64_t p_message, Array p_args) {
	List<Callable> *callables = enum_messages.getptr(p_message);
	if (callables == nullptr) {
		return;
	}
	const Variant **p_args_ptr = (const Variant **)alloca(sizeof(Variant *) * p_args.size());
	for (int i = 0; i < p_args.size(); i++) {
		p_args_ptr[i] = &p_args[i];
	}
	Variant rs;
	Callable::CallError r_error;
	auto it = callables->begin();
	while (it != callables->end()) {
		if (!it->is_valid()) {
			it = callables->erase(it);
			continue;
		}

		it->callp(p_args_ptr, p_args.size(), rs, r_error);
		if (r_error.error != Callable::CallError::CALL_OK) {
			it = callables->erase(it);
		} else {
			++it;
		}
	}
}
void MessageManager::emit_enum_deferred(int64_t p_message, Array p_args) {
	DeferredMessage msg;
	msg.id = p_message;
	msg.args = p_args;
	msg.is_enum = true;
	deferred_messages_list.push(msg);
}
void MessageManager::process(int p_max_count) {
	bool processing = true;
	DeferredMessage msg;
	while (true) {
		processing = deferred_messages_list.pop(msg);
		if (processing) {
			if (msg.is_enum) {
				emit_enum(msg.id, msg.args);
			} else {
				emit(msg.message, msg.args);
			}
			p_max_count--;
		}
		if (p_max_count <= 0) {
			break;
		}
		if (p_max_count <= 0 || processing == false) {
			break;
		}
	}
}
void MessageManager::register_message(const StringName &p_message, const Callable &p_callable) {
	if (!Thread::is_main_thread()) {
		ERR_PRINT("only call man thread,MessageManager::register_message");
		return;
	}
	List<Callable> *callables = messages.getptr(p_message);
	if (callables == nullptr) {
		callables = memnew(List<Callable>);
		messages[p_message].push_back(p_callable);
		return;
	}
	List<Callable>::Element *it = callables->find(p_callable);
	if (it == nullptr) {
		callables->push_back(p_callable);
	}
}
void MessageManager::unregister_message(const StringName &p_message, const Callable &p_callable) {
	if (!Thread::is_main_thread()) {
		ERR_PRINT("only call man thread");
		ERR_PRINT("only call man thread,MessageManager::unregister_message");
		return;
	}
	List<Callable> *callables = messages.getptr(p_message);
	if (callables == nullptr) {
		return;
	}
	List<Callable>::Element *it = callables->find(p_callable);
	if (it != nullptr) {
		// 不破坏原有的List结构
		it->get().clear();
	}
}
void MessageManager::register_enum_message(int64_t p_message, const Callable &p_callable) {
	List<Callable> *callables = enum_messages.getptr(p_message);
	if (callables == nullptr) {
		callables = memnew(List<Callable>);
		enum_messages[p_message].push_back(p_callable);
		return;
	}
	List<Callable>::Element *it = callables->find(p_callable);
	if (it == nullptr) {
		callables->push_back(p_callable);
	}
}
void MessageManager::unregister_enum_message(int64_t p_message, const Callable &p_callable) {
	List<Callable> *callables = enum_messages.getptr(p_message);
	if (callables == nullptr) {
		return;
	}
	List<Callable>::Element *it = callables->find(p_callable);
	if (it != nullptr) {
		// 不破坏原有的List结构
		it->get().clear();
	}
}
void MessageManager::clear_messages() {
	deferred_messages_list.clear();
}
void MessageManager::clear() {
	messages.clear();
}
