#pragma once
#include "../beehave_node.h"
class BeehaveDecoratorLimiterTimer : public BeehaveDecorator {
	GDCLASS(BeehaveDecoratorLimiterTimer, BeehaveDecorator);
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_max_time", "time"), &BeehaveDecoratorLimiterTimer::set_max_time);
		ClassDB::bind_method(D_METHOD("get_max_time"), &BeehaveDecoratorLimiterTimer::get_max_time);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_time"), "set_max_time", "get_max_time");
	}

public:
	virtual String get_tooltip() override {
		return String(L"时间限制装饰器将为其处于“RUNNING”状态的子级提供一定时间来完成,\n如果子级还未返回“RUNNING”状态，此装饰器将在等待时间结束后返回“FAILURE”。");
	}

	virtual String get_lable_name() override {
		return L"时间限制装饰器";
	}
	virtual void interrupt(const Ref<BeehaveRuncontext> &run_context) override {
		super_type::interrupt(run_context);
	}

	virtual void before_run(const Ref<BeehaveRuncontext> &run_context) override {
		super_type::before_run(run_context);
		Dictionary prop = run_context->get_property(this);
		prop[SNAME("current_time")] = 0;
	}
	virtual int tick(const Ref<BeehaveRuncontext> &run_context) override {
		if (get_child_count() == 0) {
			return FAILURE;
		}
		Dictionary prop = run_context->get_property(this);
		float current_time = prop.get(SNAME("current_time"), 0.0f);
		current_time += (float)run_context->delta;
		prop[SNAME("current_time")] = current_time;
		if (current_time < max_time) {
			if (run_context->get_init_status(children[0].ptr()) == 0) {
				children[0]->before_run(run_context);
				current_time = 0;
			}
			int rs = children[0]->process(run_context);
			if (rs == NONE_PROCESS) {
				// 执行到断点,直接返回
				return rs;
			}
			run_context->set_run_state(children[0].ptr(), rs);
			if (rs == SUCCESS || rs == FAILURE) {
				for (int i = 0; i < get_child_count(); ++i) {
					if (run_context->get_init_status(children[i].ptr()) == 1) {
						children[i]->after_run(run_context);
					}
				}
				return rs;
			}
		} else {
			children[0]->after_run(run_context);
			return FAILURE;
		}
		return FAILURE;
	}

public:
	void set_max_time(float time) {
		max_time = time;
	}
	float get_max_time() {
		return max_time;
	}

protected:
	float max_time = 0.0f;
};
