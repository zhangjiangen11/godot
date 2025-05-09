#include "body_animator.h"
#include "../body_main.h"
#include "../data_table_manager.h"
#include "core/io/json.h"

void CharacterAnimationLibraryItem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_path", "path"), &CharacterAnimationLibraryItem::set_path);
	ClassDB::bind_method(D_METHOD("get_path"), &CharacterAnimationLibraryItem::get_path);

	ClassDB::bind_method(D_METHOD("set_name", "name"), &CharacterAnimationLibraryItem::set_name);
	ClassDB::bind_method(D_METHOD("get_name"), &CharacterAnimationLibraryItem::get_name);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_path", "get_path");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_name", "get_name");

#if TOOLS_ENABLED
	ClassDB::bind_method(D_METHOD("_set_node", "node"), &CharacterAnimationLibraryItem::_set_node);
	ClassDB::bind_method(D_METHOD("_get_node"), &CharacterAnimationLibraryItem::_get_node);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_RESOURCE_TYPE, "CharacterAnimatorNodeBase", PROPERTY_USAGE_EDITOR), "_set_node", "_get_node");
#endif
}

void CharacterAnimationLibraryItem::load() {
	if (is_loaded == 0) {
		ResourceLoader::load_threaded_request(path);
		is_loaded = 1;
	}
}
Ref<CharacterAnimatorNodeBase> CharacterAnimationLibraryItem::get_node() {
	if (is_loaded == 1) {
		node = ResourceLoader::load_threaded_get(path);
		is_loaded = 2;
	}
	return node;
}

void CharacterAnimationLibraryItem::_set_node(Ref<CharacterAnimatorNodeBase> p_node) {
	if (p_node.is_null()) {
		return;
	}
	if (p_node->get_path() == "") {
		return;
	}
	node = p_node;
	path = p_node->get_path();
	name = path.get_file().get_basename();
}
/*****************************************************************************************************/

void CharacterAnimationLibrary::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_animation_library", "animation_library"), &CharacterAnimationLibrary::set_animation_library);
	ClassDB::bind_method(D_METHOD("get_animation_library"), &CharacterAnimationLibrary::get_animation_library);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "animation_library", PROPERTY_HINT_ARRAY_TYPE, "String"), "set_animation_library", "get_animation_library");

#if TOOLS_ENABLED

	ADD_GROUP(L"创建动画节点", "editor_");

	ClassDB::bind_method(D_METHOD("set_base_library", "base_library"), &CharacterAnimationLibrary::set_base_library);
	ClassDB::bind_method(D_METHOD("get_base_library"), &CharacterAnimationLibrary::get_base_library);

	ClassDB::bind_method(D_METHOD("set_animator_node_name", "animator_node_name"), &CharacterAnimationLibrary::set_animator_node_name);
	ClassDB::bind_method(D_METHOD("get_animator_node_name"), &CharacterAnimationLibrary::get_animator_node_name);

	ClassDB::bind_method(D_METHOD("set_animator_node_type", "animator_node_type"), &CharacterAnimationLibrary::set_animator_node_type);
	ClassDB::bind_method(D_METHOD("get_animator_node_type"), &CharacterAnimationLibrary::get_animator_node_type);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "base_library", PROPERTY_HINT_RESOURCE_TYPE, "CharacterAnimationLibrary"), "set_base_library", "get_base_library");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "editor_animator_node_name"), "set_animator_node_name", "get_animator_node_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "editor_animator_node_type", PROPERTY_HINT_ENUM, L"1D,2D,循环最后一个"), "set_animator_node_type", "get_animator_node_type");
	ADD_MEMBER_BUTTON(editor_create_animation_node, L"创建动画节点", CharacterAnimationLibrary);

#endif
}
void CharacterAnimationLibrary::set_animation_library(const TypedArray<CharacterAnimationLibraryItem> &p_animation_library) {
	if (animation_library.size() > 0) {
		return;
	}
	animation_library = p_animation_library;
}
TypedArray<CharacterAnimationLibraryItem> CharacterAnimationLibrary::get_animation_library() {
	return animation_library;
}

