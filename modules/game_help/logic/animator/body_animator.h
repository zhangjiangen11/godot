#pragma once
#include "../character_ai/animator_blackboard_set.h"
#include "../character_ai/animator_condition.h"
#include "../character_ai/body_animator_logic.h"
#include "animation_help.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/packed_scene.h"

#include "character_animation_node.h"
#include "character_update_tool.h"

class CharacterAnimatorNodeBase;
class CharacterAnimatorLayer;
class CharacterAnimator;
class CharacterAnimationLibraryItem : public RefCounted {
	GDCLASS(CharacterAnimationLibraryItem, RefCounted);
	static void _bind_methods();

public:
	void load();
	Ref<CharacterAnimatorNodeBase> get_node();
	void set_path(String p_path) { path = p_path; }
	String get_path() { return path; }

	void set_name(StringName p_name) {
		name = p_name;
	}
	StringName get_name() { return name; }

	void _set_node(Ref<CharacterAnimatorNodeBase> p_node);
	Ref<CharacterAnimatorNodeBase> _get_node() { return node; }

public:
	Ref<CharacterAnimatorNodeBase> node;
	StringName name;
	String path;
	int is_loaded = 0;
};
// 动画库
class CharacterAnimationLibrary : public Resource {
	GDCLASS(CharacterAnimationLibrary, Resource);

	static void _bind_methods();

public:
public:
	void set_animation_library(const TypedArray<CharacterAnimationLibraryItem> &p_animation_library);
	TypedArray<CharacterAnimationLibraryItem> get_animation_library();

	Ref<CharacterAnimationLibraryItem> get_animation_by_name(StringName p_name);
	void init_animation_library();

public:
	enum AnimationNodeType {
		T_CharacterAnimatorNode1D,
		T_CharacterAnimatorNode2D,
		T_CharacterAnimatorLoopLast,
	};
	void set_base_library(Ref<CharacterAnimationLibrary> p_base_library);
	Ref<CharacterAnimationLibrary> get_base_library() { return base_library; }

	void set_animator_node_name(String p_animator_node_name) { animator_node_name = p_animator_node_name; }
	String get_animator_node_name() { return animator_node_name; }

	void set_animator_node_type(int p_animator_node_type) { animator_node_type = p_animator_node_type; }
	int get_animator_node_type() { return animator_node_type; }

	String animator_node_name;
	int animator_node_type = T_CharacterAnimatorNode1D;
	DECL_MEMBER_BUTTON(editor_create_animation_node);

public:
	// 繼承的基类动作库
	Ref<CharacterAnimationLibrary> base_library;
	TypedArray<CharacterAnimationLibraryItem> animation_library;
	HashMap<StringName, Ref<CharacterAnimationLibraryItem>> animations;
	bool is_init = false;
};

// 时间线资源,这个主要用来Animation 对角色进行一些操控,比如播放动画,切换角色材质
class CharacterTimelineNode : public Node3D {
	GDCLASS(CharacterTimelineNode, Node3D);
	static void _bind_methods() {
	}

public:
	class CharacterBodyMain *m_Body = nullptr;
	AnimationPlayer *m_AnimationPlayer = nullptr;
	Ref<Animation> m_Animation;

	void play_action(StringName p_action_name) {}

	void set_float_value(StringName p_name, float value) {}
};
// 动画逻辑上下文
struct CharacterAnimationLogicContext {
	// 动画逻辑
	Ref<CharacterAnimationLogicLayer> animation_logic;
	// 当前状态名称
	StringName last_name;
	// 当前状态名称
	StringName curr_name;
	Ref<CharacterAnimationLogicRoot> curr_state_root;
	// 当前处理的逻辑节点
	Ref<CharacterAnimationLogicNode> curr_logic;
	Ref<CharacterAnimationLibraryItem> curr_animation;
	// 执行时长
	float time = 0.0f;
	bool is_start = false;
	float curr_animation_play_time = 0.0f;
	float curr_animation_time_length = 0.0f;
};

// 动画分层
class CharacterAnimatorLayer : public AnimationMixer {
	GDCLASS(CharacterAnimatorLayer, AnimationMixer);
	static void _bind_methods() {
	}

public:
public:
	void _process_logic(const Ref<Blackboard> &p_playback_info, double p_delta, bool is_first = true);
	// 处理动画
	void _process_animator(const Ref<Blackboard> &p_playback_info, double p_delta, bool is_first = true);
	// 处理动画
	void _process_animation_item(const Ref<Blackboard> &p_playback_info, CharacterRootMotion &root_motion, HashMap<String, float> &bone_blend_weight, double p_delta, bool is_using_root_motion, bool is_first = true);

