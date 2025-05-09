#pragma once

#include "../../logic/character_ai/animator_blackboard_set.h"
#include "../beehave_node.h"

class BeehaveLeafBlackboardSet : public BeehaveLeaf {
	GDCLASS(BeehaveLeafBlackboardSet, BeehaveLeaf);
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_blackboard_set", "blackboard_set"), &BeehaveLeafBlackboardSet::set_blackboard_set);
		ClassDB::bind_method(D_METHOD("get_blackboard_set"), &BeehaveLeafBlackboardSet::get_blackboard_set);

		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blackboard_set", PROPERTY_HINT_RESOURCE_TYPE, "CharacterAnimatorBlackboardSet"), "set_blackboard_set", "get_blackboard_set");
	}

public:
	virtual String get_tooltip() override {
		return String(L"设置黑板的值。");
	}
	virtual String get_lable_name() override {
		return String(L"设置黑板的叶节点");
	}
	virtual StringName get_icon() override {
		return SNAME("BTSetVar");
	}
	virtual void after_run(const Ref<BeehaveRuncontext> &run_context) override {
		if (blackboard_set.is_valid()) {
			blackboard_set->execute(run_context->blackboard.ptr());
		}
		return;
	}

public:
	void set_blackboard_set(Ref<AnimatorBlackboardSet> p_blackboard_set) {
		blackboard_set = p_blackboard_set;
	}
	Ref<AnimatorBlackboardSet> get_blackboard_set() {
		return blackboard_set;
	}

protected:
	Ref<AnimatorBlackboardSet> blackboard_set;
};