Ref<CharacterAnimationLibraryItem> CharacterAnimationLibrary::get_animation_by_name(StringName p_name) {
	if (animations.has(p_name)) {
		return animations[p_name];
	}
	if (base_library.is_valid()) {
		return base_library->get_animation_by_name(p_name);
	}
	ERR_PRINT(String("not find animation ") + p_name.operator String().utf8().get_data());
	return Ref<CharacterAnimationLibraryItem>();
}
void CharacterAnimationLibrary::init_animation_library() {
	if (is_init) {
		return;
	}
	for (int i = 0; i < animation_library.size(); i++) {
		Ref<CharacterAnimationLibraryItem> item = animation_library[i];
		if (item->get_name() != StringName()) {
			animations[item->get_name()] = item;
		}
	}
	is_init = true;
}
void CharacterAnimationLibrary::set_base_library(Ref<CharacterAnimationLibrary> p_base_library) {
	if (p_base_library.is_valid()) {
		Ref<CharacterAnimationLibrary> base = p_base_library;
		while (base.is_valid()) {
			if (base == this) {
				ERR_PRINT(L"循环引用 base library!");
				return;
			}
			base = base->base_library;
		}
	}
	base_library = p_base_library;
}

void CharacterAnimationLibrary::editor_create_animation_node() {
	if (get_path().is_empty()) {
		print_line(L"创建动画节点失败: 动画库必须存在路径,请先保存本动画库到磁盘");
		return;
	}
	if (animator_node_name.size() == 0) {
		print_line(L"创建动画节点失败: 动画节点名称不能为空");
		return;
	}
	String path = get_path().get_base_dir().path_join(animator_node_name + ".res");
	if (FileAccess::exists(path)) {
		for (int i = 0; i < animation_library.size(); i++) {
			Ref<CharacterAnimationLibraryItem> item = animation_library[i];
			if (item->get_name() == animator_node_name) {
				print_line(L"创建动画节点失败: 动画节点已经存在");
				return;
			}
		}
	}
	Ref<CharacterAnimatorNodeBase> anima_node;
	switch (animator_node_type) {
		case T_CharacterAnimatorNode1D:
			anima_node = memnew(CharacterAnimatorNode1D);
			break;
		case T_CharacterAnimatorNode2D:
			anima_node = memnew(CharacterAnimatorNode2D);
			break;
		case T_CharacterAnimatorLoopLast:
			anima_node = memnew(CharacterAnimatorLoopLast);
			break;
	}

	anima_node->set_name(animator_node_name);
	anima_node->set_path(path);
	ResourceSaver::save(anima_node, path);

	Ref<CharacterAnimationLibraryItem> item = memnew(CharacterAnimationLibraryItem);
	item->_set_node(anima_node);
	item->set_path(path);
	animation_library.push_back(item);
}