	void finish_update();

	void init(Skeleton3D *p_skeleton, CharacterAnimator *p_animator, const Ref<CharacterAnimatorLayerConfig> &_config);
	CharacterAnimationLogicContext *_get_logic_context() {
		return &logic_context;
	}

	void play_animationm(const Ref<Animation> &p_anim, const PlaybackInfo &p_playback_info, const Dictionary &bone_map);

	void play_animation(const Ref<Animation> &p_anim, bool p_is_loop);
	bool play_animation(const Ref<CharacterAnimatorNodeBase> &p_node);
	void play_animation(const StringName &p_node_name);
	void change_state(const StringName &p_state_name);
	void on_destory();
	CharacterAnimatorLayer();
	~CharacterAnimatorLayer();

public:
	void set_config(const Ref<CharacterAnimatorLayerConfig> &_config) { config = _config; }
	const Ref<CharacterAnimatorLayerConfig> &get_config() { return config; }

public:
	void set_editor_stop_animation(bool p_v) {
		editor_stop_animation = p_v;
	}

protected:
	CharacterAnimator *get_animation() {
		return (CharacterAnimator *)Object::cast_to<CharacterAnimator>(ObjectDB::get_instance(m_Animator_ID));
	}

public:
	Vector<Vector2> m_ChildInputVectorArray;
	Vector<int> m_TempCropArray;

protected:
	ObjectID skeleton_id;
	// 黑板信息
	Ref<Blackboard> blackboard;
	// 逻辑上下文
	CharacterAnimationLogicContext logic_context;
	// 动画掩码
	Ref<CharacterAnimatorLayerConfig> config;
	Ref<CharacterAnimationUpdateTool> update_tool;
	List<Ref<CharacterAnimatorNodeBase>> play_list;
	Vector<float> m_TotalAnimationWeight;
	List<CharacterAnimationInstance> m_AnimationInstances;
	ObjectID m_Animator_ID;
	float blend_weight = 1.0f;
	bool editor_stop_animation = false;
};

// 动画逻辑节点
class CharacterAnimationLogicNode : public Resource {
	GDCLASS(CharacterAnimationLogicNode, Resource)
	static void _bind_methods();

public:
	enum AnimatorAIStopCheckType {
		// 固定生命期
		Life,
		AnimationLengthScale,
		// 通过检测条件结束
		Condition,
		Script
	};
	struct SortCharacterAnimationLogicNode {
		bool operator()(const Ref<CharacterAnimationLogicNode> &l, const Ref<CharacterAnimationLogicNode> &r) const {
			int lp = 0;
			int rp = 0;
			if (l.is_valid()) {
				lp = l->priority;
			}
			if (r.is_valid()) {
				rp = r->priority;
			}

			return lp > rp;
		}
	};

public:
	void set_blackboard_plan(const Ref<BlackboardPlan> &p_blackboard_plan);
	Ref<BlackboardPlan> get_blackboard_plan() { return blackboard_plan; }
	void update_blackboard_plan();
	bool get_editor_state() {
		return false;
	}

	void set_priority(int p_priority) { priority = p_priority; }
	int get_priority() { return priority; }

	void set_player_animation_name(StringName p_player_animation_name) { player_animation_name = p_player_animation_name; }
	StringName get_player_animation_name() { return player_animation_name; }

	void set_enter_condtion(const Ref<CharacterAnimatorCondition> &p_enter_condtion) {
		enter_condtion = p_enter_condtion;
		update_blackboard_plan();
	}
	Ref<CharacterAnimatorCondition> get_enter_condtion() { return enter_condtion; }

	void set_start_blackboard_set(const Ref<AnimatorBlackboardSet> &p_start_blackboard_set) {
		start_blackboard_set = p_start_blackboard_set;
		update_blackboard_plan();
	}
	Ref<AnimatorBlackboardSet> get_start_blackboard_set() { return start_blackboard_set; }

	void set_stop_blackboard_set(const Ref<AnimatorBlackboardSet> &p_stop_blackboard_set) {
		stop_blackboard_set = p_stop_blackboard_set;
		update_blackboard_plan();
	}
	Ref<AnimatorBlackboardSet> get_stop_blackboard_set() { return stop_blackboard_set; }

	void set_check_stop_delay_time(float p_check_stop_delay_time) { check_stop_delay_time = p_check_stop_delay_time; }
	float get_check_stop_delay_time() { return check_stop_delay_time; }

