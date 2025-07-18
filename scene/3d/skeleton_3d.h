/**************************************************************************/
/*  skeleton_3d.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/templates/a_hash_map.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/skin.h"

typedef int BoneId;
namespace human_anim {
namespace human {
struct Human;
}
} //namespace human_anim

struct BonePose {
	int bone_index;
	Vector3 position;
	Quaternion rotation;
	// 虛擬子骨骼的前方
	Vector3 right;
	Transform3D global_pose;
	Transform3D local_pose;
	Vector<StringName> child_bones;

	void set_bone_forward();

	void load(Dictionary &aDict);
	void save(Dictionary &aDict);
	void clear();

public:
	static Vector4 get_root_lookat(const Basis &rest_rotation, const Basis &curr_rotation, const Vector3 &forward, const Vector3 &right);
	// xyz 是世界位置,,我是自身轴旋转角度
	Vector4 get_look_at_and_roll(const Transform3D &p_parent_trans, Basis &p_curr_basis, Transform3D &p_curr_global_trans);
	// 重定向骨骼
	void retarget(const Transform3D &parent_trans, const Vector4 &lookat, Transform3D &out_global_trans, Basis &local_rotation);
};

class HumanBoneConfig : public Resource {
	GDCLASS(HumanBoneConfig, Resource);

	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_data", "data"), &HumanBoneConfig::set_data);
		ClassDB::bind_method(D_METHOD("get_data"), &HumanBoneConfig::get_data);

		ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_data", "get_data");
	}

public:
	void load(Dictionary &aDict) {
		clear();
		Dictionary pose = aDict["virtual_pose"];
		auto keys = pose.keys();
		for (auto &it : pose) {
			Dictionary dict = it.value;
			virtual_pose[it.key].load(dict);
		}

		Vector<String> root = aDict["root_bone"];
		for (int i = 0; i < root.size(); i++) {
			root_bone.push_back(StringName(root[i]));
		}
	}

	void save(Dictionary &aDict) {
		Dictionary pose;
		for (auto &it : virtual_pose) {
			Dictionary dict;
			it.value.save(dict);
			pose[it.key] = dict;
		}
		aDict["virtual_pose"] = pose;
		Vector<String> root;
		for (int i = 0; i < root_bone.size(); i++) {
			root.push_back(root_bone[i]);
		}
		aDict["root_bone"] = root;
	}

	void clear() {
		virtual_pose.clear();
		root_bone.clear();
	}

	void set_data(Dictionary aDict) {
		load(aDict);
	}
	Dictionary get_data() {
		Dictionary dict;
		save(dict);
		return dict;
	}

public:
	// 虚拟姿勢
	HashMap<StringName, BonePose> virtual_pose;

	Vector<StringName> root_bone;
};

class Skeleton3D;

class SkinReference : public RefCounted {
	GDCLASS(SkinReference, RefCounted)
	friend class Skeleton3D;

	Skeleton3D *skeleton_node = nullptr;
	RID skeleton;
	Ref<Skin> skin;
	uint32_t bind_count = 0;
	uint64_t skeleton_version = 0;
	Vector<uint32_t> skin_bone_indices;
	uint32_t *skin_bone_indices_ptrs = nullptr;

protected:
	static void _bind_methods();

public:
	// Public for use with callable_mp.
	void _skin_changed();

	RID get_skeleton() const;
	Ref<Skin> get_skin() const;
	~SkinReference();
};

class Skeleton3D : public Node3D {
	GDCLASS(Skeleton3D, Node3D);

#ifdef TOOLS_ENABLED
	bool saving = false;
#endif //TOOLS_ENABLED

#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
	bool animate_physical_bones = true;
	Node *simulator = nullptr;
	void setup_simulator();
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED

public:
	enum ModifierCallbackModeProcess {
		MODIFIER_CALLBACK_MODE_PROCESS_PHYSICS,
		MODIFIER_CALLBACK_MODE_PROCESS_IDLE,
		MODIFIER_CALLBACK_MODE_PROCESS_MANUAL,
	};

private:
	friend class SkinReference;

	enum UpdateFlag {
		UPDATE_FLAG_NONE = 1,
		UPDATE_FLAG_MODIFIER = 2,
		UPDATE_FLAG_POSE = 4,
	};

	void _update_deferred(UpdateFlag p_update_flag = UPDATE_FLAG_POSE);
	uint8_t update_flags = UPDATE_FLAG_NONE;
	bool updating = false; // Is updating now?
	double update_delta = 0.0;

	struct Bone {
		String name;

		int parent = -1;
		Vector<int> child_bones;

		Transform3D rest;
		Transform3D global_rest;

		bool enabled = true;
		bool pose_cache_dirty = true;
		// 是否为人形骨骼
		bool is_human_bone = false;
		Transform3D pose_cache;
		Vector3 pose_position;
		Quaternion pose_rotation;
		Vector3 pose_scale = Vector3(1, 1, 1);
		Transform3D global_pose;
		int nested_set_offset = 0; // Offset in nested set of bone hierarchy.
		int nested_set_span = 0; // Subtree span in nested set of bone hierarchy.

		void update_pose_cache() {
			if (pose_cache_dirty) {
				pose_cache.basis.set_quaternion_scale(pose_rotation, pose_scale);
				pose_cache.origin = pose_position;
				pose_cache_dirty = false;
			}
		}

		HashMap<StringName, Variant> metadata;

#ifndef DISABLE_DEPRECATED
		Transform3D pose_global_no_override;
		real_t global_pose_override_amount = 0.0;
		bool global_pose_override_reset = false;
		Transform3D global_pose_override;
#endif // _DISABLE_DEPRECATED
	};

	struct BonePoseBackup {
		Transform3D pose_cache;
		Vector3 pose_position;
		Quaternion pose_rotation;
		Vector3 pose_scale = Vector3(1, 1, 1);
		Transform3D global_pose;

		void save(const Bone &p_bone) {
			pose_cache = p_bone.pose_cache;
			pose_position = p_bone.pose_position;
			pose_rotation = p_bone.pose_rotation;
			pose_scale = p_bone.pose_scale;
			global_pose = p_bone.global_pose;
		}

		void restore(Bone &r_bone) {
			r_bone.pose_cache = pose_cache;
			r_bone.pose_position = pose_position;
			r_bone.pose_rotation = pose_rotation;
			r_bone.pose_scale = pose_scale;
			r_bone.global_pose = global_pose;
		}
	};

	HashSet<SkinReference *> skin_bindings;
	void _skin_changed();

	mutable LocalVector<Bone> bones;
	mutable bool process_order_dirty = false;

	mutable Vector<int> parentless_bones;
	AHashMap<String, int> name_to_bone_index;

	mutable StringName concatenated_bone_names;
	void _update_bone_names() const;

public:
	void _make_dirty();
	mutable bool dirty = false;
	mutable bool rest_dirty = false;

	bool show_rest_only = false;
	float motion_scale = 1.0;
	bool _notification_bone_pose = true;

	void set_notification_bone_pose(bool p_enable) {
		_notification_bone_pose = p_enable;
	}

	bool get_notification_bone_pose() const {
		return _notification_bone_pose;
	}

	uint64_t version = 1;

	void _update_process_order() const;

	// To process modifiers.
	ModifierCallbackModeProcess modifier_callback_mode_process = MODIFIER_CALLBACK_MODE_PROCESS_IDLE;
	LocalVector<ObjectID> modifiers;
	Ref<HumanBoneConfig> human_config;
	bool modifiers_dirty = false;
	void _find_modifiers();
	void _process_modifiers();
	void _process_changed();
	void _make_modifiers_dirty();

	// Global bone pose calculation.
	mutable LocalVector<int> nested_set_offset_to_bone_index; // Map from Bone::nested_set_offset to bone index.
	mutable LocalVector<bool> bone_global_pose_dirty; // Indexable with Bone::nested_set_offset.
	void _update_bones_nested_set() const;
	int _update_bone_nested_set(int p_bone, int p_offset) const;
	void _make_bone_global_poses_dirty() const;
	void _make_bone_global_pose_subtree_dirty(int p_bone) const;
	void _update_bone_global_pose(int p_bone) const;

#ifndef DISABLE_DEPRECATED
	void _add_bone_bind_compat_88791(const String &p_name);

	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

protected:
	bool _get(const StringName &p_path, Variant &r_ret) const;
	bool _set(const StringName &p_path, const Variant &p_value);
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void _notification(int p_what);
	TypedArray<StringName> _get_bone_meta_list_bind(int p_bone) const;
	static void _bind_methods();

	virtual void add_child_notify(Node *p_child) override;
	virtual void move_child_notify(Node *p_child) override;
	virtual void remove_child_notify(Node *p_child) override;

public:
	enum {
		NOTIFICATION_UPDATE_SKELETON = 50
	};

	// Skeleton creation API
	uint64_t get_version() const;
	int add_bone(const String &p_name);
	void remove_bone(int p_bone);
	int find_bone(const String &p_name) const;
	String get_bone_name(int p_bone) const;
	void set_bone_name(int p_bone, const String &p_name);
	StringName get_concatenated_bone_names() const;

	bool is_bone_parent_of(int p_bone_id, int p_parent_bone_id) const;

	void set_bone_parent(int p_bone, int p_parent);
	int get_bone_parent(int p_bone) const;

	void unparent_bone_and_rest(int p_bone);

	Vector<int> get_bone_children(int p_bone) const;
	Vector<int> get_parentless_bones() const;

	Vector<int> get_root_bones() const;

	Dictionary get_human_bone_mapping();
	void set_human_bone_mapping(const Dictionary &p_human_bone_mapping);

	Vector<String> get_bone_names() const;

	int get_bone_count() const;

	void set_bone_rest(int p_bone, const Transform3D &p_rest);
	Transform3D get_bone_rest(int p_bone) const;
	Transform3D get_bone_global_rest(int p_bone) const;

	void set_bone_enabled(int p_bone, bool p_enabled);
	bool is_bone_enabled(int p_bone) const;

	void set_show_rest_only(bool p_enabled);
	bool is_show_rest_only() const;
	void clear_bones();

	void set_motion_scale(float p_motion_scale);
	float get_motion_scale() const;

	// bone metadata
	Variant get_bone_meta(int p_bone, const StringName &p_key) const;
	void get_bone_meta_list(int p_bone, List<StringName> *p_list) const;
	bool has_bone_meta(int p_bone, const StringName &p_key) const;
	void set_bone_meta(int p_bone, const StringName &p_key, const Variant &p_value);

	// Posing API
	Transform3D get_bone_pose(int p_bone) const;
	Vector3 get_bone_pose_position(int p_bone) const;
	Quaternion get_bone_pose_rotation(int p_bone) const;
	Vector3 get_bone_pose_scale(int p_bone) const;
	bool is_human_bone(int p_bone) const;
	void set_bone_pose(int p_bone, const Transform3D &p_pose,bool p_notify = true);
	void set_bone_pose_position(int p_bone, const Vector3 &p_position, bool p_notify = true);
	void set_bone_pose_rotation(int p_bone, const Quaternion &p_rotation, bool p_notify = true);
	void set_bone_pose_scale(int p_bone, const Vector3 &p_scale, bool p_notify = true);
	void set_human_bone(int p_bone, bool p_is_human_bone);

	Transform3D get_bone_global_pose(int p_bone) const;
	void set_bone_global_pose(int p_bone, const Transform3D &p_pose);
	void change_to_human_bone(int p_bone);

	void reset_bone_pose(int p_bone);
	void reset_bone_poses();

	void localize_rests(); // Used for loaders and tools.

	Ref<Skin> create_skin_from_rest_transforms();

	Ref<SkinReference> register_skin(const Ref<Skin> &p_skin);

	void force_update_all_dirty_bones(bool p_notify = true);
	void _force_update_all_dirty_bones(bool p_notify = true) const;
	void force_update_all_bone_transforms(bool p_notify = true);
	void _force_update_all_bone_transforms(bool p_notify = true) const;
	void force_update_bone_children_transforms(int bone_idx);
	void _force_update_bone_children_transforms(int bone_idx) const;
	void force_update_deferred();

	void set_modifier_callback_mode_process(ModifierCallbackModeProcess p_mode);
	ModifierCallbackModeProcess get_modifier_callback_mode_process() const;

	void set_human_config(const Ref<HumanBoneConfig> &p_human_config);
	Ref<HumanBoneConfig> get_human_config() const;

	void advance(double p_delta);

#ifndef DISABLE_DEPRECATED
	Transform3D get_bone_global_pose_no_override(int p_bone) const;
	void clear_bones_global_pose_override();
	Transform3D get_bone_global_pose_override(int p_bone) const;
	void set_bone_global_pose_override(int p_bone, const Transform3D &p_pose, real_t p_amount, bool p_persistent = false);

	Node *get_simulator();
#ifndef PHYSICS_3D_DISABLED
	void set_animate_physical_bones(bool p_enabled);
	bool get_animate_physical_bones() const;
	void physical_bones_stop_simulation();
	void physical_bones_start_simulation_on(const TypedArray<StringName> &p_bones);
	void physical_bones_add_collision_exception(RID p_exception);
	void physical_bones_remove_collision_exception(RID p_exception);
#endif // PHYSICS_3D_DISABLED
#endif // _DISABLE_DEPRECATED

public:
	Skeleton3D();
	~Skeleton3D();
};

VARIANT_ENUM_CAST(Skeleton3D::ModifierCallbackModeProcess);