void CharacterAnimatorLayer::_process_logic(const Ref<Blackboard> &p_playback_info, double p_delta, bool is_first) {
	if (logic_context.animation_logic.is_null()) {
		return;
	}
	if (logic_context.curr_name == StringName()) {
		logic_context.curr_name = logic_context.animation_logic->get_default_state_name();
	}
	Ref<CharacterAnimationLogicNode> curr_logic = logic_context.curr_logic;

	bool change_state = false;
	if (curr_logic.is_null() || logic_context.curr_name != logic_context.last_name) {
		change_state = true;
	}

	else if (curr_logic.is_valid()) {
		if (curr_logic->check_stop(this, *p_playback_info)) {
			change_state = true;
		}
	}

	if (change_state) {
		curr_logic = logic_context.animation_logic->process_logic(logic_context.curr_name, *p_playback_info);
	}
	if (curr_logic.is_null()) {
		return;
	}
	if (curr_logic != logic_context.curr_logic) {
		logic_context.curr_logic->process_stop(this, *p_playback_info);
		logic_context.curr_logic = curr_logic;
		logic_context.curr_logic->process_start(this, *p_playback_info);
	}
	logic_context.last_name = logic_context.curr_name;
	curr_logic->process(this, *p_playback_info, p_delta);
}
// 处理动画
void CharacterAnimatorLayer::_process_animator(const Ref<Blackboard> &p_playback_info, double p_delta, bool is_first) {
	if (editor_stop_animation) {
		m_AnimationInstances.clear();
		editor_stop_animation = false;
	}
	//clear_animation_instances();

	// 重置一下手动处理线程安全标签
	set_is_manual_thread(true);
	Node *parent = get_node_or_null(root_node);
	if (parent) {
		parent->set_is_manual_thread(get_is_manual_thread());
	}
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(ObjectDB::get_instance(skeleton_id));
	update_tool->clear_cache(skeleton, parent);
	// 处理逻辑节点请求播放的动作
	if (logic_context.curr_animation.is_valid()) {
		auto anim = logic_context.curr_animation->get_node();
		if (play_animation(anim)) {
			logic_context.curr_animation_time_length = anim->_get_animation_length();
			logic_context.curr_animation_play_time = 0.0f;
		}
	}
	logic_context.curr_animation_play_time += p_delta;

	for (auto &anim : m_AnimationInstances) {
		if (anim.m_PlayState == CharacterAnimationInstance::PS_FadeOut) {
			anim.fadeTotalTime += p_delta;
		} else {
			anim.delta = p_delta;
			anim.time += p_delta;
			// 非淡出的动画需要更新事件
			anim.node->update_animation_time(&anim);
		}
	}
	auto it = m_AnimationInstances.begin();
	float total_weight = 0.0f;
	while (it != m_AnimationInstances.end()) {
		if (it->get_weight() == 0.0f) {
			it = m_AnimationInstances.erase(it);
		} else {
			total_weight += it->get_weight();
			++it;
		}
	}

	if (m_TotalAnimationWeight.size() < m_AnimationInstances.size()) {
		m_TotalAnimationWeight.resize(m_AnimationInstances.size());
	}
	for (auto &anim : m_AnimationInstances) {
		anim.node->process_animation(this, &anim, anim.get_weight() / total_weight, p_playback_info);
	}
}
void CharacterAnimatorLayer::init(Skeleton3D *p_skeleton, CharacterAnimator *p_animator, const Ref<CharacterAnimatorLayerConfig> &_config) {
	if (p_animator == nullptr) {
		m_Animator_ID = ObjectID();
	} else {
		m_Animator_ID = p_animator->get_instance_id();
	}
	config = _config;
	skeleton_id = ObjectID();
	if (p_skeleton != nullptr) {
		skeleton_id = p_skeleton->get_instance_id();
	}
}

void CharacterAnimatorLayer::play_animationm(const Ref<Animation> &p_anim, const PlaybackInfo &p_playback_info, const Dictionary &bone_map) {
	ERR_FAIL_COND(p_anim.is_null());
	AnimationData ad;
	ad.name = p_anim->get_name();
	ad.animation = p_anim;
	ad.bone_map = bone_map;
	//ad.animation_library = find_animation_library(ad.animation);

	AnimationInstance ai;
	ai.animation_data = ad;
	ai.playback_info = p_playback_info;
	update_tool->add_animation_instance(ai);
}
// 处理动画
void CharacterAnimatorLayer::_process_animation_item(const Ref<Blackboard> &p_playback_info, CharacterRootMotion &root_motion, HashMap<String, float> &bone_blend_weight, double p_delta, bool is_using_root_motion, bool is_first) {
	Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(ObjectDB::get_instance(skeleton_id));
	if (skeleton == nullptr) {
		return;
	}
	update_tool->process_animations(root_motion, bone_blend_weight, blend_weight);
	update_tool->layer_blend_apply(config, root_motion, bone_blend_weight, blend_weight, is_using_root_motion);

	Node *parent = get_node_or_null(root_node);
	if (parent) {
		cache_valid = false;
		parent->set_is_manual_thread(get_is_manual_thread());
	}
	set_is_manual_thread(false);
	return;
}

void CharacterAnimatorLayer::finish_update() {
}
void CharacterAnimatorLayer::play_animation(const Ref<Animation> &p_anim, bool p_is_loop) {
	Ref<CharacterAnimatorNode1D> anim_node;
	anim_node.instantiate();
	anim_node->add_animation(p_anim, 1);
	play_animation(anim_node);
}

bool CharacterAnimatorLayer::play_animation(const Ref<CharacterAnimatorNodeBase> &p_node) {
	if (p_node.is_null()) {
		return false;
	}
	if (m_AnimationInstances.size() > 0 && m_AnimationInstances.back()->get().node == p_node) {
		return false;
	}
	p_node->_init();
	for (auto &anim : m_AnimationInstances) {
		anim.m_PlayState = CharacterAnimationInstance::PS_FadeOut;
	}
	CharacterAnimationInstance ins;
	ins.node = p_node;
	ins.m_PlayState = CharacterAnimationInstance::PS_Play;
	if (config.is_valid() && config->get_mask().is_valid()) {
		ins.disable_path = config->get_mask()->disable_path;
	}
	m_AnimationInstances.push_back(ins);
	return true;
}

