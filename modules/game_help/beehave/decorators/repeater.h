#pragma once
#include "../beehave_node.h"

class BeehaveDecoratorRepeater : public BeehaveDecorator {
	GDCLASS(BeehaveDecoratorRepeater, BeehaveDecorator);

public:
protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_max_count"), &BeehaveDecoratorRepeater::get_max_count);
		ClassDB::bind_method(D_METHOD("set_max_count", "max_count"), &BeehaveDecoratorRepeater::set_max_count);

		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_count"), "set_max_count", "get_max_count");
	}

public:
	virtual String get_tooltip() override {
		return L"转发器将执行其子进程，直到它返回一定次数的 `SUCCESS`当达到最大刻度数时，它将返回 `SUCCESS` 状态代码。如果子进程返回 `FAILURE`，转发器将立即返回 `FAILURE`。";
	}

	virtual String get_lable_name() override {
		return L"重复执行装饰器";
	}
	virtual void interrupt(const Ref<BeehaveRuncontext> &run_context) override {
		super_type::interrupt(run_context);
	}

	virtual void before_run(const Ref<BeehaveRuncontext> &run_context) override {
		super_type::before_run(run_context);
		Dictionary prop = run_context->get_property(this);
		prop[SNAME("current_count")] = 0;
	}
	virtual int tick(const Ref<BeehaveRuncontext> &run_context) override {
		if (get_child_count() == 0) {
			return FAILURE;
		}
		Dictionary prop = run_context->get_property(this);
		int current_count = prop.get(SNAME("current_count"), 0);
		if (current_count < max_count) {
			current_count++;
			if (run_context->get_init_status(children[0].ptr()) == 0) {
				children[0]->before_run(run_context);
			}
			int rs = children[0]->process(run_context);
			if (rs == NONE_PROCESS) {
				return rs;
			}
			run_context->set_run_state(children[0].ptr(), rs);
			if (rs == RUNNING) {
				// 执行到断点,直接返回
				return rs;
			}
			current_count += 1;
			children[0]->after_run(run_context);
			if (rs == FAILURE) {
				return FAILURE;
			}
		} else {
			children[0]->after_run(run_context);
			return FAILURE;
		}
		return SUCCESS;
	}

public:
	void set_max_count(int p_max_count) {
		max_count = p_max_count;
	}

	int get_max_count() {
		return max_count;
	}

protected:
	int max_count = 0;
};
