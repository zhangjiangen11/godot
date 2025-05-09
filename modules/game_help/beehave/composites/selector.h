#pragma once
#include "../beehave_node.h"
class BeehaveCompositeSelector : public BeehaveComposite {
	GDCLASS(BeehaveCompositeSelector, BeehaveComposite);
	static void _bind_methods() {
	}

public:
	virtual TypedArray<StringName> get_class_name() override {
		TypedArray<StringName> rs = super_type::get_class_name();
		rs.push_back("BeehaveCompositeSelector");
		return rs;
	}
	// 獲取支持放几个子节点,-1 是任意多子节点
	virtual int get_supper_child_count() override {
		return -1;
	}
	virtual String get_lable_name() override {
		return String(L"选择组合节点");
	}
	virtual String get_tooltip() override {
		return String(L"## 选择器节点将尝试执行其每个子节点,直到其中一个返回“SUCCESS”。\n如果所有子节点都返回“FAILURE”,则此节点也将 返回“FAILURE”。\n如果子节点返回“RUNNING”,它将再次运行。");
	}
	virtual StringName get_icon() override {
		return SNAME("selector");
	}

	virtual int tick(const Ref<BeehaveRuncontext> &run_context) override {
		for (int i = last_execution_index; i < get_child_count(); i++) {
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
				last_execution_index = i + 1;
				children[i]->after_run(run_context);
				return SUCCESS;
			} else if (rs == FAILURE) {
				last_execution_index = i + 1;
				children[i]->after_run(run_context);
				return rs;
			} else if (rs == RUNNING) {
				return rs;
			}
		}
		return FAILURE;
	}
	virtual void interrupt(const Ref<BeehaveRuncontext> &run_context) override {
		super_type::interrupt(run_context);
		last_execution_index = 0;
	}
	virtual void after_run(const Ref<BeehaveRuncontext> &run_context) override {
		super_type::after_run(run_context);
		last_execution_index = 0;
	}

	int last_execution_index = 0;
};