void CharacterAnimatorLayer::play_animation(const StringName &p_node_name) {
	CharacterAnimator *m_Animator = get_animation();
	if (m_Animator == nullptr) {
		return;
	}
	logic_context.curr_animation = m_Animator->get_animation_by_name(p_node_name);
}
void CharacterAnimatorLayer::change_state(const StringName &p_state_name) {
	if (logic_context.curr_name == p_state_name) {
		logic_context.last_name = logic_context.curr_name;
		logic_context.curr_name = p_state_name;
	}
}
void CharacterAnimatorLayer::on_destory() {
	m_Animator_ID = ObjectID();
}
CharacterAnimatorLayer::CharacterAnimatorLayer() {
	update_tool.instantiate();
}

CharacterAnimatorLayer::~CharacterAnimatorLayer() {
	CharacterAnimator *m_Animator = get_animation();
	if (m_Animator != nullptr) {
		m_Animator->on_layer_delete(this);
	}
}

void CharacterAnimatorLayerConfigInstance::editor_play_select_animation() {
	CharacterAnimatorLayer *layer = get_layer();
	if (layer == nullptr) {
		return;
	}
	layer->play_animation(play_animation, true);
}

void CharacterAnimatorLayerConfigInstance::set_body(class CharacterBodyMain *p_body) {
	CharacterAnimatorLayer *layer = get_layer();
	if (layer) {
		layer->on_destory();
		layer->queue_free();
		layer = nullptr;
	}
	if (p_body == nullptr) {
		m_Body_ID = ObjectID();
	} else {
		m_Body_ID = p_body->get_instance_id();
	}
	auto_init();
}
void CharacterAnimatorLayerConfigInstance::auto_init() {
	class CharacterBodyMain *m_Body = get_body();
	if (m_Body == nullptr || config.is_null()) {
		return;
	}
	Skeleton3D *skeleton = m_Body->get_skeleton();
	if (skeleton == nullptr) {
		return;
	}
	CharacterAnimatorLayer *layer = memnew(CharacterAnimatorLayer);
	m_Body->add_child(layer);
	layer->set_owner(m_Body);
	layer->init(skeleton, m_Body->get_animator().ptr(), config);
	layer_id = layer->get_instance_id();
}
CharacterAnimatorLayer *CharacterAnimatorLayerConfigInstance::get_layer() {
	if (layer_id.is_null()) {
		return nullptr;
	}

	return (CharacterAnimatorLayer *)Object::cast_to<CharacterAnimatorLayer>(ObjectDB::get_instance(layer_id));
}
class CharacterBodyMain *CharacterAnimatorLayerConfigInstance::get_body() {
	if (m_Body_ID.is_null()) {
		return nullptr;
	}
	return (CharacterBodyMain *)Object::cast_to<CharacterBodyMain>(ObjectDB::get_instance(m_Body_ID));
}
/**********************************************************************************************************/

void CharacterAnimator::set_body(class CharacterBodyMain *p_body) {
	if (p_body == nullptr) {
		m_Body_ID = ObjectID();
	} else {
		m_Body_ID = p_body->get_instance_id();
	}
	auto it = m_LayerConfigInstanceList.begin();
	while (it != m_LayerConfigInstanceList.end()) {
		Ref<CharacterAnimatorLayerConfigInstance> layer = *it;
		layer->set_body(p_body);
		++it;
	}
}
void CharacterAnimator::change_state(const StringName &p_state_name) {
	auto it = m_LayerConfigInstanceList.begin();
	while (it != m_LayerConfigInstanceList.end()) {
		(*it)->change_state(p_state_name);
		++it;
	}
}

void CharacterAnimator::on_layer_delete(CharacterAnimatorLayer *p_layer) {
	auto it = m_LayerConfigInstanceList.begin();
	while (it != m_LayerConfigInstanceList.end()) {
		if ((*it)->get_layer() == p_layer) {
			it = m_LayerConfigInstanceList.erase(it);
			break;
		} else {
			++it;
		}
	}
}

