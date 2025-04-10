#pragma once

#include "beehave_node.h"

class BeehaveNode;
class BeehaveListener : public RefCounted {
	GDCLASS(BeehaveListener, RefCounted);

	static void _bind_methods() {
		GDVIRTUAL_BIND(_start, "run_context");
		GDVIRTUAL_BIND(_process, "run_context");
		GDVIRTUAL_BIND(_stop, "run_context");
	}

public:
	virtual void start(const Ref<BeehaveRuncontext> &run_context) {
#if !TOOLS_ENABLED
		if (is_editor_only) {
			return;
		}
#endif
		GDVIRTUAL_CALL(_start, run_context);
	}
	virtual void process(const Ref<BeehaveRuncontext> &run_context) {
#if !TOOLS_ENABLED
		if (is_editor_only) {
			return;
		}
#endif
		GDVIRTUAL_CALL(_process, run_context);
	}
	virtual void stop(const Ref<BeehaveRuncontext> &run_context) {
#if !TOOLS_ENABLED
		if (is_editor_only) {
			return;
		}
#endif
		GDVIRTUAL_CALL(_stop, run_context);
	}
	GDVIRTUAL1(_start, const Ref<BeehaveRuncontext> &);
	GDVIRTUAL1(_process, const Ref<BeehaveRuncontext> &);
	GDVIRTUAL1(_stop, const Ref<BeehaveRuncontext> &);
	bool is_editor_only = false;
};

class BeehaveTree : public Resource {
	GDCLASS(BeehaveTree, Resource);
	static void _bind_methods();
	enum Status {
		SUCCESS,
		FAILURE,
		RUNNING
	};

public:
	// 初始化
	void init(const Ref<BeehaveRuncontext> &run_context);
	int process(const Ref<BeehaveRuncontext> &run_context);
	void stop(const Ref<BeehaveRuncontext> &run_context);

protected:
	void on_stop(const Ref<BeehaveRuncontext> &run_context);
	int tick(const Ref<BeehaveRuncontext> &run_context);

public:
	void set_root_node(const Ref<BeehaveNode>& p_root_node);
	Ref<BeehaveNode> get_root_node();

	void set_listener(const TypedArray<BeehaveListener> &p_listener);

	TypedArray<BeehaveListener> get_listener();

public:
	void set_debug_break_node(BeehaveNode *p_node);
	BeehaveNode *get_debug_break_node();

public:
	Ref<BeehaveNode> root_node;

	LocalVector<Ref<BeehaveListener>> listeners;
	// 当前中断的节点
	class BeehaveNode *debug_break_node = nullptr;

	ObjectID last_editor_id;
	float tick_rate = 0.1f;
	float last_tick = 0.0f;
	float _process_time_metric_value = 0.0f;
	int status = -1;
	bool _can_send_message = false;
};
