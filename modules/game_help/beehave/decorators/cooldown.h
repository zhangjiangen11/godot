#pragma once
#include "../beehave_node.h"
class BeehaveDecoratorCooldown : public BeehaveDecorator {
	GDCLASS(BeehaveDecoratorCooldown, BeehaveDecorator);
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_wait_time", "time"), &BeehaveDecoratorCooldown::set_wait_time);
		ClassDB::bind_method(D_METHOD("get_wait_time"), &BeehaveDecoratorCooldown::get_wait_time);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wait_time"), "set_wait_time", "get_wait_time");
	}

public:
	virtual String get_tooltip() override {
		return String(L"冷却装饰器将在执行其子进程后在规定的时间内返回“FAILURE”。\n下次执行其子进程且其未处于“RUNNING”状态时,计时器将重置");
	}
	virtual String get_lable_name() override {
		return String(L"冷却装饰器");
	}
	virtual TypedArray<StringName> get_class_name() override {
		TypedArray<StringName> rs = super_type::get_class_name();
		rs.push_back("BeehaveSequence");
		return rs;
	}
	virtual StringName get_icon() override {
		return SNAME("cooldown");
	}
	virtual void before_run(const Ref<BeehaveRuncontext> &run_context) override {
		super_type::before_run(run_context);
		Dictionary prop = run_context->get_property(this);
		prop[SNAME("is_init")] = false;
		prop[SNAME("remaining_time")] = wait_time;
	}
	int tick(const Ref<BeehaveRuncontext> &run_context) override {
		Ref<BeehaveNode> child = get_child(0);
		if (child.is_null()) {
			return FAILURE;
		}
		int response;
		Dictionary prop = run_context->get_property(this);
		// float total_time = prop.get(SNAME("total_time"), 0.0f);
		bool is_init = prop.get(SNAME("is_init"), false);
		if (!is_init) {
			before_run(run_context);
		}
		float remaining_time = prop.get(SNAME("remaining_time"), wait_time);
		if (remaining_time > 0) {
			response = FAILURE;
			remaining_time -= run_context->delta;
			prop[SNAME("remaining_time")] = remaining_time;
		} else {
			if (run_context->get_init_status(children[0].ptr()) == 0) {
				children[0]->before_run(run_context);
			}
			response = child->process(run_context);
			if (response == NONE_PROCESS) {
				// 执行到断点,直接返回
				return response;
			}
			if (response == SUCCESS || response == FAILURE) {
				children[0]->after_run(run_context);
			}
		}
		if (response == NONE_PROCESS) {
			return response;
		}
		run_context->set_run_state(child.ptr(), response);
		return response;
	}

public:
	void set_wait_time(float time) {
		wait_time = time;
	}
	float get_wait_time() {
		return wait_time;
	}

protected:
	float wait_time = 0;
};