void CharacterAnimator::add_layer(const Ref<CharacterAnimatorLayerConfig> &_mask) {
	if (_mask.is_null()) {
		return;
	}
	Ref<CharacterAnimatorLayerConfigInstance> ins;
	ins.instantiate();
	ins->set_config(_mask);
	ins->set_body(get_body());
	m_LayerConfigInstanceList.push_back(ins);
}
void CharacterAnimator::_thread_update_animator(float delta) {
	time_delta = delta;
	class CharacterBodyMain *m_Body = get_body();
	if (m_Body == nullptr) {
		return;
	}
	auto it = m_LayerConfigInstanceList.begin();
	bool is_first = true;
	while (it != m_LayerConfigInstanceList.end()) {
		Ref<CharacterAnimatorLayerConfigInstance> layer = *it;
		layer->_process_animator(m_Body->get_blackboard(), delta, is_first);
		is_first = false;
		++it;
	}
}
void CharacterAnimator::_thread_update_animation(float delta) {
	class CharacterBodyMain *m_Body = get_body();
	if (m_Body == nullptr) {
		return;
	}
	auto it = m_LayerConfigInstanceList.begin();
	bool is_first = true;
	root_motion.reset();
	while (it != m_LayerConfigInstanceList.end()) {
		Ref<CharacterAnimatorLayerConfigInstance> layer = *it;
		layer->_process_animation_item(m_Body->get_blackboard(), root_motion, bone_blend_weight, delta, is_using_root_motion, is_first);
		is_first = false;
		++it;
	}
}
void CharacterAnimator::finish_update() {
	auto it = m_LayerConfigInstanceList.begin();
	//bool is_first = true;
	while (it != m_LayerConfigInstanceList.end()) {
		Ref<CharacterAnimatorLayerConfigInstance> layer = *it;
		layer->finish_update();
		//is_first = false;
		++it;
	}
}
Ref<CharacterAnimationLibraryItem> CharacterAnimator::get_animation_by_name(const StringName &p_name) {
	CharacterBodyMain *m_Body = get_body();
	if (m_Body == nullptr) {
		return Ref<CharacterAnimationLibraryItem>();
	}
	auto anim_lib = m_Body->get_animation_library();
	if (anim_lib.is_valid()) {
		return anim_lib->get_animation_by_name(p_name);
	}

	return Ref<CharacterAnimationLibraryItem>();
}

void CharacterAnimator::set_animation_layer_arrays(TypedArray<CharacterAnimatorLayerConfigInstance> p_animation_layer_arrays) {
	m_LayerConfigInstanceList.clear();
	CharacterBodyMain *m_Body = get_body();
	for (int i = 0; i < p_animation_layer_arrays.size(); ++i) {
		Ref<CharacterAnimatorLayerConfigInstance> ins = p_animation_layer_arrays[i];
		if (ins.is_null()) {
			ins.instantiate();
		}
		ins->set_body(m_Body);
		m_LayerConfigInstanceList.push_back(ins);
	}
}
TypedArray<CharacterAnimatorLayerConfigInstance> CharacterAnimator::get_animation_layer_arrays() {
	TypedArray<CharacterAnimatorLayerConfigInstance> rs;
	auto it = m_LayerConfigInstanceList.begin();
	while (it != m_LayerConfigInstanceList.end()) {
		rs.append(*it);
		++it;
	}
	return rs;
}
void CharacterAnimator::init() {
	if (m_LayerConfigInstanceList.size() == 0) {
		Ref<CharacterAnimatorLayerConfig> _mask;
		_mask.instantiate();
		add_layer(_mask);
	}
}

void CharacterAnimator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_animation_layer_arrays", "animation_layer_arrays"), &CharacterAnimator::set_animation_layer_arrays);
	ClassDB::bind_method(D_METHOD("get_animation_layer_arrays"), &CharacterAnimator::get_animation_layer_arrays);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "animation_layer_arrays"), "set_animation_layer_arrays", "get_animation_layer_arrays");
}

CharacterAnimator::~CharacterAnimator() {
	set_body(nullptr);
	m_LayerConfigInstanceList.clear();
}
class CharacterBodyMain *CharacterAnimator::get_body() {
	if (m_Body_ID.is_null()) {
		return nullptr;
	}
	return Object::cast_to<CharacterBodyMain>(ObjectDB::get_instance(m_Body_ID));
}

//////////////////////////////////////////////// CharacterAnimationLogicNode /////////////////////////////////////////