	void set_life_time(float p_life_time) { life_time = p_life_time; }
	float get_life_time() { return life_time; }

	void set_stop_check_type(AnimatorAIStopCheckType p_stop_check_type) { stop_check_type = p_stop_check_type; }
	AnimatorAIStopCheckType get_stop_check_type() { return stop_check_type; }

	void set_stop_check_condtion(const Ref<CharacterAnimatorCondition> &p_stop_check_condtion) {
		stop_check_condtion = p_stop_check_condtion;
		update_blackboard_plan();
	}
	Ref<CharacterAnimatorCondition> get_stop_check_condtion() { return stop_check_condtion; }

	void set_stop_check_anmation_length_scale(float p_stop_check_anmation_length_scale) { anmation_scale = p_stop_check_anmation_length_scale; }
	float get_stop_check_anmation_length_scale() { return anmation_scale; }

	static void init_blackboard(Ref<BlackboardPlan> p_blackboard_plan);

public:
	virtual void process_start(CharacterAnimatorLayer *animator, Blackboard *blackboard);
	virtual void process(CharacterAnimatorLayer *animator, Blackboard *blackboard, double delta);
	virtual bool check_stop(CharacterAnimatorLayer *animator, Blackboard *blackboard);
	virtual void process_stop(CharacterAnimatorLayer *animator, Blackboard *blackboard);

	void _blackboard_changed() {
		editor_state_change.call();
	}
	Callable editor_state_change;

public:
	bool is_enter(Blackboard *blackboard);
	~CharacterAnimationLogicNode();

private:
	GDVIRTUAL2(_animation_process_start, CharacterAnimatorLayer *, Blackboard *)
	GDVIRTUAL2(_animation_process_stop, CharacterAnimatorLayer *, Blackboard *)
	GDVIRTUAL3(_animation_process, CharacterAnimatorLayer *, Blackboard *, double)
	GDVIRTUAL2R(bool, _check_stop, CharacterAnimatorLayer *, Blackboard *)

public:
	// 优先级
	int priority = 0;
	// 播放的动画名称
	StringName player_animation_name;
	// 进入条件
	Ref<CharacterAnimatorCondition> enter_condtion;
	// 進入节点设置的黑板
	Ref<AnimatorBlackboardSet> start_blackboard_set;
	// 退出节点设置的黑板
	Ref<AnimatorBlackboardSet> stop_blackboard_set;
	Ref<BlackboardPlan> blackboard_plan;
	// 检测结束等待时间
	float check_stop_delay_time = 0.0f;
	AnimatorAIStopCheckType stop_check_type = Life;
	// 生命期
	float life_time = 0.0f;
	float anmation_scale = 1.0f;

	// 退出检测条件
	Ref<CharacterAnimatorCondition> stop_check_condtion;
};

// 动画层配置实例
class CharacterAnimatorLayerConfigInstance : public RefCounted {
	GDCLASS(CharacterAnimatorLayerConfigInstance, RefCounted);
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_config", "config"), &CharacterAnimatorLayerConfigInstance::set_config);
		ClassDB::bind_method(D_METHOD("get_config"), &CharacterAnimatorLayerConfigInstance::get_config);

		ClassDB::bind_method(D_METHOD("set_play_animation", "play_animation"), &CharacterAnimatorLayerConfigInstance::set_play_animation);
		ClassDB::bind_method(D_METHOD("get_play_animation"), &CharacterAnimatorLayerConfigInstance::get_play_animation);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "play_animation", PROPERTY_HINT_RESOURCE_TYPE, "Animation"), "set_play_animation", "get_play_animation");

		ADD_MEMBER_BUTTON(editor_play_select_animation, L"播放动画", CharacterAnimatorLayerConfigInstance);

		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "config", PROPERTY_HINT_RESOURCE_TYPE, "CharacterAnimatorLayerConfig"), "set_config", "get_config");
	}

