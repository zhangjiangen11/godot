#pragma once
#include "../beehave_node.h"
class BeehaveCompositeSequence : public BeehaveComposite {
	GDCLASS(BeehaveCompositeSequence, BeehaveComposite);
	static void _bind_methods() {
	}

public:
	virtual String get_tooltip() override {
		return String(L"## 序列节点将尝试执行其所有子节点，并报告, \n如果所有子节点都报告 `SUCCESS` 状态代码，则报告 `SUCCESS`。\n如果至少一个子节点报告 `FAILURE` 状态代码，则此节点也将返回 `FAILURE` 下次继续从这个位置开始。 \n如果子节点返回 `RUNNING`，则此节点将再次运行。");
	}
	virtual String get_lable_name() override {
		return String(L"序列组合节点");
	}
	virtual StringName get_icon() override {
		return SNAME("BTSequence");
	}
	virtual void interrupt(const Ref<BeehaveRuncontext> &run_context) override {
		super_type::interrupt(run_context);
		Dictionary prop = run_context->get_property(this);
		prop[SNAME("successful_index")] = 0;
	}

	virtual TypedArray<StringName> get_class_name() override {
		TypedArray<StringName> rs = super_type::get_class_name();
		rs.push_back(StringName("BeehaveCompositeSequence"));
		return rs;
	}

	virtual int tick(const Ref<BeehaveRuncontext> &run_context) override {
		Dictionary prop = run_context->get_property(this);
		int _successful_index = prop.get(SNAME("successful_index"), 0);
		for (int i = _successful_index; i < get_child_count(); i++) {
			if (run_context->get_init_status(children[i].ptr()) == 0) {
				children[i]->before_run(run_context);
			}
			int rs = children[i]->process(run_context);
			if (rs == NONE_PROCESS) {
				// 执行到断点,直接返回
				return rs;
			}
			run_context->set_run_state(children[i].ptr(), rs);
			if (rs == SUCCESS) {
				_successful_index = i + 1;
				prop[SNAME("successful_index")] = _successful_index;
				children[i]->after_run(run_context);
			}
			if (rs == FAILURE) {
				_successful_index = i + 1;
				prop[SNAME("successful_index")] = _successful_index;
				children[i]->after_run(run_context);
				return rs;
			}
			if (rs == RUNNING) {
				return rs;
			}
		}
		_successful_index = 0;
		prop[SNAME("successful_index")] = _successful_index;
		return SUCCESS;
	}
};