void CharacterAnimationLogicNode::set_blackboard_plan(const Ref<BlackboardPlan> &p_blackboard_plan) {
	if (p_blackboard_plan == blackboard_plan) {
		return;
	}
	if (blackboard_plan.is_valid()) {
		blackboard_plan->disconnect("changed", callable_mp(this, &CharacterAnimationLogicNode::_blackboard_changed));
	}
	blackboard_plan = p_blackboard_plan;
	if (blackboard_plan.is_valid()) {
		blackboard_plan->connect("changed", callable_mp(this, &CharacterAnimationLogicNode::_blackboard_changed));
	}
	init_blackboard(blackboard_plan);
	update_blackboard_plan();
}
void CharacterAnimationLogicNode::update_blackboard_plan() {
	if (enter_condtion.is_valid()) {
		enter_condtion->set_blackboard_plan(blackboard_plan);
	}
	if (stop_check_condtion.is_valid()) {
		stop_check_condtion->set_blackboard_plan(blackboard_plan);
	}
	if (start_blackboard_set.is_valid()) {
		start_blackboard_set->set_blackboard_plan(blackboard_plan);
	}

	if (stop_blackboard_set.is_valid()) {
		stop_blackboard_set->set_blackboard_plan(blackboard_plan);
	}
}

void CharacterAnimationLogicNode::process_start(CharacterAnimatorLayer *animator, Blackboard *blackboard) {
	if (start_blackboard_set.is_valid()) {
		start_blackboard_set->execute(blackboard);
	}
	// 播放动作
	animator->play_animation(player_animation_name);
	if (GDVIRTUAL_IS_OVERRIDDEN(_animation_process_start)) {
		GDVIRTUAL_CALL(_animation_process_start, animator, blackboard);
		return;
	}
}

void CharacterAnimationLogicNode::process(CharacterAnimatorLayer *animator, Blackboard *blackboard, double delta) {
	if (GDVIRTUAL_IS_OVERRIDDEN(_animation_process)) {
		GDVIRTUAL_CALL(_animation_process, animator, blackboard, delta);
		return;
	}
}

void CharacterAnimationLogicNode::process_stop(CharacterAnimatorLayer *animator, Blackboard *blackboard) {
	if (stop_blackboard_set.is_valid()) {
		stop_blackboard_set->execute(blackboard);
	}
	if (GDVIRTUAL_IS_OVERRIDDEN(_animation_process_stop)) {
		GDVIRTUAL_CALL(_animation_process_stop, animator, blackboard);
		return;
	}
}
bool CharacterAnimationLogicNode::check_stop(CharacterAnimatorLayer *animator, Blackboard *blackboard) {
	auto context = animator->_get_logic_context();
	if (context->time < check_stop_delay_time) {
		return false;
	}
	if (GDVIRTUAL_IS_OVERRIDDEN(_check_stop)) {
		bool is_stop = false;
		GDVIRTUAL_CALL(_check_stop, animator, blackboard, is_stop);
		return is_stop;
	}
	if (stop_check_type == Life) {
		return (life_time >= context->time);
	} else if (stop_check_type == AnimationLengthScale) {
		return (context->curr_animation_play_time / context->curr_animation_time_length >= anmation_scale);
	} else {
		if (stop_check_condtion.is_valid()) {
			return stop_check_condtion->is_enable(blackboard);
		}
	}
	return true;
}

