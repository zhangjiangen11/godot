#pragma once
#include "../../character_ai/animator_condition.h"
#include "../beehave_node.h"

class BeehaveLeafBlackboardCondition : public BeehaveLeaf {
	GDCLASS(BeehaveLeafBlackboardCondition, BeehaveLeaf);
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_blackboard_condition", "blackboard_condition"), &BeehaveLeafBlackboardCondition::set_blackboard_condition);
		ClassDB::bind_method(D_METHOD("get_blackboard_condition"), &BeehaveLeafBlackboardCondition::get_blackboard_condition);

		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blackboard_condition", PROPERTY_HINT_RESOURCE_TYPE, "CharacterAnimatorCondition"), "set_blackboard_condition", "get_blackboard_condition");
	}

public:
	virtual String get_tooltip() override {
		return String(L"判断黑板条件是否成功。");
	}
	virtual String get_lable_name() override {
		return L"黑板条件叶节点";
	}
	virtual StringName get_icon() override {
		return SNAME("condition");
	}
	virtual int tick(const Ref<BeehaveRuncontext> &run_context) override {
		if (blackboard_condition.is_valid()) {
			if (blackboard_condition->is_enable(run_context->blackboard.ptr())) {
				return SUCCESS;
			} else {
				return FAILURE;
			}
		}
		return SUCCESS;
	}

protected:
	void set_blackboard_condition(Ref<CharacterAnimatorCondition> _blackboard_condition) {
		blackboard_condition = _blackboard_condition;
	}

	Ref<CharacterAnimatorCondition> get_blackboard_condition() {
		return blackboard_condition;
	}

public:
	Ref<CharacterAnimatorCondition> blackboard_condition;
};