public:
	void set_body(class CharacterBodyMain *p_body);
	void change_state(const StringName &p_state_name) {
	}
	void set_config(const Ref<CharacterAnimatorLayerConfig> &_config) {
		config = _config;
		auto_init();
	}
	Ref<CharacterAnimatorLayerConfig> get_config() {
		return config;
	}

	void set_play_animation(const Ref<Animation> &p_play_animation) {
		play_animation = p_play_animation;
	}

	Ref<Animation> get_play_animation() {
		return play_animation;
	}

	void _process_animator(const Ref<Blackboard> &p_playback_info, double p_delta, bool is_first = true) {
		CharacterAnimatorLayer *layer = get_layer();
		if (layer == nullptr) {
			return;
		}

		if (layer->is_active()) {
			layer->_process_animator(p_playback_info, p_delta, is_first);
		}
	}
	void _process_animation_item(const Ref<Blackboard> &p_playback_info, CharacterRootMotion &root_motion, HashMap<String, float> &bone_blend_weight, double p_delta, bool p_is_root_motion, bool is_first = true) {
		CharacterAnimatorLayer *layer = get_layer();
		if (layer == nullptr) {
			return;
		}

		if (layer->is_active()) {
			layer->_process_animation_item(p_playback_info, root_motion, bone_blend_weight, p_delta, p_is_root_motion, is_first);
		}
	}
	void finish_update() {
		CharacterAnimatorLayer *layer = get_layer();
		if (layer == nullptr) {
			return;
		}

		if (layer->is_active()) {
			layer->finish_update();
		}
	}

public:
	void editor_play_animation(const Ref<CharacterAnimatorNodeBase> &p_node) {
		CharacterAnimatorLayer *layer = get_layer();
		layer->play_animation(p_node);
	}
	void editor_play_animation(const Ref<Animation> &p_node) {
		play_animation = p_node;
		CharacterAnimatorLayer *layer = get_layer();
		layer->play_animation(p_node, true);
	}

	void set_editor_stop_animation(bool p_v) {
		CharacterAnimatorLayer *layer = get_layer();
		if (layer != nullptr) {
			layer->set_editor_stop_animation(p_v);
		}
	}

	CharacterAnimatorLayer *get_layer();

protected:
	void auto_init();
	class CharacterBodyMain *get_body();

protected:
	Ref<Animation> play_animation;
	DECL_MEMBER_BUTTON(editor_play_select_animation);

protected:
	Ref<CharacterAnimatorLayerConfig> config;
	ObjectID layer_id;
	ObjectID m_Body_ID;
	bool is_init = false;
};

class CharacterAnimator : public RefCounted {
	GDCLASS(CharacterAnimator, RefCounted);
	static void _bind_methods();

	List<Ref<CharacterAnimatorLayerConfigInstance>> m_LayerConfigInstanceList;
	CharacterRootMotion root_motion;
	HashMap<String, float> bone_blend_weight;
	ObjectID m_Body_ID;
	double time_delta = 0.0;
	bool is_using_root_motion = true;

public:
	void set_body(class CharacterBodyMain *p_body);

	void add_layer(const Ref<CharacterAnimatorLayerConfig> &_mask);

	void _thread_update_animator(float delta);

	double get_time_delta() {
		return time_delta;
	}

	void set_is_using_root_motion(bool p_is_root_motion) {
		is_using_root_motion = p_is_root_motion;
	}
	bool get_is_using_root_motion() {
		return is_using_root_motion;
	}

	void _thread_update_animation(float delta);

	void finish_update();
	void change_state(const StringName &p_state_name);

	void on_layer_delete(CharacterAnimatorLayer *p_layer);
	const CharacterRootMotion &get_root_motion() {
		return root_motion;
	}
	Ref<CharacterAnimationLibraryItem> get_animation_by_name(const StringName &p_name);
	void set_animation_layer_arrays(TypedArray<CharacterAnimatorLayerConfigInstance> p_animation_layer_arrays);
	TypedArray<CharacterAnimatorLayerConfigInstance> get_animation_layer_arrays();
	void init();

	~CharacterAnimator();

public:
	void editor_play_animation(const Ref<Animation> &p_node) {
		if (m_LayerConfigInstanceList.size() == 0) {
			return;
		}
		m_LayerConfigInstanceList.front()->get()->editor_play_animation(p_node);
	}
	void editor_play_animation(const Ref<CharacterAnimatorNodeBase> &p_node) {
		if (m_LayerConfigInstanceList.size() == 0) {
			return;
		}
		m_LayerConfigInstanceList.front()->get()->editor_play_animation(p_node);
	}

	void set_editor_stop_animation(bool v) {
		if (m_LayerConfigInstanceList.size() == 0) {
			return;
		}
		m_LayerConfigInstanceList.front()->get()->set_editor_stop_animation(v);
	}

protected:
	class CharacterBodyMain *get_body();
};
VARIANT_ENUM_CAST(CharacterAnimatorNodeBase::LoopType)
VARIANT_ENUM_CAST(CharacterAnimationLogicNode::AnimatorAIStopCheckType)
VARIANT_ENUM_CAST(CharacterAnimatorLayerConfig::BlendType)
VARIANT_ENUM_CAST(CharacterAnimationLibrary::AnimationNodeType)