void CharacterAnimationLogicNode::init_blackboard(Ref<BlackboardPlan> p_blackboard_plan) {
	Ref<BlackboardPlan> blackboard_plan = p_blackboard_plan;
	if (blackboard_plan.is_null()) {
		return;
	}
	if (!blackboard_plan->has_var("OldForward")) {
		blackboard_plan->add_var("OldForward", BBVariable(Variant::VECTOR3, Vector3()));
	}

	if (!blackboard_plan->has_var("CurrForward")) {
		blackboard_plan->add_var("CurrForward", BBVariable(Variant::VECTOR3, Vector3()));
	}

	if (!blackboard_plan->has_var("MoveTarget")) {
		blackboard_plan->add_var("MoveTarget", BBVariable(Variant::VECTOR3, Vector3()));
	}

	if (!blackboard_plan->has_var("CurrState")) {
		blackboard_plan->add_var("CurrState", BBVariable(Variant::STRING_NAME, StringName()));
	}

	if (!blackboard_plan->has_var("HorizontalMovement")) {
		blackboard_plan->add_var("HorizontalMovement", BBVariable(Variant::FLOAT, 0.0f));
	}

	if (!blackboard_plan->has_var("VerticalMovement")) {
		blackboard_plan->add_var("VerticalMovement", BBVariable(Variant::FLOAT, 0.0f));
	}

	if (!blackboard_plan->has_var("Pitch")) {
		blackboard_plan->add_var("Pitch", BBVariable(Variant::FLOAT, 0.0f));
	}
	if (!blackboard_plan->has_var("Yaw")) {
		blackboard_plan->add_var("Yaw", BBVariable(Variant::FLOAT, 0.0f));
	}
	if (!blackboard_plan->has_var("Speed")) {
		blackboard_plan->add_var("Speed", BBVariable(Variant::FLOAT, 0.0f));
	}

	// 是否使用能力
	if (!blackboard_plan->has_var("IsAbility")) {
		blackboard_plan->add_var("IsAbility", BBVariable(Variant::BOOL, false));
	}
	if (!blackboard_plan->has_var("AbilityIndex")) {
		blackboard_plan->add_var("AbilityIndex", BBVariable(Variant::INT, 0));
	}
	if (!blackboard_plan->has_var("AbilityIntData")) {
		blackboard_plan->add_var("AbilityIntData", BBVariable(Variant::INT, 0));
	}
	if (!blackboard_plan->has_var("AbilityFloatData")) {
		blackboard_plan->add_var("AbilityFloatData", BBVariable(Variant::FLOAT, 0.0f));
	}

	if (!blackboard_plan->has_var("IsGround")) {
		blackboard_plan->add_var("IsGround", BBVariable(Variant::BOOL, false));
	}
	if (!blackboard_plan->has_var("IsMoving")) {
		blackboard_plan->add_var("IsMoving", BBVariable(Variant::BOOL, false));
	}

	if (!blackboard_plan->has_var("IsJump")) {
		blackboard_plan->add_var("IsJump", BBVariable(Variant::BOOL, false));
	}
	if (!blackboard_plan->has_var("IsCrouch")) {
		blackboard_plan->add_var("IsCrouch", BBVariable(Variant::BOOL, false));
	}
	if (!blackboard_plan->has_var("IsAttack")) {
		blackboard_plan->add_var("IsAttack", BBVariable(Variant::BOOL, false));
	}
	if (!blackboard_plan->has_var("IsDead")) {
		blackboard_plan->add_var("IsDead", BBVariable(Variant::BOOL, false));
	}
	if (!blackboard_plan->has_var("LegIndex")) {
		blackboard_plan->add_var("LegIndex", BBVariable(Variant::INT, 0));
	}
	if (!blackboard_plan->has_var("IdentityIndex")) {
		blackboard_plan->add_var("IdentityIndex", BBVariable(Variant::INT, 0));
	}

	// AI 大腦更新頻率
	if (!blackboard_plan->has_var("AI_BrainUpdate_Rate")) {
		blackboard_plan->add_var("AI_BrainUpdate_Rate", BBVariable(Variant::FLOAT, 1.0f));
	}
}

bool CharacterAnimationLogicNode::is_enter(Blackboard *blackboard) {
	if (enter_condtion.is_valid()) {
		return enter_condtion->is_enable(blackboard);
	}
	return true;
}
CharacterAnimationLogicNode::~CharacterAnimationLogicNode() {
	if (blackboard_plan.is_valid()) {
		blackboard_plan->disconnect("changed", callable_mp(this, &CharacterAnimationLogicNode::_blackboard_changed));
	}
}

void CharacterAnimationLogicNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_blackboard_plan", "blackboard_plan"), &CharacterAnimationLogicNode::set_blackboard_plan);
	ClassDB::bind_method(D_METHOD("get_blackboard_plan"), &CharacterAnimationLogicNode::get_blackboard_plan);

	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &CharacterAnimationLogicNode::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &CharacterAnimationLogicNode::get_priority);

	ClassDB::bind_method(D_METHOD("set_player_animation_name", "player_animation_name"), &CharacterAnimationLogicNode::set_player_animation_name);
	ClassDB::bind_method(D_METHOD("get_player_animation_name"), &CharacterAnimationLogicNode::get_player_animation_name);

	ClassDB::bind_method(D_METHOD("set_enter_condtion", "enter_condtion"), &CharacterAnimationLogicNode::set_enter_condtion);
	ClassDB::bind_method(D_METHOD("get_enter_condtion"), &CharacterAnimationLogicNode::get_enter_condtion);

	ClassDB::bind_method(D_METHOD("set_start_blackboard_set", "start_blackboard_set"), &CharacterAnimationLogicNode::set_start_blackboard_set);
	ClassDB::bind_method(D_METHOD("get_start_blackboard_set"), &CharacterAnimationLogicNode::get_start_blackboard_set);

	ClassDB::bind_method(D_METHOD("set_stop_blackboard_set", "stop_blackboard_set"), &CharacterAnimationLogicNode::set_stop_blackboard_set);
	ClassDB::bind_method(D_METHOD("get_stop_blackboard_set"), &CharacterAnimationLogicNode::get_stop_blackboard_set);

	ClassDB::bind_method(D_METHOD("set_check_stop_delay_time", "check_stop_delay_time"), &CharacterAnimationLogicNode::set_check_stop_delay_time);
	ClassDB::bind_method(D_METHOD("get_check_stop_delay_time"), &CharacterAnimationLogicNode::get_check_stop_delay_time);

	ClassDB::bind_method(D_METHOD("set_life_time", "life_time"), &CharacterAnimationLogicNode::set_life_time);
	ClassDB::bind_method(D_METHOD("get_life_time"), &CharacterAnimationLogicNode::get_life_time);

	ClassDB::bind_method(D_METHOD("set_stop_check_type", "stop_check_type"), &CharacterAnimationLogicNode::set_stop_check_type);
	ClassDB::bind_method(D_METHOD("get_stop_check_type"), &CharacterAnimationLogicNode::get_stop_check_type);

	ClassDB::bind_method(D_METHOD("set_stop_check_condtion", "stop_check_condtion"), &CharacterAnimationLogicNode::set_stop_check_condtion);
	ClassDB::bind_method(D_METHOD("get_stop_check_condtion"), &CharacterAnimationLogicNode::get_stop_check_condtion);

	ClassDB::bind_method(D_METHOD("set_stop_check_anmation_length_scale", "stop_check_anmation_length_scale"), &CharacterAnimationLogicNode::set_stop_check_anmation_length_scale);
	ClassDB::bind_method(D_METHOD("get_stop_check_anmation_length_scale"), &CharacterAnimationLogicNode::get_stop_check_anmation_length_scale);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blackboard_plan", PROPERTY_HINT_RESOURCE_TYPE, "BlackboardPlan"), "set_blackboard_plan", "get_blackboard_plan");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "player_animation_name"), "set_player_animation_name", "get_player_animation_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "enter_condtion", PROPERTY_HINT_RESOURCE_TYPE, "CharacterAnimatorCondition"), "set_enter_condtion", "get_enter_condtion");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "start_blackboard_set", PROPERTY_HINT_RESOURCE_TYPE, "AnimatorBlackboardSet"), "set_start_blackboard_set", "get_start_blackboard_set");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stop_blackboard_set", PROPERTY_HINT_RESOURCE_TYPE, "AnimatorBlackboardSet"), "set_stop_blackboard_set", "get_stop_blackboard_set");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "check_stop_delay_time"), "set_check_stop_delay_time", "get_check_stop_delay_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "life_time"), "set_life_time", "get_life_time");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stop_check_type", PROPERTY_HINT_ENUM, "Life,PlayCount,Condition,Script"), "set_stop_check_type", "get_stop_check_type");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stop_check_condtion", PROPERTY_HINT_RESOURCE_TYPE, "CharacterAnimatorCondition"), "set_stop_check_condtion", "get_stop_check_condtion");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stop_check_anmation_length_scale"), "set_stop_check_anmation_length_scale", "get_stop_check_anmation_length_scale");

	GDVIRTUAL_BIND(_animation_process_start, "_layer", "_blackboard");
	GDVIRTUAL_BIND(_animation_process_stop, "_layer", "_blackboard");
	GDVIRTUAL_BIND(_animation_process, "_layer", "_blackboard", "_delta");
	GDVIRTUAL_BIND(_check_stop, "_layer", "_blackboard");

	BIND_ENUM_CONSTANT(Life);
	BIND_ENUM_CONSTANT(AnimationLengthScale);
	BIND_ENUM_CONSTANT(Condition);
	BIND_ENUM_CONSTANT(Script);
}
