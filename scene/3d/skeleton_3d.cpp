/**************************************************************************/
/*  skeleton_3d.cpp                                                       */
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

#include "skeleton_3d.h"
#include "skeleton_3d.compat.inc"

#include "scene/3d/skeleton_modifier_3d.h"
#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
#include "scene/3d/physics/physical_bone_simulator_3d.h"
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED

void BonePose::set_bone_forward() {
	right = position.normalized();
	Vector3::Axis min_axis = right.min_axis_index();

	Vector3 up = Vector3(0, 1, 0);
	switch (min_axis) {
		case Vector3::AXIS_X:
			up = Vector3(1, 0, 0);
			break;
		case Vector3::AXIS_Y:
			up = Vector3(0, 1, 0);
			break;
		case Vector3::AXIS_Z:
			up = Vector3(0, 0, 1);
			break;
	}
	right = up.cross(position);
	right.normalize();
}

void BonePose::load(Dictionary &aDict) {
	clear();
	bone_index = aDict["bone_index"];
	position = aDict["position"];
	rotation = aDict["rotation"];
	right = aDict["right"];
	local_pose = Transform3D(Basis(rotation), position);
	Vector<String> child = aDict["child_bones"];
	for (int i = 0; i < child.size(); i++) {
		child_bones.push_back(StringName(child[i]));
	}
}
void BonePose::save(Dictionary &aDict) {
	aDict["bone_index"] = bone_index;
	aDict["position"] = position;
	aDict["rotation"] = rotation;
	aDict["right"] = right;
	Vector<String> child;
	for (int i = 0; i < child_bones.size(); i++) {
		child.push_back(child_bones[i]);
	}
	aDict["child_bones"] = child;
}
void BonePose::clear() {
	child_bones.clear();
	bone_index = -1;
	position = Vector3();
	rotation = Quaternion();
	right = Vector3();
}
Vector4 BonePose::get_root_lookat(const Basis &rest_rotation, const Basis &curr_rotation, const Vector3 &forward, const Vector3 &right) {
	Vector4 lookat;
	Basis diff_rotation = rest_rotation.inverse() * curr_rotation;
	Vector3 new_forward = diff_rotation.xform(forward);

	// 计算沿着自身轴的旋转角度
	{
		Basis new_basis;
		new_basis.rotate_to_align(forward, new_forward);
		//
		Vector3 org_rest_right = new_basis.xform(right);

		Vector3 new_right = diff_rotation.xform(right);

		Plane plane = Plane(new_forward, 0.0);
		Vector3 intersect;
		plane.intersects_ray(new_right - new_forward, -new_forward, &intersect);

		if (intersect.x + intersect.y + intersect.z == 0) {
			lookat.w = 0;
		} else {
			float angle = org_rest_right.signed_angle_to(intersect.normalized(), new_forward);
			lookat.w = angle;
		}
	}
	lookat.x = new_forward.x;
	lookat.y = new_forward.y;
	lookat.z = new_forward.z;
	return lookat;
}
// xyz 是世界位置,,我是自身轴旋转角度
Vector4 BonePose::get_look_at_and_roll(const Transform3D &p_parent_trans, Basis &p_curr_basis, Transform3D &p_curr_global_trans) {
	p_curr_global_trans = p_parent_trans * Transform3D(p_curr_basis, position);
	// 计算出观察方向
	//Vector3 lookat = p_curr_global_trans.xform(forward);

	Vector4 ret;
	//{
	//	Transform3D new_rest = p_parent_trans * Transform3D(Basis(rotation), position);
	//	Vector3 rest_forward = new_rest.basis.xform(forward);

	//	// 初始状态直接对齐新的观察点得到预测后不带有自身轴旋转的新的右方向
	//	Vector3 org_rest_right = new_rest.basis.xform(right);

	//	// 计算原始动画姿势计算后的右方向朝向
	//	Vector3 new_right = p_curr_global_trans.basis.xform(right);

	//	Plane plane = Plane(lookat - p_curr_global_trans.origin, 0.0);
	//	Vector3 intersect;
	//	plane.intersects_ray(new_right - plane.normal,-plane.normal, &intersect);

	//	// 计算自身轴的旋转角度
	//	if(intersect.x + intersect.y + intersect.z == 0)
	//	{
	//		ret.w = 0;
	//	}
	//	else {
	//		float angle = org_rest_right.signed_angle_to(intersect.normalized(), org_rest_right);
	//		ret.w = angle;
	//	}
	//
	//}
	//ret.x = lookat.x;
	//ret.y = lookat.y;
	//ret.z = lookat.z;
	return ret;
}
// 重定向骨骼
void BonePose::retarget(const Transform3D &parent_trans, const Vector4 &lookat, Transform3D &out_global_trans, Basis &local_rotation) {
	// 重定向骨骼的世界坐标
	//out_global_trans = parent_trans * local_pose;
	//if (forward.dot(forward) == 0)
	//{
	//	local_rotation = local_pose.basis;
	//	return;
	//}
	//Vector3 rest_forward = out_global_trans.basis.xform(forward);
	//Vector3 new_forward = Vector3(lookat.x, lookat.y, lookat.z) - out_global_trans.origin;
	//// 朝向观察点
	//out_global_trans.basis.rotate_to_align(rest_forward, new_forward);
	//// if (lookat.w != 0) {
	//// 	// 计算自身轴旋转
	//// 	Vector3 new_forward = out_global_trans.basis.xform(forward);
	//// 	out_global_trans.basis.rotate(new_forward, lookat.w);
	//// }
	//// 计算出本地旋转
	//local_rotation = parent_trans.basis.inverse() * out_global_trans.basis;
}
/**********************************************************************************************************************/

void SkinReference::_skin_changed() {
	if (skeleton_node) {
		skeleton_node->_make_dirty();
	}
	skeleton_version = 0;
}

void SkinReference::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_skeleton"), &SkinReference::get_skeleton);
	ClassDB::bind_method(D_METHOD("get_skin"), &SkinReference::get_skin);
}

RID SkinReference::get_skeleton() const {
	return skeleton;
}

Ref<Skin> SkinReference::get_skin() const {
	return skin;
}

SkinReference::~SkinReference() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	if (skeleton_node) {
		skeleton_node->skin_bindings.erase(this);
	}
	RS::get_singleton()->free(skeleton);
}

///////////////////////////////////////

bool Skeleton3D::_set(const StringName &p_path, const Variant &p_value) {
#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
	if (p_path == SNAME("animate_physical_bones")) {
		set_animate_physical_bones(p_value);
		return true;
	}
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED
	String path = p_path;

	if (!path.begins_with("bones/")) {
		return false;
	}

	uint32_t which = path.get_slicec('/', 1).to_int();
	String what = path.get_slicec('/', 2);

	if (which == bones.size() && what == "name") {
		add_bone(p_value);
		return true;
	}

	ERR_FAIL_UNSIGNED_INDEX_V(which, bones.size(), false);

	if (what == "parent") {
		set_bone_parent(which, p_value);
	} else if (what == "rest") {
		set_bone_rest(which, p_value);
	} else if (what == "enabled") {
		set_bone_enabled(which, p_value);
	} else if (what == "position") {
		set_bone_pose_position(which, p_value);
	} else if (what == "rotation") {
		set_bone_pose_rotation(which, p_value);
	} else if (what == "scale") {
		set_bone_pose_scale(which, p_value);
	} else if (what == "is_human_bone") {
		set_human_bone(which, p_value);
	} else if (what == "bone_meta") {
		set_bone_meta(which, path.get_slicec('/', 3), p_value);
#ifndef DISABLE_DEPRECATED
	} else if (what == "pose" || what == "bound_children") {
		// Kept for compatibility from 3.x to 4.x.
		WARN_DEPRECATED_MSG("Skeleton uses old pose format, which is deprecated (and loads slower). Consider re-importing or re-saving the scene." +
				(is_inside_tree() ? vformat(" Path: \"%s\"", get_path()) : String()));
		if (what == "pose") {
			// Old Skeleton poses were relative to rest, new ones are absolute, so we need to recompute the pose.
			// Skeleton3D nodes were always written with rest before pose, so this *SHOULD* work...
			Transform3D rest = get_bone_rest(which);
			Transform3D pose = rest * (Transform3D)p_value;
			set_bone_pose_position(which, pose.origin);
			set_bone_pose_rotation(which, pose.basis.get_rotation_quaternion());
			set_bone_pose_scale(which, pose.basis.get_scale());
		} else { // bound_children
			// This handles the case where the pose was set to the rest position; the pose property would == Transform() and would not be saved to the scene by default.
			// However, the bound_children property was always saved regardless of value, and it was always saved after both pose and rest.
			// We don't do anything else with bound_children, as it's not present on Skeleton3D.
			Vector3 pos = get_bone_pose_position(which);
			Quaternion rot = get_bone_pose_rotation(which);
			Vector3 scale = get_bone_pose_scale(which);
			Transform3D rest = get_bone_rest(which);
			if (rest != Transform3D() && pos == Vector3() && rot == Quaternion() && scale == Vector3(1, 1, 1)) {
				set_bone_pose_position(which, rest.origin);
				set_bone_pose_rotation(which, rest.basis.get_rotation_quaternion());
				set_bone_pose_scale(which, rest.basis.get_scale());
			}
		}
#endif
	} else {
		return false;
	}

	return true;
}

bool Skeleton3D::_get(const StringName &p_path, Variant &r_ret) const {
#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
	if (p_path == SNAME("animate_physical_bones")) {
		r_ret = get_animate_physical_bones();
		return true;
	}
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED
	String path = p_path;

	if (!path.begins_with("bones/")) {
		return false;
	}

	uint32_t which = path.get_slicec('/', 1).to_int();
	String what = path.get_slicec('/', 2);

	ERR_FAIL_UNSIGNED_INDEX_V(which, bones.size(), false);

	if (what == "name") {
		r_ret = get_bone_name(which);
	} else if (what == "parent") {
		r_ret = get_bone_parent(which);
	} else if (what == "rest") {
		r_ret = get_bone_rest(which);
	} else if (what == "enabled") {
		r_ret = is_bone_enabled(which);
	} else if (what == "position") {
		r_ret = get_bone_pose_position(which);
	} else if (what == "rotation") {
		r_ret = get_bone_pose_rotation(which);
	} else if (what == "scale") {
		r_ret = get_bone_pose_scale(which);
	} else if (what == "is_human_bone") {
		r_ret = is_human_bone(which);
	} else if (what == "bone_meta") {
		r_ret = get_bone_meta(which, path.get_slicec('/', 3));
	} else {
		return false;
	}

	return true;
}

void Skeleton3D::_get_property_list(List<PropertyInfo> *p_list) const {
	for (uint32_t i = 0; i < bones.size(); i++) {
		const String prep = vformat("%s/%d/", "bones", i);

		int enabled_usage = PROPERTY_USAGE_NO_EDITOR;
		int xform_usage = PROPERTY_USAGE_NO_EDITOR;
		if (is_show_rest_only()) {
			enabled_usage |= PROPERTY_USAGE_READ_ONLY;
			xform_usage |= PROPERTY_USAGE_READ_ONLY;
		} else if (!is_bone_enabled(i)) {
			xform_usage |= PROPERTY_USAGE_READ_ONLY;
		}

		p_list->push_back(PropertyInfo(Variant::STRING, prep + "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
		p_list->push_back(PropertyInfo(Variant::INT, prep + "parent", PROPERTY_HINT_RANGE, "-1," + itos(bones.size() - 1) + ",1", PROPERTY_USAGE_NO_EDITOR));
		p_list->push_back(PropertyInfo(Variant::TRANSFORM3D, prep + "rest", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY));
		p_list->push_back(PropertyInfo(Variant::BOOL, prep + "enabled", PROPERTY_HINT_NONE, "", enabled_usage));
		p_list->push_back(PropertyInfo(Variant::VECTOR3, prep + "position", PROPERTY_HINT_NONE, "", xform_usage));
		p_list->push_back(PropertyInfo(Variant::QUATERNION, prep + "rotation", PROPERTY_HINT_NONE, "", xform_usage));
		p_list->push_back(PropertyInfo(Variant::VECTOR3, prep + "scale", PROPERTY_HINT_NONE, "", xform_usage));
		p_list->push_back(PropertyInfo(Variant::VECTOR3, prep + PNAME("is_human_bone"), PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));

		for (const KeyValue<StringName, Variant> &K : bones[i].metadata) {
			PropertyInfo pi = PropertyInfo(bones[i].metadata[K.key].get_type(), prep + "bone_meta/" + K.key, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR);
			p_list->push_back(pi);
		}
	}
}

void Skeleton3D::_update_process_order() const {
	if (!process_order_dirty) {
		return;
	}

	Bone *bonesptr = bones.ptr();
	int len = bones.size();

	parentless_bones.clear();

	for (int i = 0; i < len; i++) {
		bonesptr[i].child_bones.clear();
	}

	for (int i = 0; i < len; i++) {
		if (bonesptr[i].parent >= len) {
			// Validate this just in case.
			ERR_PRINT("Bone " + itos(i) + " has invalid parent: " + itos(bonesptr[i].parent));
			bonesptr[i].parent = -1;
		}

		if (bonesptr[i].parent != -1) {
			int parent_bone_idx = bonesptr[i].parent;

			// Check to see if this node is already added to the parent.
			if (!bonesptr[parent_bone_idx].child_bones.has(i)) {
				// Add the child node.
				bonesptr[parent_bone_idx].child_bones.push_back(i);
			} else {
				ERR_PRINT("Skeleton3D parenthood graph is cyclic");
			}
		} else {
			parentless_bones.push_back(i);
		}
	}

	concatenated_bone_names = StringName();

	_update_bones_nested_set();

	process_order_dirty = false;

	const_cast<Skeleton3D *>(this)->emit_signal("bone_list_changed");
}

void Skeleton3D::_update_bone_names() const {
	String names;
	for (uint32_t i = 0; i < bones.size(); i++) {
		if (i > 0) {
			names += ",";
		}
		names += bones[i].name;
	}
	concatenated_bone_names = StringName(names);
}

StringName Skeleton3D::get_concatenated_bone_names() const {
	if (concatenated_bone_names == StringName()) {
		_update_bone_names();
	}
	return concatenated_bone_names;
}

#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
void Skeleton3D::setup_simulator() {
	if (simulator && simulator->get_parent() == this) {
		remove_child(simulator);
		simulator->queue_free();
	}
	PhysicalBoneSimulator3D *sim = memnew(PhysicalBoneSimulator3D);
	simulator = sim;
	sim->is_compat = true;
	sim->set_active(false); // Don't run unneeded process.
	add_child(simulator, false, INTERNAL_MODE_BACK);
	set_animate_physical_bones(animate_physical_bones);
}
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED

void Skeleton3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_process_changed();
			_make_dirty();
			_make_modifiers_dirty();
			force_update_all_dirty_bones();
#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
			setup_simulator();
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED
			update_flags = UPDATE_FLAG_POSE;
			_notification(NOTIFICATION_UPDATE_SKELETON);
		} break;
#ifdef TOOLS_ENABLED
		case NOTIFICATION_EDITOR_PRE_SAVE: {
			saving = true;
		} break;
		case NOTIFICATION_EDITOR_POST_SAVE: {
			saving = false;
		} break;
#endif // TOOLS_ENABLED
		case NOTIFICATION_UPDATE_SKELETON: {
			// Update bone transforms to apply unprocessed poses.
			force_update_all_dirty_bones();

			updating = true;

			Bone *bonesptr = bones.ptr();
			int len = bones.size();

			thread_local LocalVector<bool> bone_global_pose_dirty_backup;

			// Process modifiers.

			thread_local LocalVector<BonePoseBackup> bones_backup;
			_find_modifiers();
			if (!modifiers.is_empty()) {
				bones_backup.resize(bones.size());
				// Store unmodified bone poses.
				for (uint32_t i = 0; i < bones.size(); i++) {
					bones_backup[i].save(bonesptr[i]);
				}
				// Store dirty flags for global bone poses.
				bone_global_pose_dirty_backup = bone_global_pose_dirty;

				if (update_flags & UPDATE_FLAG_MODIFIER) {
					_process_modifiers();
				}
			}

			// Abort if pose is not changed.
			if (!(update_flags & UPDATE_FLAG_POSE)) {
				updating = false;
				update_flags = UPDATE_FLAG_NONE;
				return;
			}

			emit_signal(SceneStringName(skeleton_updated));

			// Update skins.
			RenderingServer *rs = RenderingServer::get_singleton();
			for (SkinReference *E : skin_bindings) {
				const Skin *skin = E->skin.operator->();
				RID skeleton = E->skeleton;
				uint32_t bind_count = skin->get_bind_count();

				if (E->bind_count != bind_count) {
					RS::get_singleton()->skeleton_allocate_data(skeleton, bind_count);
					E->bind_count = bind_count;
					E->skin_bone_indices.resize(bind_count);
					E->skin_bone_indices_ptrs = E->skin_bone_indices.ptrw();
				}

				if (E->skeleton_version != version) {
					for (uint32_t i = 0; i < bind_count; i++) {
						StringName bind_name = skin->get_bind_name(i);

						if (bind_name != StringName()) {
							// Bind name used, use this.
							bool found = false;
							for (int j = 0; j < len; j++) {
								if (bonesptr[j].name == bind_name) {
									E->skin_bone_indices_ptrs[i] = j;
									found = true;
									break;
								}
							}

							if (!found) {
								ERR_PRINT("Skin bind #" + itos(i) + " contains named bind '" + String(bind_name) + "' but Skeleton3D has no bone by that name.");
								E->skin_bone_indices_ptrs[i] = 0;
							}
						} else if (skin->get_bind_bone(i) >= 0) {
							int bind_index = skin->get_bind_bone(i);
							if (bind_index >= len) {
								ERR_PRINT("Skin bind #" + itos(i) + " contains bone index bind: " + itos(bind_index) + " , which is greater than the skeleton bone count: " + itos(len) + ".");
								E->skin_bone_indices_ptrs[i] = 0;
							} else {
								E->skin_bone_indices_ptrs[i] = bind_index;
							}
						} else {
							ERR_PRINT("Skin bind #" + itos(i) + " does not contain a name nor a bone index.");
							E->skin_bone_indices_ptrs[i] = 0;
						}
					}

					E->skeleton_version = version;
				}

				for (uint32_t i = 0; i < bind_count; i++) {
					uint32_t bone_index = E->skin_bone_indices_ptrs[i];
					ERR_CONTINUE(bone_index >= (uint32_t)len);
					rs->skeleton_bone_set_transform(skeleton, i, bonesptr[bone_index].global_pose * skin->get_bind_pose(i));
				}
			}

			if (!modifiers.is_empty()) {
				// Restore unmodified bone poses.
				for (uint32_t i = 0; i < bones.size(); i++) {
					bones_backup[i].restore(bones[i]);
				}
				// Restore dirty flags for global bone poses.
				bone_global_pose_dirty = bone_global_pose_dirty_backup;
			}

			updating = false;
			update_flags = UPDATE_FLAG_NONE;
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			advance(get_process_delta_time());
		} break;
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			advance(get_physics_process_delta_time());
		} break;
	}
}

void Skeleton3D::advance(double p_delta) {
	_find_modifiers();
	if (!modifiers.is_empty()) {
		update_delta += p_delta; // Accumulate delta for manual advance as it needs to process in deferred update.
		_update_deferred(UPDATE_FLAG_MODIFIER);
	}
}

void Skeleton3D::set_modifier_callback_mode_process(Skeleton3D::ModifierCallbackModeProcess p_mode) {
	if (modifier_callback_mode_process == p_mode) {
		return;
	}
	modifier_callback_mode_process = p_mode;
	_process_changed();
}

Skeleton3D::ModifierCallbackModeProcess Skeleton3D::get_modifier_callback_mode_process() const {
	return modifier_callback_mode_process;
}

void Skeleton3D::set_human_config(const Ref<HumanBoneConfig> &p_human_config) {
	human_config = p_human_config;
}
Ref<HumanBoneConfig> Skeleton3D::get_human_config() const {
	return human_config;
}
void Skeleton3D::_process_changed() {
	if (modifier_callback_mode_process == MODIFIER_CALLBACK_MODE_PROCESS_IDLE) {
		set_process_internal(true);
		set_physics_process_internal(false);
	} else if (modifier_callback_mode_process == MODIFIER_CALLBACK_MODE_PROCESS_PHYSICS) {
		set_process_internal(false);
		set_physics_process_internal(true);
	} else {
		set_process_internal(false);
		set_physics_process_internal(false);
	}
}

void Skeleton3D::_make_modifiers_dirty() {
	modifiers_dirty = true;
	_update_deferred(UPDATE_FLAG_MODIFIER);
}

void Skeleton3D::_update_bones_nested_set() const {
	nested_set_offset_to_bone_index.resize(bones.size());
	bone_global_pose_dirty.resize(bones.size());
	_make_bone_global_poses_dirty();

	int offset = 0;
	for (int bone : parentless_bones) {
		offset += _update_bone_nested_set(bone, offset);
	}
}

int Skeleton3D::_update_bone_nested_set(int p_bone, int p_offset) const {
	Bone &bone = bones[p_bone];
	int offset = p_offset + 1;
	int span = 1;

	for (int child_bone : bone.child_bones) {
		int subspan = _update_bone_nested_set(child_bone, offset);
		offset += subspan;
		span += subspan;
	}

	nested_set_offset_to_bone_index[p_offset] = p_bone;
	bone.nested_set_offset = p_offset;
	bone.nested_set_span = span;

	return span;
}

void Skeleton3D::_make_bone_global_poses_dirty() const {
	for (uint32_t i = 0; i < bone_global_pose_dirty.size(); i++) {
		bone_global_pose_dirty[i] = true;
	}
}

void Skeleton3D::_make_bone_global_pose_subtree_dirty(int p_bone) const {
	if (process_order_dirty) {
		return;
	}

	const Bone &bone = bones[p_bone];
	int span_offset = bone.nested_set_offset;
	// No need to make subtree dirty when bone is already dirty.
	if (bone_global_pose_dirty[span_offset]) {
		return;
	}

	// Make global poses of subtree dirty.
	int span_end = span_offset + bone.nested_set_span;
	for (int i = span_offset; i < span_end; i++) {
		bone_global_pose_dirty[i] = true;
	}
}

void Skeleton3D::_update_bone_global_pose(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	_update_process_order();

	// Global pose is already calculated.
	int nested_set_offset = bones[p_bone].nested_set_offset;
	if (!bone_global_pose_dirty[nested_set_offset]) {
		return;
	}

	thread_local LocalVector<int> bone_list;
	bone_list.clear();
	Transform3D global_pose;

	// Create list of parent bones for which the global pose needs to be recalculated.
	for (int bone = p_bone; bone >= 0; bone = bones[bone].parent) {
		int offset = bones[bone].nested_set_offset;
		// Stop searching when global pose is not dirty.
		if (!bone_global_pose_dirty[offset]) {
			global_pose = bones[bone].global_pose;
			break;
		}

		bone_list.push_back(bone);
	}

	// Calculate global poses for all parent bones and the current bone.
	for (int i = bone_list.size() - 1; i >= 0; i--) {
		int bone_idx = bone_list[i];
		Bone &bone = bones[bone_idx];
		bool bone_enabled = bone.enabled && !show_rest_only;
		Transform3D bone_pose = bone_enabled ? get_bone_pose(bone_idx) : get_bone_rest(bone_idx);

		global_pose *= bone_pose;
#ifndef DISABLE_DEPRECATED
		if (bone.global_pose_override_amount >= CMP_EPSILON) {
			global_pose = global_pose.interpolate_with(bone.global_pose_override, bone.global_pose_override_amount);
		}
#endif // _DISABLE_DEPRECATED

		bone.global_pose = global_pose;
		bone_global_pose_dirty[bone.nested_set_offset] = false;
	}
}

Transform3D Skeleton3D::get_bone_global_pose(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	_update_bone_global_pose(p_bone);
	return bones[p_bone].global_pose;
}

void Skeleton3D::set_bone_global_pose(int p_bone, const Transform3D &p_pose) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	Transform3D pt;
	if (bones[p_bone].parent >= 0) {
		pt = get_bone_global_pose(bones[p_bone].parent);
	}
	Transform3D t = pt.affine_inverse() * p_pose;
	set_bone_pose(p_bone, t);
}

void Skeleton3D::set_motion_scale(float p_motion_scale) {
	if (p_motion_scale <= 0) {
		motion_scale = 1;
		ERR_FAIL_MSG("Motion scale must be larger than 0.");
	}
	motion_scale = p_motion_scale;
}

float Skeleton3D::get_motion_scale() const {
	ERR_FAIL_COND_V(motion_scale <= 0, 1);
	return motion_scale;
}

// Skeleton creation api

uint64_t Skeleton3D::get_version() const {
	return version;
}

int Skeleton3D::add_bone(const String &p_name) {
	ERR_FAIL_COND_V_MSG(p_name.is_empty() || p_name.contains_char(':') || p_name.contains_char('/'), -1, vformat("Bone name cannot be empty or contain ':' or '/'.", p_name));
	ERR_FAIL_COND_V_MSG(name_to_bone_index.has(p_name), -1, vformat("Skeleton3D \"%s\" already has a bone with name \"%s\".", to_string(), p_name));

	Bone b;
	b.name = p_name;
	bones.push_back(b);
	int new_idx = bones.size() - 1;
	name_to_bone_index.insert(p_name, new_idx);
	process_order_dirty = true;
	version++;
	rest_dirty = true;
	_make_dirty();
	update_gizmos();
	return new_idx;
}

int Skeleton3D::find_bone(const String &p_name) const {
	const int *bone_index_ptr = name_to_bone_index.getptr(p_name);
	return bone_index_ptr != nullptr ? *bone_index_ptr : -1;
}

String Skeleton3D::get_bone_name(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, "");
	return bones[p_bone].name;
}

void Skeleton3D::set_bone_name(int p_bone, const String &p_name) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	const int *bone_index_ptr = name_to_bone_index.getptr(p_name);
	if (bone_index_ptr != nullptr) {
		ERR_FAIL_COND_MSG(*bone_index_ptr != p_bone, "Skeleton3D: '" + get_name() + "', bone name:  '" + p_name + "' already exists.");
		return; // No need to rename, the bone already has the given name.
	}

	name_to_bone_index.erase(bones[p_bone].name);
	bones[p_bone].name = p_name;
	name_to_bone_index.insert(p_name, p_bone);

	version++;
}

Variant Skeleton3D::get_bone_meta(int p_bone, const StringName &p_key) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Variant());

	if (!bones[p_bone].metadata.has(p_key)) {
		return Variant();
	}
	return bones[p_bone].metadata[p_key];
}

TypedArray<StringName> Skeleton3D::_get_bone_meta_list_bind(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, TypedArray<StringName>());

	TypedArray<StringName> _metaret;
	for (const KeyValue<StringName, Variant> &K : bones[p_bone].metadata) {
		_metaret.push_back(K.key);
	}
	return _metaret;
}

void Skeleton3D::get_bone_meta_list(int p_bone, List<StringName> *p_list) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	for (const KeyValue<StringName, Variant> &K : bones[p_bone].metadata) {
		p_list->push_back(K.key);
	}
}

bool Skeleton3D::has_bone_meta(int p_bone, const StringName &p_key) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, false);

	return bones[p_bone].metadata.has(p_key);
}

void Skeleton3D::set_bone_meta(int p_bone, const StringName &p_key, const Variant &p_value) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	if (p_value.get_type() == Variant::NIL) {
		if (bones[p_bone].metadata.has(p_key)) {
			bones[p_bone].metadata.erase(p_key);
		}
		return;
	}

	bones[p_bone].metadata.insert(p_key, p_value, false);
}

bool Skeleton3D::is_bone_parent_of(int p_bone, int p_parent_bone_id) const {
	int parent_of_bone = get_bone_parent(p_bone);

	if (-1 == parent_of_bone) {
		return false;
	}

	if (parent_of_bone == p_parent_bone_id) {
		return true;
	}

	return is_bone_parent_of(parent_of_bone, p_parent_bone_id);
}

int Skeleton3D::get_bone_count() const {
	return bones.size();
}

void Skeleton3D::set_bone_parent(int p_bone, int p_parent) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	ERR_FAIL_COND(p_parent != -1 && (p_parent < 0));
	ERR_FAIL_COND(p_bone == p_parent);

	bones[p_bone].parent = p_parent;
	process_order_dirty = true;
	rest_dirty = true;
	_make_dirty();
}

void Skeleton3D::unparent_bone_and_rest(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	_update_process_order();

	int parent = bones[p_bone].parent;
	while (parent >= 0) {
		bones[p_bone].rest = bones[parent].rest * bones[p_bone].rest;
		parent = bones[parent].parent;
	}

	bones[p_bone].parent = -1;
	process_order_dirty = true;

	rest_dirty = true;
	_make_dirty();
}

int Skeleton3D::get_bone_parent(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, -1);
	if (process_order_dirty) {
		_update_process_order();
	}
	return bones[p_bone].parent;
}

Vector<int> Skeleton3D::get_bone_children(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector<int>());
	if (process_order_dirty) {
		_update_process_order();
	}
	return bones[p_bone].child_bones;
}

Vector<int> Skeleton3D::get_parentless_bones() const {
	if (process_order_dirty) {
		_update_process_order();
	}
	return parentless_bones;
}
Vector<int> Skeleton3D::get_root_bones() const {
	Vector<int> rs;
	for (uint32_t i = 0; i < bones.size(); i++) {
		if (bones[i].parent < 0) {
			rs.push_back(i);
		}
	}
	return rs;
}

void Skeleton3D::set_bone_rest(int p_bone, const Transform3D &p_rest) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].rest = p_rest;
	rest_dirty = true;
	_make_dirty();
	_make_bone_global_pose_subtree_dirty(p_bone);
}
Transform3D Skeleton3D::get_bone_rest(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());

	return bones[p_bone].rest;
}
Transform3D Skeleton3D::get_bone_global_rest(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	if (rest_dirty) {
		_force_update_all_bone_transforms();
	}
	return bones[p_bone].global_rest;
}

void Skeleton3D::set_bone_enabled(int p_bone, bool p_enabled) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].enabled = p_enabled;
	emit_signal(SceneStringName(bone_enabled_changed), p_bone);
	_make_dirty();
	_make_bone_global_pose_subtree_dirty(p_bone);
}

bool Skeleton3D::is_bone_enabled(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, false);
	return bones[p_bone].enabled;
}

void Skeleton3D::set_show_rest_only(bool p_enabled) {
	show_rest_only = p_enabled;
	emit_signal(SceneStringName(show_rest_only_changed));
	_make_dirty();
	_make_bone_global_poses_dirty();
}

bool Skeleton3D::is_show_rest_only() const {
	return show_rest_only;
}

void Skeleton3D::clear_bones() {
	bones.clear();
	name_to_bone_index.clear();

	// All these structures contain references to now invalid bone indices.
	skin_bindings.clear();
	bone_global_pose_dirty.clear();
	parentless_bones.clear();
	nested_set_offset_to_bone_index.clear();

	process_order_dirty = true;
	version++;
	_make_dirty();
}

// Posing api

void Skeleton3D::set_bone_pose(int p_bone, const Transform3D &p_pose, bool p_notify ) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].pose_position = p_pose.origin;
	bones[p_bone].pose_rotation = p_pose.basis.get_rotation_quaternion();
	bones[p_bone].pose_scale = p_pose.basis.get_scale();
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		if (p_notify) {
			_make_dirty();
		}
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

void Skeleton3D::set_bone_pose_position(int p_bone, const Vector3 &p_position, bool p_notify) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].pose_position = p_position;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		if (p_notify) {
			_make_dirty();
		}
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}
void Skeleton3D::set_bone_pose_rotation(int p_bone, const Quaternion &p_rotation, bool p_notify) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].pose_rotation = p_rotation;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		if (p_notify) {
			_make_dirty();
		}
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}
void Skeleton3D::set_bone_pose_scale(int p_bone, const Vector3 &p_scale, bool p_notify) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].pose_scale = p_scale;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		if (p_notify) {
			_make_dirty();
		}
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}
void Skeleton3D::set_human_bone(int p_bone, bool p_is_human_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].is_human_bone = true;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
	}
}

Vector3 Skeleton3D::get_bone_pose_position(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector3());
	return bones[p_bone].pose_position;
}

Quaternion Skeleton3D::get_bone_pose_rotation(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Quaternion());
	return bones[p_bone].pose_rotation;
}

Vector3 Skeleton3D::get_bone_pose_scale(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector3());
	return bones[p_bone].pose_scale;
}
bool Skeleton3D::is_human_bone(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, false);
	return bones[p_bone].is_human_bone;
}

void Skeleton3D::reset_bone_pose(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	set_bone_pose_position(p_bone, bones[p_bone].rest.origin);
	set_bone_pose_rotation(p_bone, bones[p_bone].rest.basis.get_rotation_quaternion());
	set_bone_pose_scale(p_bone, bones[p_bone].rest.basis.get_scale());
}

void Skeleton3D::reset_bone_poses() {
	for (uint32_t i = 0; i < bones.size(); i++) {
		reset_bone_pose(i);
	}
}

Transform3D Skeleton3D::get_bone_pose(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	bones[p_bone].update_pose_cache();
	return bones[p_bone].pose_cache;
}

void Skeleton3D::_make_dirty() {
	if (dirty) {
		return;
	}
	dirty = true;
	_update_deferred();
}

void Skeleton3D::_update_deferred(UpdateFlag p_update_flag) {
	if (is_inside_tree()) {
#ifdef TOOLS_ENABLED
		if (saving) {
			update_flags |= p_update_flag;
			_notification(NOTIFICATION_UPDATE_SKELETON);
			return;
		}
#endif //TOOLS_ENABLED
		if (update_flags == UPDATE_FLAG_NONE && !updating) {
			notify_deferred_thread_group(NOTIFICATION_UPDATE_SKELETON); // It must never be called more than once in a single frame.
		}
		update_flags |= p_update_flag;
	}
}

void Skeleton3D::localize_rests() {
	Vector<int> bones_to_process = get_parentless_bones();
	while (bones_to_process.size() > 0) {
		int current_bone_idx = bones_to_process[0];
		bones_to_process.erase(current_bone_idx);

		if (bones[current_bone_idx].parent >= 0) {
			set_bone_rest(current_bone_idx, bones[bones[current_bone_idx].parent].rest.affine_inverse() * bones[current_bone_idx].rest);
		}

		// Add the bone's children to the list of bones to be processed.
		int child_bone_size = bones[current_bone_idx].child_bones.size();
		for (int i = 0; i < child_bone_size; i++) {
			bones_to_process.push_back(bones[current_bone_idx].child_bones[i]);
		}
	}
}

void Skeleton3D::_skin_changed() {
	_make_dirty();
}

Ref<Skin> Skeleton3D::create_skin_from_rest_transforms() {
	Ref<Skin> skin;

	skin.instantiate();
	skin->set_bind_count(bones.size());

	// Pose changed, rebuild cache of inverses.
	const Bone *bonesptr = bones.ptr();
	uint32_t len = bones.size();

	// Calculate global rests and invert them.
	LocalVector<int> bones_to_process;
	bones_to_process = get_parentless_bones();
	while (bones_to_process.size() > 0) {
		int current_bone_idx = bones_to_process[0];
		const Bone &b = bonesptr[current_bone_idx];
		bones_to_process.erase(current_bone_idx);
		LocalVector<int> child_bones_vector;
		child_bones_vector = get_bone_children(current_bone_idx);
		int child_bones_size = child_bones_vector.size();
		if (b.parent < 0) {
			skin->set_bind_pose(current_bone_idx, b.rest);
		}
		for (int i = 0; i < child_bones_size; i++) {
			int child_bone_idx = child_bones_vector[i];
			const Bone &cb = bonesptr[child_bone_idx];
			skin->set_bind_pose(child_bone_idx, skin->get_bind_pose(current_bone_idx) * cb.rest);
			// Add the bone's children to the list of bones to be processed.
			bones_to_process.push_back(child_bones_vector[i]);
		}
	}

	for (uint32_t i = 0; i < len; i++) {
		// The inverse is what is actually required.
		skin->set_bind_bone(i, i);
		skin->set_bind_pose(i, skin->get_bind_pose(i).affine_inverse());
	}

	return skin;
}

Ref<SkinReference> Skeleton3D::register_skin(const Ref<Skin> &p_skin) {
	ERR_FAIL_COND_V(p_skin.is_null(), Ref<SkinReference>());

	for (const SkinReference *E : skin_bindings) {
		if (E->skin == p_skin) {
			return Ref<SkinReference>(E);
		}
	}

	Ref<SkinReference> skin_ref;
	skin_ref.instantiate();

	skin_ref->skeleton_node = this;
	skin_ref->bind_count = 0;
	skin_ref->skeleton = RenderingServer::get_singleton()->skeleton_create();
	skin_ref->skeleton_node = this;
	skin_ref->skin = p_skin;

	skin_bindings.insert(skin_ref.operator->());

	skin_ref->skin->connect_changed(callable_mp(skin_ref.operator->(), &SkinReference::_skin_changed));

	_make_dirty(); // Skin needs to be updated, so update skeleton.

	return skin_ref;
}

void Skeleton3D::force_update_deferred() {
	_make_dirty();
}

void Skeleton3D::force_update_all_dirty_bones(bool p_notify) {
	_force_update_all_dirty_bones();
}

void Skeleton3D::_force_update_all_dirty_bones(bool p_notify) const {
	if (!dirty) {
		return;
	}
	_force_update_all_bone_transforms();
}

void Skeleton3D::force_update_all_bone_transforms(bool p_notify) {
	_force_update_all_bone_transforms();
}

void Skeleton3D::_force_update_all_bone_transforms(bool p_notify) const {
	_update_process_order();
	for (int i = 0; i < parentless_bones.size(); i++) {
		_force_update_bone_children_transforms(parentless_bones[i]);
	}
	if (rest_dirty) {
		rest_dirty = false;
		const_cast<Skeleton3D *>(this)->emit_signal(SNAME("rest_updated"));
	} else {
		rest_dirty = false;
	}
	dirty = false;
	if (updating) {
		return;
	}
	if (p_notify && _notification_bone_pose) {
		const_cast<Skeleton3D *>(this)->emit_signal(SceneStringName(pose_updated));
	}
}

void Skeleton3D::force_update_bone_children_transforms(int p_bone_idx) {
	_force_update_bone_children_transforms(p_bone_idx);
}

void Skeleton3D::_force_update_bone_children_transforms(int p_bone_idx) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone_idx, bone_size);

	_update_process_order();

	Bone *bonesptr = bones.ptr();

	// Loop through nested set.
	for (int offset = 0; offset < bone_size; offset++) {
		if (rest_dirty) {
			int current_bone_idx = nested_set_offset_to_bone_index[offset];
			Bone &b = bonesptr[current_bone_idx];
			b.global_rest = b.parent >= 0 ? bonesptr[b.parent].global_rest * b.rest : b.rest; // Rest needs update apert from pose.
		}

		if (!bone_global_pose_dirty[offset]) {
			continue;
		}

		int current_bone_idx = nested_set_offset_to_bone_index[offset];
		Bone &b = bonesptr[current_bone_idx];
		bool bone_enabled = b.enabled && !show_rest_only;

		if (bone_enabled) {
			b.update_pose_cache();
			Transform3D pose = b.pose_cache;

			if (b.parent >= 0) {
				b.global_pose = bonesptr[b.parent].global_pose * pose;
			} else {
				b.global_pose = pose;
			}
		} else {
			if (b.parent >= 0) {
				b.global_pose = bonesptr[b.parent].global_pose * b.rest;
			} else {
				b.global_pose = b.rest;
			}
		}

#ifndef DISABLE_DEPRECATED
		if (bone_enabled) {
			Transform3D pose = b.pose_cache;
			if (b.parent >= 0) {
				b.pose_global_no_override = bonesptr[b.parent].pose_global_no_override * pose;
			} else {
				b.pose_global_no_override = pose;
			}
		} else {
			if (b.parent >= 0) {
				b.pose_global_no_override = bonesptr[b.parent].pose_global_no_override * b.rest;
			} else {
				b.pose_global_no_override = b.rest;
			}
		}
		if (b.global_pose_override_amount >= CMP_EPSILON) {
			b.global_pose = b.global_pose.interpolate_with(b.global_pose_override, b.global_pose_override_amount);
		}
		if (b.global_pose_override_reset) {
			b.global_pose_override_amount = 0.0;
		}
#endif // _DISABLE_DEPRECATED

		bone_global_pose_dirty[offset] = false;
	}
}

void Skeleton3D::_find_modifiers() {
	if (!modifiers_dirty) {
		return;
	}
	modifiers.clear();
	for (int i = 0; i < get_child_count(); i++) {
		SkeletonModifier3D *c = Object::cast_to<SkeletonModifier3D>(get_child(i));
		if (c) {
			modifiers.push_back(c->get_instance_id());
		}
	}
	modifiers_dirty = false;
}

void Skeleton3D::_process_modifiers() {
	for (const ObjectID &oid : modifiers) {
		Object *t_obj = ObjectDB::get_instance(oid);
		if (!t_obj) {
			continue;
		}
		SkeletonModifier3D *mod = cast_to<SkeletonModifier3D>(t_obj);
		if (!mod) {
			continue;
		}
#ifdef TOOLS_ENABLED
		if (saving && !mod->is_processed_on_saving()) {
			continue;
		}
#endif //TOOLS_ENABLED
		real_t influence = mod->get_influence();
		if (influence < 1.0) {
			LocalVector<Transform3D> old_poses;
			for (int i = 0; i < get_bone_count(); i++) {
				old_poses.push_back(get_bone_pose(i));
			}
			mod->process_modification(update_delta);
			LocalVector<Transform3D> new_poses;
			for (int i = 0; i < get_bone_count(); i++) {
				new_poses.push_back(get_bone_pose(i));
			}
			for (int i = 0; i < get_bone_count(); i++) {
				if (old_poses[i] == new_poses[i]) {
					continue; // Avoid unneeded calculation.
				}
				set_bone_pose(i, old_poses[i].interpolate_with(new_poses[i], influence));
			}
		} else {
			mod->process_modification(update_delta);
		}
		force_update_all_dirty_bones();
	}
	update_delta = 0; // Reset accumulated delta.
}

void Skeleton3D::add_child_notify(Node *p_child) {
	if (Object::cast_to<SkeletonModifier3D>(p_child)) {
		_make_modifiers_dirty();
	}
}

void Skeleton3D::move_child_notify(Node *p_child) {
	if (Object::cast_to<SkeletonModifier3D>(p_child)) {
		_make_modifiers_dirty();
	}
}

void Skeleton3D::remove_child_notify(Node *p_child) {
	if (Object::cast_to<SkeletonModifier3D>(p_child)) {
		_make_modifiers_dirty();
	}
}

void Skeleton3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_bone", "name"), &Skeleton3D::add_bone);
	ClassDB::bind_method(D_METHOD("find_bone", "name"), &Skeleton3D::find_bone);
	ClassDB::bind_method(D_METHOD("get_bone_name", "bone_idx"), &Skeleton3D::get_bone_name);
	ClassDB::bind_method(D_METHOD("set_bone_name", "bone_idx", "name"), &Skeleton3D::set_bone_name);

	ClassDB::bind_method(D_METHOD("get_bone_meta", "bone_idx", "key"), &Skeleton3D::get_bone_meta);
	ClassDB::bind_method(D_METHOD("get_bone_meta_list", "bone_idx"), &Skeleton3D::_get_bone_meta_list_bind);
	ClassDB::bind_method(D_METHOD("has_bone_meta", "bone_idx", "key"), &Skeleton3D::has_bone_meta);
	ClassDB::bind_method(D_METHOD("set_bone_meta", "bone_idx", "key", "value"), &Skeleton3D::set_bone_meta);

	ClassDB::bind_method(D_METHOD("get_concatenated_bone_names"), &Skeleton3D::get_concatenated_bone_names);

	ClassDB::bind_method(D_METHOD("get_bone_parent", "bone_idx"), &Skeleton3D::get_bone_parent);
	ClassDB::bind_method(D_METHOD("set_bone_parent", "bone_idx", "parent_idx"), &Skeleton3D::set_bone_parent);

	ClassDB::bind_method(D_METHOD("get_bone_count"), &Skeleton3D::get_bone_count);
	ClassDB::bind_method(D_METHOD("get_version"), &Skeleton3D::get_version);

	ClassDB::bind_method(D_METHOD("unparent_bone_and_rest", "bone_idx"), &Skeleton3D::unparent_bone_and_rest);

	ClassDB::bind_method(D_METHOD("get_bone_children", "bone_idx"), &Skeleton3D::get_bone_children);

	ClassDB::bind_method(D_METHOD("get_parentless_bones"), &Skeleton3D::get_parentless_bones);

	ClassDB::bind_method(D_METHOD("get_root_bones"), &Skeleton3D::get_root_bones);

	ClassDB::bind_method(D_METHOD("get_human_bone_mapping"), &Skeleton3D::get_human_bone_mapping);

	ClassDB::bind_method(D_METHOD("set_human_bone_mapping", "mapping"), &Skeleton3D::set_human_bone_mapping);

	ClassDB::bind_method(D_METHOD("get_bone_rest", "bone_idx"), &Skeleton3D::get_bone_rest);
	ClassDB::bind_method(D_METHOD("set_bone_rest", "bone_idx", "rest"), &Skeleton3D::set_bone_rest);
	ClassDB::bind_method(D_METHOD("get_bone_global_rest", "bone_idx"), &Skeleton3D::get_bone_global_rest);

	ClassDB::bind_method(D_METHOD("create_skin_from_rest_transforms"), &Skeleton3D::create_skin_from_rest_transforms);
	ClassDB::bind_method(D_METHOD("register_skin", "skin"), &Skeleton3D::register_skin);

	ClassDB::bind_method(D_METHOD("localize_rests"), &Skeleton3D::localize_rests);

	ClassDB::bind_method(D_METHOD("clear_bones"), &Skeleton3D::clear_bones);

	ClassDB::bind_method(D_METHOD("get_bone_pose", "bone_idx"), &Skeleton3D::get_bone_pose);
	ClassDB::bind_method(D_METHOD("set_bone_pose", "bone_idx", "pose", "notify"), &Skeleton3D::set_bone_pose, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_bone_pose_position", "bone_idx", "position", "notify"), &Skeleton3D::set_bone_pose_position, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_bone_pose_rotation", "bone_idx", "rotation", "notify"), &Skeleton3D::set_bone_pose_rotation, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_bone_pose_scale", "bone_idx", "scale", "notify"), &Skeleton3D::set_bone_pose_scale, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("get_bone_pose_position", "bone_idx"), &Skeleton3D::get_bone_pose_position);
	ClassDB::bind_method(D_METHOD("get_bone_pose_rotation", "bone_idx"), &Skeleton3D::get_bone_pose_rotation);
	ClassDB::bind_method(D_METHOD("get_bone_pose_scale", "bone_idx"), &Skeleton3D::get_bone_pose_scale);

	ClassDB::bind_method(D_METHOD("reset_bone_pose", "bone_idx"), &Skeleton3D::reset_bone_pose);
	ClassDB::bind_method(D_METHOD("reset_bone_poses"), &Skeleton3D::reset_bone_poses);

	ClassDB::bind_method(D_METHOD("is_bone_enabled", "bone_idx"), &Skeleton3D::is_bone_enabled);
	ClassDB::bind_method(D_METHOD("set_bone_enabled", "bone_idx", "enabled"), &Skeleton3D::set_bone_enabled, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("get_bone_global_pose", "bone_idx"), &Skeleton3D::get_bone_global_pose);
	ClassDB::bind_method(D_METHOD("set_bone_global_pose", "bone_idx", "pose"), &Skeleton3D::set_bone_global_pose);

	ClassDB::bind_method(D_METHOD("force_update_all_bone_transforms", "notify"), &Skeleton3D::force_update_all_bone_transforms, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("force_update_bone_child_transform", "bone_idx"), &Skeleton3D::force_update_bone_children_transforms);

	ClassDB::bind_method(D_METHOD("set_motion_scale", "motion_scale"), &Skeleton3D::set_motion_scale);
	ClassDB::bind_method(D_METHOD("get_motion_scale"), &Skeleton3D::get_motion_scale);

	ClassDB::bind_method(D_METHOD("set_show_rest_only", "enabled"), &Skeleton3D::set_show_rest_only);
	ClassDB::bind_method(D_METHOD("is_show_rest_only"), &Skeleton3D::is_show_rest_only);

	ClassDB::bind_method(D_METHOD("set_modifier_callback_mode_process", "mode"), &Skeleton3D::set_modifier_callback_mode_process);
	ClassDB::bind_method(D_METHOD("get_modifier_callback_mode_process"), &Skeleton3D::get_modifier_callback_mode_process);

	ClassDB::bind_method(D_METHOD("set_human_config", "config"), &Skeleton3D::set_human_config);
	ClassDB::bind_method(D_METHOD("get_human_config"), &Skeleton3D::get_human_config);
	ClassDB::bind_method(D_METHOD("advance", "delta"), &Skeleton3D::advance);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "motion_scale", PROPERTY_HINT_RANGE, "0.001,10,0.001,or_greater"), "set_motion_scale", "get_motion_scale");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_rest_only"), "set_show_rest_only", "is_show_rest_only");

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "human_config", PROPERTY_HINT_RESOURCE_TYPE, "HumanBoneConfig"), "set_human_config", "get_human_config");

	ADD_GROUP("Modifier", "modifier_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "modifier_callback_mode_process", PROPERTY_HINT_ENUM, "Physics,Idle,Manual"), "set_modifier_callback_mode_process", "get_modifier_callback_mode_process");

	ADD_SIGNAL(MethodInfo("rest_updated"));
	ADD_SIGNAL(MethodInfo("pose_updated"));
	ADD_SIGNAL(MethodInfo("skeleton_updated"));
	ADD_SIGNAL(MethodInfo("bone_enabled_changed", PropertyInfo(Variant::INT, "bone_idx")));
	ADD_SIGNAL(MethodInfo("bone_list_changed"));
	ADD_SIGNAL(MethodInfo("show_rest_only_changed"));

	BIND_CONSTANT(NOTIFICATION_UPDATE_SKELETON);
	BIND_ENUM_CONSTANT(MODIFIER_CALLBACK_MODE_PROCESS_PHYSICS);
	BIND_ENUM_CONSTANT(MODIFIER_CALLBACK_MODE_PROCESS_IDLE);
	BIND_ENUM_CONSTANT(MODIFIER_CALLBACK_MODE_PROCESS_MANUAL);

#ifndef DISABLE_DEPRECATED
	ClassDB::bind_method(D_METHOD("clear_bones_global_pose_override"), &Skeleton3D::clear_bones_global_pose_override);
	ClassDB::bind_method(D_METHOD("set_bone_global_pose_override", "bone_idx", "pose", "amount", "persistent"), &Skeleton3D::set_bone_global_pose_override, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_bone_global_pose_override", "bone_idx"), &Skeleton3D::get_bone_global_pose_override);
	ClassDB::bind_method(D_METHOD("get_bone_global_pose_no_override", "bone_idx"), &Skeleton3D::get_bone_global_pose_no_override);

#ifndef PHYSICS_3D_DISABLED
	ClassDB::bind_method(D_METHOD("set_animate_physical_bones", "enabled"), &Skeleton3D::set_animate_physical_bones);
	ClassDB::bind_method(D_METHOD("get_animate_physical_bones"), &Skeleton3D::get_animate_physical_bones);
	ClassDB::bind_method(D_METHOD("physical_bones_stop_simulation"), &Skeleton3D::physical_bones_stop_simulation);
	ClassDB::bind_method(D_METHOD("physical_bones_start_simulation", "bones"), &Skeleton3D::physical_bones_start_simulation_on, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("physical_bones_add_collision_exception", "exception"), &Skeleton3D::physical_bones_add_collision_exception);
	ClassDB::bind_method(D_METHOD("physical_bones_remove_collision_exception", "exception"), &Skeleton3D::physical_bones_remove_collision_exception);

	ADD_GROUP("Deprecated", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "animate_physical_bones"), "set_animate_physical_bones", "get_animate_physical_bones");
#endif // PHYSICS_3D_DISABLED
#endif // _DISABLE_DEPRECATED
}

#ifndef DISABLE_DEPRECATED
void Skeleton3D::clear_bones_global_pose_override() {
	for (uint32_t i = 0; i < bones.size(); i += 1) {
		bones[i].global_pose_override_amount = 0;
		bones[i].global_pose_override_reset = true;
	}
	_make_dirty();
	_make_bone_global_poses_dirty();
}

void Skeleton3D::set_bone_global_pose_override(int p_bone, const Transform3D &p_pose, real_t p_amount, bool p_persistent) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].global_pose_override_amount = p_amount;
	bones[p_bone].global_pose_override = p_pose;
	bones[p_bone].global_pose_override_reset = !p_persistent;
	_make_dirty();
	_make_bone_global_pose_subtree_dirty(p_bone);
}

Transform3D Skeleton3D::get_bone_global_pose_override(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	return bones[p_bone].global_pose_override;
}

Transform3D Skeleton3D::get_bone_global_pose_no_override(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	_force_update_all_dirty_bones();
	return bones[p_bone].pose_global_no_override;
}

#ifndef PHYSICS_3D_DISABLED
Node *Skeleton3D::get_simulator() {
	return simulator;
}

void Skeleton3D::set_animate_physical_bones(bool p_enabled) {
	animate_physical_bones = p_enabled;
	PhysicalBoneSimulator3D *sim = cast_to<PhysicalBoneSimulator3D>(simulator);
	if (!sim) {
		return;
	}
	sim->set_active(animate_physical_bones || sim->is_simulating_physics());
}

bool Skeleton3D::get_animate_physical_bones() const {
	return animate_physical_bones;
}

void Skeleton3D::physical_bones_stop_simulation() {
	PhysicalBoneSimulator3D *sim = cast_to<PhysicalBoneSimulator3D>(simulator);
	if (!sim) {
		return;
	}
	sim->physical_bones_stop_simulation();
	sim->set_active(animate_physical_bones || sim->is_simulating_physics());
}

void Skeleton3D::physical_bones_start_simulation_on(const TypedArray<StringName> &p_bones) {
	PhysicalBoneSimulator3D *sim = cast_to<PhysicalBoneSimulator3D>(simulator);
	if (!sim) {
		return;
	}
	sim->set_active(true);
	sim->physical_bones_start_simulation_on(p_bones);
}

void Skeleton3D::physical_bones_add_collision_exception(RID p_exception) {
	PhysicalBoneSimulator3D *sim = cast_to<PhysicalBoneSimulator3D>(simulator);
	if (!sim) {
		return;
	}
	sim->physical_bones_add_collision_exception(p_exception);
}

void Skeleton3D::physical_bones_remove_collision_exception(RID p_exception) {
	PhysicalBoneSimulator3D *sim = cast_to<PhysicalBoneSimulator3D>(simulator);
	if (!sim) {
		return;
	}
	sim->physical_bones_remove_collision_exception(p_exception);
}
#endif // PHYSICS_3D_DISABLED
#endif // _DISABLE_DEPRECATED

Skeleton3D::Skeleton3D() {
}

Skeleton3D::~Skeleton3D() {
	// Some skins may remain bound.
	for (SkinReference *E : skin_bindings) {
		E->skeleton_node = nullptr;
	}
}

#include "modules/regex/regex.h"

static bool is_match_with_bone_name(const String &p_bone_name, const String &p_word) {
	RegEx re = RegEx(p_word);
	return !re.search(p_bone_name.to_lower()).is_null();
}
enum BoneSegregation {
	BONE_SEGREGATION_NONE,
	BONE_SEGREGATION_LEFT,
	BONE_SEGREGATION_RIGHT
};
static BoneSegregation guess_bone_segregation(const String &p_bone_name) {
	String fixed_bn = p_bone_name.to_snake_case();

	LocalVector<String> left_words;
	left_words.push_back("(?<![a-zA-Z])left");
	left_words.push_back("(?<![a-zA-Z0-9])l(?![a-zA-Z0-9])");

	LocalVector<String> right_words;
	right_words.push_back("(?<![a-zA-Z])right");
	right_words.push_back("(?<![a-zA-Z0-9])r(?![a-zA-Z0-9])");

	for (uint32_t i = 0; i < left_words.size(); i++) {
		RegEx re_l = RegEx(left_words[i]);
		if (!re_l.search(fixed_bn).is_null()) {
			return BONE_SEGREGATION_LEFT;
		}
		RegEx re_r = RegEx(right_words[i]);
		if (!re_r.search(fixed_bn).is_null()) {
			return BONE_SEGREGATION_RIGHT;
		}
	}

	return BONE_SEGREGATION_NONE;
}

static int search_bone_by_name(Skeleton3D *p_skeleton, const Vector<String> &p_picklist, bool p_using_segregation, BoneSegregation p_segregation = BONE_SEGREGATION_NONE, int p_parent = -1, int p_child = -1, int p_children_count = -1) {
	// There may be multiple candidates hit by existing the subsidiary bone.
	// The one with the shortest name is probably the original.
	LocalVector<String> hit_list;
	String shortest = "";
	Skeleton3D *skeleton = p_skeleton;

	for (int word_idx = 0; word_idx < p_picklist.size(); word_idx++) {
		if (p_child == -1) {
			Vector<int> bones_to_process = p_parent == -1 ? p_skeleton->get_parentless_bones() : p_skeleton->get_bone_children(p_parent);
			while (bones_to_process.size() > 0) {
				int idx = bones_to_process[0];
				bones_to_process.erase(idx);
				Vector<int> children = p_skeleton->get_bone_children(idx);
				for (int i = 0; i < children.size(); i++) {
					bones_to_process.push_back(children[i]);
				}

				if (p_children_count == 0 && children.size() > 0) {
					continue;
				}
				if (p_children_count > 0 && children.size() < p_children_count) {
					continue;
				}

				String bn = skeleton->get_bone_name(idx);
				if (is_match_with_bone_name(bn, p_picklist[word_idx]) && (!p_using_segregation || guess_bone_segregation(bn) == p_segregation)) {
					hit_list.push_back(bn);
				}
			}

			if (hit_list.size() > 0) {
				shortest = hit_list[0];
				for (const String &hit : hit_list) {
					if (hit.length() < shortest.length()) {
						shortest = hit; // Prioritize parent.
					}
				}
			}
		} else {
			int idx = skeleton->get_bone_parent(p_child);
			while (idx != p_parent && idx >= 0) {
				Vector<int> children = p_skeleton->get_bone_children(idx);
				if (p_children_count == 0 && children.size() > 0) {
					continue;
				}
				if (p_children_count > 0 && children.size() < p_children_count) {
					continue;
				}

				String bn = skeleton->get_bone_name(idx);
				if (is_match_with_bone_name(bn, p_picklist[word_idx]) && (!p_using_segregation || guess_bone_segregation(bn) == p_segregation)) {
					hit_list.push_back(bn);
				}
				idx = skeleton->get_bone_parent(idx);
			}

			if (hit_list.size() > 0) {
				shortest = hit_list[0];
				for (const String &hit : hit_list) {
					if (hit.length() <= shortest.length()) {
						shortest = hit; // Prioritize parent.
					}
				}
			}
		}

		if (shortest != "") {
			break;
		}
	}

	if (shortest == "") {
		return -1;
	}

	return skeleton->find_bone(shortest);
}
/**
 * 骨骼层次结构如下,一共54根骨头,这些层次包含的骨头只能有旋转缩放,不能存在偏移
 "Root\n"
"└─ Hips\n"
"    ├─ LeftUpperLeg\n"
"    │  └─ LeftLowerLeg\n"
"    │     └─ LeftFoot\n"
"    │        └─ LeftToes\n"
"    ├─ RightUpperLeg\n"
"    │  └─ RightLowerLeg\n"
"    │     └─ RightFoot\n"
"    │        └─ RightToes\n"
"    └─ Spine\n"
"        └─ Chest\n"
"            └─ UpperChest\n"
"                ├─ Neck\n"
"                │   └─ Head\n"
"                │       ├─ Jaw\n"
"                │       ├─ LeftEye\n"
"                │       └─ RightEye\n"
"                ├─ LeftShoulder\n"
"                │  └─ LeftUpperArm\n"
"                │     └─ LeftLowerArm\n"
"                │        └─ LeftHand\n"
"                │           ├─ LeftThumbMetacarpal\n"
"                │           │  └─ LeftThumbProximal\n"
"                │           ├─ LeftIndexProximal\n"
"                │           │  └─ LeftIndexIntermediate\n"
"                │           │    └─ LeftIndexDistal\n"
"                │           ├─ LeftMiddleProximal\n"
"                │           │  └─ LeftMiddleIntermediate\n"
"                │           │    └─ LeftMiddleDistal\n"
"                │           ├─ LeftRingProximal\n"
"                │           │  └─ LeftRingIntermediate\n"
"                │           │    └─ LeftRingDistal\n"
"                │           └─ LeftLittleProximal\n"
"                │              └─ LeftLittleIntermediate\n"
"                │                └─ LeftLittleDistal\n"
"                └─ RightShoulder\n"
"                   └─ RightUpperArm\n"
"                      └─ RightLowerArm\n"
"                         └─ RightHand\n"
"                            ├─ RightThumbMetacarpal\n"
"                            │  └─ RightThumbProximal\n"
"                            ├─ RightIndexProximal\n"
"                            │  └─ RightIndexIntermediate\n"
"                            │     └─ RightIndexDistal\n"
"                            ├─ RightMiddleProximal\n"
"                            │  └─ RightMiddleIntermediate\n"
"                            │     └─ RightMiddleDistal\n"
"                            ├─ RightRingProximal\n"
"                            │  └─ RightRingIntermediate\n"
"                            │     └─ RightRingDistal\n"
"                            └─ RightLittleProximal\n"
"                               └─ RightLittleIntermediate\n"
"                                 └─ RightLittleDistal\n"
 **/
static void auto_mapping_process(Skeleton3D *skeleton, Dictionary &p_bone_map) {
	int bone_idx = -1;
	Vector<String> picklist; // Use Vector<String> because match words have priority.
	Vector<int> search_path;

	// 1. Guess Hips
	picklist.push_back("hip");
	picklist.push_back("pelvis");
	picklist.push_back("waist");
	picklist.push_back("torso");
	picklist.push_back("spine");
	int hips = search_bone_by_name(skeleton, picklist, false);
	if (hips == -1) {
		Vector<int> root = skeleton->get_root_bones();
		if (root.size() == 1) {
			hips = root[0];
		}
	}
	if (hips == -1) {
		WARN_PRINT("Auto Mapping couldn't guess Hips. Abort auto mapping.");
		return; // If there is no Hips, we cannot guess bone after then.
	} else {
		p_bone_map["Hips"] = StringName(skeleton->get_bone_name(hips)); // Hips is always first skeleton->get_bone_name(hips));
	}
	picklist.clear();

	// 2. Guess Root
	bone_idx = skeleton->get_bone_parent(hips);
	while (bone_idx >= 0) {
		search_path.push_back(bone_idx);
		bone_idx = skeleton->get_bone_parent(bone_idx);
	}
	if (search_path.size() == 0) {
		bone_idx = -1;
	} else if (search_path.size() == 1) {
		bone_idx = search_path[0]; // It is only one bone which can be root.
	} else {
		bool found = false;
		for (int i = 0; i < search_path.size(); i++) {
			RegEx re = RegEx("root");
			if (!re.search(skeleton->get_bone_name(search_path[i]).to_lower()).is_null()) {
				bone_idx = search_path[i]; // Name match is preferred.
				found = true;
				break;
			}
		}
		if (!found) {
			for (int i = 0; i < search_path.size(); i++) {
				if (skeleton->get_bone_global_rest(search_path[i]).origin.is_zero_approx()) {
					bone_idx = search_path[i]; // The bone existing at the origin is appropriate as a root.
					found = true;
					break;
				}
			}
		}
		if (!found) {
			bone_idx = search_path[search_path.size() - 1]; // Ambiguous, but most parental bone selected.
		}
	}
	if (bone_idx == -1) {
		WARN_PRINT("Auto Mapping couldn't guess Root."); // Root is not required, so continue.
	} else {
		p_bone_map["Root"] = StringName(skeleton->get_bone_name(bone_idx)); // Root is always first skeleton->get_bone_name(bone_idx));
	}
	bone_idx = -1;
	search_path.clear();

	// 3. Guess Foots
	picklist.push_back("foot");
	picklist.push_back("ankle");
	int left_foot = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, hips);
	if (left_foot == -1) {
		WARN_PRINT("Auto Mapping couldn't guess LeftFoot.");
	} else {
		p_bone_map["LeftFoot"] = StringName(skeleton->get_bone_name(left_foot)); // LeftFoot is always first skeleton->get_bone_name(left_foot));
	}
	int right_foot = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, hips);
	if (right_foot == -1) {
		WARN_PRINT("Auto Mapping couldn't guess RightFoot.");
	} else {
		p_bone_map["RightFoot"] = StringName(skeleton->get_bone_name(right_foot)); // RightFoot is always first skeleton->get_bone_name(right_foot));
	}
	picklist.clear();

	// 3-1. Guess LowerLegs
	picklist.push_back("(low|under).*leg");
	picklist.push_back("knee");
	picklist.push_back("shin");
	picklist.push_back("calf");
	picklist.push_back("leg");
	int left_lower_leg = -1;
	if (left_foot != -1) {
		left_lower_leg = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, hips, left_foot);
	}
	if (left_lower_leg == -1) {
		WARN_PRINT("Auto Mapping couldn't guess LeftLowerLeg.");
	} else {
		p_bone_map["LeftLowerLeg"] = StringName(skeleton->get_bone_name(left_lower_leg)); // LeftLowerLeg is always first skeleton->get_bone_name(left_lower_leg));
	}
	int right_lower_leg = -1;
	if (right_foot != -1) {
		right_lower_leg = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, hips, right_foot);
	}
	if (right_lower_leg == -1) {
		WARN_PRINT("Auto Mapping couldn't guess RightLowerLeg.");
	} else {
		p_bone_map["RightLowerLeg"] = StringName(skeleton->get_bone_name(right_lower_leg)); // RightLowerLeg is always first, skeleton->get_bone_name(right_lower_leg));
	}
	picklist.clear();

	// 3-2. Guess UpperLegs
	picklist.push_back("up.*leg");
	picklist.push_back("thigh");
	picklist.push_back("leg");
	if (left_lower_leg != -1) {
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, hips, left_lower_leg);
	}
	if (bone_idx == -1) {
		WARN_PRINT("Auto Mapping couldn't guess LeftUpperLeg.");
	} else {
		p_bone_map["LeftUpperLeg"] = StringName(skeleton->get_bone_name(bone_idx)); // LeftUpperLeg is always first, skeleton->get_bone_name(bone_idx"LeftUpperLeg", skeleton->get_bone_name(bone_idx));
	}
	bone_idx = -1;
	if (right_lower_leg != -1) {
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, hips, right_lower_leg);
	}
	if (bone_idx == -1) {
		WARN_PRINT("Auto Mapping couldn't guess RightUpperLeg.");
	} else {
		p_bone_map["RightUpperLeg"] = StringName(skeleton->get_bone_name(bone_idx)); // RightUpperLeg is always first skeleton->get_bone_name(bone_idx));
	}
	bone_idx = -1;
	picklist.clear();

	// 3-3. Guess Toes
	picklist.push_back("toe");
	picklist.push_back("ball");
	if (left_foot != -1) {
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, left_foot);
		if (bone_idx == -1) {
			search_path = skeleton->get_bone_children(left_foot);
			if (search_path.size() == 1) {
				bone_idx = search_path[0]; // Maybe only one child of the Foot is Toes.
			}
			search_path.clear();
		}
	}
	if (bone_idx == -1) {
		WARN_PRINT("Auto Mapping couldn't guess LeftToes.");
	} else {
		p_bone_map["LeftToes"] = StringName(skeleton->get_bone_name(bone_idx)); // p_bone_map->_set_skeleton_bone_name("LeftToes", skeleton->get_bone_name(bone_idx));
	}
	bone_idx = -1;
	if (right_foot != -1) {
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, right_foot);
		if (bone_idx == -1) {
			search_path = skeleton->get_bone_children(right_foot);
			if (search_path.size() == 1) {
				bone_idx = search_path[0]; // Maybe only one child of the Foot is Toes.
			}
			search_path.clear();
		}
	}
	if (bone_idx == -1) {
		WARN_PRINT("Auto Mapping couldn't guess RightToes.");
	} else {
		p_bone_map["RightToes"] = StringName(skeleton->get_bone_name(bone_idx)); // p_bone_map->_set_skeleton_bone_name("RightToes", skeleton->get_bone_name(bone_idx));
	}
	bone_idx = -1;
	picklist.clear();

	// 4. Guess Hands
	picklist.push_back("hand");
	picklist.push_back("wrist");
	picklist.push_back("palm");
	picklist.push_back("fingers");
	int left_hand_or_palm = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, hips, -1, 5);
	if (left_hand_or_palm == -1) {
		// Ambiguous, but try again for fewer finger models.
		left_hand_or_palm = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, hips);
	}
	int left_hand = left_hand_or_palm; // Check for the presence of a wrist, since bones with five children may be palmar.
	while (left_hand != -1) {
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, hips, left_hand);
		if (bone_idx == -1) {
			break;
		}
		left_hand = bone_idx;
	}
	if (left_hand == -1) {
		WARN_PRINT("Auto Mapping couldn't guess LeftHand.");
	} else {
		p_bone_map["LeftHand"] = StringName(skeleton->get_bone_name(left_hand)); // LeftHand is always skeleton->get_bone_name(left_hand));
	}
	bone_idx = -1;
	int right_hand_or_palm = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, hips, -1, 5);
	if (right_hand_or_palm == -1) {
		// Ambiguous, but try again for fewer finger models.
		right_hand_or_palm = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, hips);
	}
	int right_hand = right_hand_or_palm;
	while (right_hand != -1) {
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, hips, right_hand);
		if (bone_idx == -1) {
			break;
		}
		right_hand = bone_idx;
	}
	if (right_hand == -1) {
		WARN_PRINT("Auto Mapping couldn't guess RightHand.");
	} else {
		p_bone_map["RightHand"] = StringName(skeleton->get_bone_name(right_hand)); // p_bone_map->_set_skeleton_bone_name("RightHand", skeleton->get_bone_name(right_hand));
	}
	bone_idx = -1;
	picklist.clear();

	// 4-1. Guess Finger
	bool named_finger_is_found = false;
	LocalVector<String> fingers;
	fingers.push_back("thumb|pollex");
	fingers.push_back("index|fore");
	fingers.push_back("middle");
	fingers.push_back("ring");
	fingers.push_back("little|pinkie|pinky");
	if (left_hand_or_palm != -1) {
		LocalVector<LocalVector<String>> left_fingers_map;
		left_fingers_map.resize(5);
		left_fingers_map[0].push_back("LeftThumbMetacarpal");
		left_fingers_map[0].push_back("LeftThumbProximal");
		left_fingers_map[0].push_back("LeftThumbDistal");
		left_fingers_map[1].push_back("LeftIndexProximal");
		left_fingers_map[1].push_back("LeftIndexIntermediate");
		left_fingers_map[1].push_back("LeftIndexDistal");
		left_fingers_map[2].push_back("LeftMiddleProximal");
		left_fingers_map[2].push_back("LeftMiddleIntermediate");
		left_fingers_map[2].push_back("LeftMiddleDistal");
		left_fingers_map[3].push_back("LeftRingProximal");
		left_fingers_map[3].push_back("LeftRingIntermediate");
		left_fingers_map[3].push_back("LeftRingDistal");
		left_fingers_map[4].push_back("LeftLittleProximal");
		left_fingers_map[4].push_back("LeftLittleIntermediate");
		left_fingers_map[4].push_back("LeftLittleDistal");
		for (int i = 0; i < 5; i++) {
			picklist.push_back(fingers[i]);
			int finger = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, left_hand_or_palm, -1, 0);
			if (finger != -1) {
				while (finger != left_hand_or_palm && finger >= 0) {
					search_path.push_back(finger);
					finger = skeleton->get_bone_parent(finger);
				}
				search_path.reverse();
				if (search_path.size() == 1) {
					p_bone_map[left_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					named_finger_is_found = true;
				} else if (search_path.size() == 2) {
					p_bone_map[left_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					p_bone_map[left_fingers_map[i][1]] = StringName(skeleton->get_bone_name(search_path[1])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][1], skeleton->get_bone_name(search_path[1]));
					named_finger_is_found = true;
				} else if (search_path.size() >= 3) {
					search_path = search_path.slice(-3); // Eliminate the possibility of carpal bone.
					p_bone_map[left_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					p_bone_map[left_fingers_map[i][1]] = StringName(skeleton->get_bone_name(search_path[1])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][1], skeleton->get_bone_name(search_path[1]));
					p_bone_map[left_fingers_map[i][2]] = StringName(skeleton->get_bone_name(search_path[2])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][2], skeleton->get_bone_name(search_path[2]));
					named_finger_is_found = true;
				}
			}
			picklist.clear();
			search_path.clear();
		}

		// It is a bit corner case, but possibly the finger names are sequentially numbered...
		if (!named_finger_is_found) {
			picklist.push_back("finger");
			RegEx finger_re = RegEx("finger");
			search_path = skeleton->get_bone_children(left_hand_or_palm);
			Vector<String> finger_names;
			for (int i = 0; i < search_path.size(); i++) {
				String bn = skeleton->get_bone_name(search_path[i]);
				if (!finger_re.search(bn.to_lower()).is_null()) {
					finger_names.push_back(bn);
				}
			}
			finger_names.sort(); // Order by lexicographic, normal use cases never have more than 10 fingers in one hand.
			search_path.clear();
			for (int i = 0; i < finger_names.size(); i++) {
				if (i >= 5) {
					break;
				}
				int finger_root = skeleton->find_bone(finger_names[i]);
				int finger = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, finger_root, -1, 0);
				if (finger != -1) {
					while (finger != finger_root && finger >= 0) {
						search_path.push_back(finger);
						finger = skeleton->get_bone_parent(finger);
					}
				}
				search_path.push_back(finger_root);
				search_path.reverse();
				if (search_path.size() == 1) {
					p_bone_map[left_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
				} else if (search_path.size() == 2) {
					p_bone_map[left_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					p_bone_map[left_fingers_map[i][1]] = StringName(skeleton->get_bone_name(search_path[1])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][1], skeleton->get_bone_name(search_path[1]));
				} else if (search_path.size() >= 3) {
					search_path = search_path.slice(-3); // Eliminate the possibility of carpal bone.
					p_bone_map[left_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					p_bone_map[left_fingers_map[i][1]] = StringName(skeleton->get_bone_name(search_path[1])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][1], skeleton->get_bone_name(search_path[1]));
					p_bone_map[left_fingers_map[i][2]] = StringName(skeleton->get_bone_name(search_path[2])); // p_bone_map->_set_skeleton_bone_name(left_fingers_map[i][2], skeleton->get_bone_name(search_path[2]));
				}
				search_path.clear();
			}
			picklist.clear();
		}
	}
	named_finger_is_found = false;
	if (right_hand_or_palm != -1) {
		LocalVector<LocalVector<String>> right_fingers_map;
		right_fingers_map.resize(5);
		right_fingers_map[0].push_back("RightThumbMetacarpal");
		right_fingers_map[0].push_back("RightThumbProximal");
		right_fingers_map[0].push_back("RightThumbDistal");
		right_fingers_map[1].push_back("RightIndexProximal");
		right_fingers_map[1].push_back("RightIndexIntermediate");
		right_fingers_map[1].push_back("RightIndexDistal");
		right_fingers_map[2].push_back("RightMiddleProximal");
		right_fingers_map[2].push_back("RightMiddleIntermediate");
		right_fingers_map[2].push_back("RightMiddleDistal");
		right_fingers_map[3].push_back("RightRingProximal");
		right_fingers_map[3].push_back("RightRingIntermediate");
		right_fingers_map[3].push_back("RightRingDistal");
		right_fingers_map[4].push_back("RightLittleProximal");
		right_fingers_map[4].push_back("RightLittleIntermediate");
		right_fingers_map[4].push_back("RightLittleDistal");
		for (int i = 0; i < 5; i++) {
			picklist.push_back(fingers[i]);
			int finger = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, right_hand_or_palm, -1, 0);
			if (finger != -1) {
				while (finger != right_hand_or_palm && finger >= 0) {
					search_path.push_back(finger);
					finger = skeleton->get_bone_parent(finger);
				}
				search_path.reverse();
				if (search_path.size() == 1) {
					p_bone_map[right_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					named_finger_is_found = true;
				} else if (search_path.size() == 2) {
					p_bone_map[right_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					p_bone_map[right_fingers_map[i][1]] = StringName(skeleton->get_bone_name(search_path[1])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][1], skeleton->get_bone_name(search_path[1]));
					named_finger_is_found = true;
				} else if (search_path.size() >= 3) {
					search_path = search_path.slice(-3); // Eliminate the possibility of carpal bone.
					p_bone_map[right_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					p_bone_map[right_fingers_map[i][1]] = StringName(skeleton->get_bone_name(search_path[1])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][1], skeleton->get_bone_name(search_path[1]));
					p_bone_map[right_fingers_map[i][2]] = StringName(skeleton->get_bone_name(search_path[2])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][2], skeleton->get_bone_name(search_path[2]));
					named_finger_is_found = true;
				}
			}
			picklist.clear();
			search_path.clear();
		}

		// It is a bit corner case, but possibly the finger names are sequentially numbered...
		if (!named_finger_is_found) {
			picklist.push_back("finger");
			RegEx finger_re = RegEx("finger");
			search_path = skeleton->get_bone_children(right_hand_or_palm);
			Vector<String> finger_names;
			for (int i = 0; i < search_path.size(); i++) {
				String bn = skeleton->get_bone_name(search_path[i]);
				if (!finger_re.search(bn.to_lower()).is_null()) {
					finger_names.push_back(bn);
				}
			}
			finger_names.sort(); // Order by lexicographic, normal use cases never have more than 10 fingers in one hand.
			search_path.clear();
			for (int i = 0; i < finger_names.size(); i++) {
				if (i >= 5) {
					break;
				}
				int finger_root = skeleton->find_bone(finger_names[i]);
				int finger = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, finger_root, -1, 0);
				if (finger != -1) {
					while (finger != finger_root && finger >= 0) {
						search_path.push_back(finger);
						finger = skeleton->get_bone_parent(finger);
					}
				}
				search_path.push_back(finger_root);
				search_path.reverse();
				if (search_path.size() == 1) {
					p_bone_map[right_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
				} else if (search_path.size() == 2) {
					p_bone_map[right_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					p_bone_map[right_fingers_map[i][1]] = StringName(skeleton->get_bone_name(search_path[1])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][1], skeleton->get_bone_name(search_path[1]));
				} else if (search_path.size() >= 3) {
					search_path = search_path.slice(-3); // Eliminate the possibility of carpal bone.
					p_bone_map[right_fingers_map[i][0]] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][0], skeleton->get_bone_name(search_path[0]));
					p_bone_map[right_fingers_map[i][1]] = StringName(skeleton->get_bone_name(search_path[1])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][1], skeleton->get_bone_name(search_path[1]));
					p_bone_map[right_fingers_map[i][2]] = StringName(skeleton->get_bone_name(search_path[2])); // p_bone_map->_set_skeleton_bone_name(right_fingers_map[i][2], skeleton->get_bone_name(search_path[2]));
				}
				search_path.clear();
			}
			picklist.clear();
		}
	}

	// 5. Guess Arms
	picklist.push_back("shoulder");
	picklist.push_back("clavicle");
	picklist.push_back("collar");
	int left_shoulder = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, hips);
	if (left_shoulder == -1) {
		WARN_PRINT("Auto Mapping couldn't guess LeftShoulder.");
	} else {
		p_bone_map["LeftShoulder"] = StringName(skeleton->get_bone_name(left_shoulder)); // p_bone_map->_set_skeleton_bone_name("LeftShoulder", skeleton->get_bone_name(left_shoulder));
	}
	int right_shoulder = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, hips);
	if (right_shoulder == -1) {
		WARN_PRINT("Auto Mapping couldn't guess RightShoulder.");
	} else {
		p_bone_map["RightShoulder"] = StringName(skeleton->get_bone_name(right_shoulder)); // p_bone_map->_set_skeleton_bone_name("RightShoulder", skeleton->get_bone_name(right_shoulder));
	}
	picklist.clear();

	// 5-1. Guess LowerArms
	picklist.push_back("(low|fore).*arm");
	picklist.push_back("elbow");
	picklist.push_back("arm");
	int left_lower_arm = -1;
	if (left_shoulder != -1 && left_hand_or_palm != -1) {
		left_lower_arm = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, left_shoulder, left_hand_or_palm);
	}
	if (left_lower_arm == -1) {
		WARN_PRINT("Auto Mapping couldn't guess LeftLowerArm.");
	} else {
		p_bone_map["LeftLowerArm"] = StringName(skeleton->get_bone_name(left_lower_arm)); // p_bone_map->_set_skeleton_bone_name("LeftLowerArm", skeleton->get_bone_name(left_lower_arm));
	}
	int right_lower_arm = -1;
	if (right_shoulder != -1 && right_hand_or_palm != -1) {
		right_lower_arm = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, right_shoulder, right_hand_or_palm);
	}
	if (right_lower_arm == -1) {
		WARN_PRINT("Auto Mapping couldn't guess RightLowerArm.");
	} else {
		p_bone_map["RightLowerArm"] = StringName(skeleton->get_bone_name(right_lower_arm)); // p_bone_map->_set_skeleton_bone_name("RightLowerArm", skeleton->get_bone_name(right_lower_arm));
	}
	picklist.clear();

	// 5-2. Guess UpperArms
	picklist.push_back("up.*arm");
	picklist.push_back("arm");
	if (left_shoulder != -1 && left_lower_arm != -1) {
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, left_shoulder, left_lower_arm);
	}
	if (bone_idx == -1) {
		WARN_PRINT("Auto Mapping couldn't guess LeftUpperArm.");
	} else {
		p_bone_map["LeftUpperArm"] = StringName(skeleton->get_bone_name(bone_idx)); // p_bone_map->_set_skeleton_bone_name("LeftUpperArm", skeleton->get_bone_name(bone_idx));
	}
	bone_idx = -1;
	if (right_shoulder != -1 && right_lower_arm != -1) {
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, right_shoulder, right_lower_arm);
	}
	if (bone_idx == -1) {
		WARN_PRINT("Auto Mapping couldn't guess RightUpperArm.");
	} else {
		p_bone_map["RightUpperArm"] = StringName(skeleton->get_bone_name(bone_idx)); // p_bone_map->_set_skeleton_bone_name("RightUpperArm", skeleton->get_bone_name(bone_idx));
	}
	bone_idx = -1;
	picklist.clear();

	// 6. Guess Neck
	picklist.push_back("neck");
	picklist.push_back("head"); // For no neck model.
	picklist.push_back("face"); // Same above.
	int neck = search_bone_by_name(skeleton, picklist, false, BONE_SEGREGATION_NONE, hips);
	picklist.clear();
	if (neck == -1) {
		// If it can't expect by name, search child spine of where the right and left shoulders (or hands) cross.
		int ls_idx = left_shoulder != -1 ? left_shoulder : (left_hand_or_palm != -1 ? left_hand_or_palm : -1);
		int rs_idx = right_shoulder != -1 ? right_shoulder : (right_hand_or_palm != -1 ? right_hand_or_palm : -1);
		if (ls_idx != -1 && rs_idx != -1) {
			bool detect = false;
			while (ls_idx != hips && ls_idx >= 0 && rs_idx != hips && rs_idx >= 0) {
				ls_idx = skeleton->get_bone_parent(ls_idx);
				rs_idx = skeleton->get_bone_parent(rs_idx);
				if (ls_idx == rs_idx) {
					detect = true;
					break;
				}
			}
			if (detect) {
				Vector<int> children = skeleton->get_bone_children(ls_idx);
				children.erase(ls_idx);
				children.erase(rs_idx);
				String word = "spine"; // It would be better to limit the search with "spine" because it could be mistaken with breast, wing and etc...
				for (int i = 0; i < children.size(); i++) {
					bone_idx = children[i];
					if (is_match_with_bone_name(skeleton->get_bone_name(bone_idx), word)) {
						neck = bone_idx;
						break;
					};
				}
				bone_idx = -1;
			}
		}
	}

	// 7. Guess Head
	picklist.push_back("head");
	picklist.push_back("face");
	int head = search_bone_by_name(skeleton, picklist, false, BONE_SEGREGATION_NONE, neck);
	if (head == -1) {
		if (neck != -1) {
			search_path = skeleton->get_bone_children(neck);
			if (search_path.size() == 1) {
				head = search_path[0]; // Maybe only one child of the Neck is Head.
			}
		}
	}
	if (head == -1) {
		if (neck != -1) {
			head = neck; // The head animation should have more movement.
			neck = -1;
			p_bone_map["Head"] = StringName(skeleton->get_bone_name(head)); // p_bone_map->_set_skeleton_bone_name("Head", skeleton->get_bone_name(head));
		} else {
			//WARN_PRINT("Auto Mapping couldn't guess Neck or Head."); // Continued for guessing on the other bones. But abort when guessing spines step.
		}
	} else {
		p_bone_map["Neck"] = StringName(skeleton->get_bone_name(neck)); // p_bone_map->_set_skeleton_bone_name("Neck", skeleton->get_bone_name(neck));
		p_bone_map["Head"] = StringName(skeleton->get_bone_name(head)); // p_bone_map->_set_skeleton_bone_name("Head", skeleton->get_bone_name(head));
	}
	picklist.clear();
	search_path.clear();

	int neck_or_head = neck != -1 ? neck : (head != -1 ? head : -1);
	if (neck_or_head != -1) {
		// 7-1. Guess Eyes
		picklist.push_back("eye(?!.*(brow|lash|lid))");
		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_LEFT, neck_or_head);
		if (bone_idx == -1) {
			//WARN_PRINT("Auto Mapping couldn't guess LeftEye.");
		} else {
			p_bone_map[("LeftEye")] = StringName(skeleton->get_bone_name(bone_idx)); // p_bone_map->_set_skeleton_bone_name("LeftEye", skeleton->get_bone_name(bone_idx));
		}

		bone_idx = search_bone_by_name(skeleton, picklist, true, BONE_SEGREGATION_RIGHT, neck_or_head);
		if (bone_idx == -1) {
			//WARN_PRINT("Auto Mapping couldn't guess RightEye.");
		} else {
			p_bone_map[("RightEye")] = StringName(skeleton->get_bone_name(bone_idx)); // p_bone_map->_set_skeleton_bone_name("RightEye", skeleton->get_bone_name(bone_idx));
		}
		picklist.clear();

		// 7-2. Guess Jaw
		picklist.push_back("jaw");
		bone_idx = search_bone_by_name(skeleton, picklist, false, BONE_SEGREGATION_NONE, neck_or_head);
		if (bone_idx == -1) {
			//WARN_PRINT("Auto Mapping couldn't guess Jaw.");
		} else {
			p_bone_map[("Jaw")] = StringName(skeleton->get_bone_name(bone_idx)); // p_bone_map->_set_skeleton_bone_name("Jaw", skeleton->get_bone_name(bone_idx));
		}
		bone_idx = -1;
		picklist.clear();
	}

	// 8. Guess UpperChest or Chest
	if (neck_or_head == -1) {
		return; // Abort.
	}
	int chest_or_upper_chest = skeleton->get_bone_parent(neck_or_head);
	bool is_appropriate = true;
	if (left_shoulder != -1) {
		bone_idx = skeleton->get_bone_parent(left_shoulder);
		bool detect = false;
		while (bone_idx != hips && bone_idx >= 0) {
			if (bone_idx == chest_or_upper_chest) {
				detect = true;
				break;
			}
			bone_idx = skeleton->get_bone_parent(bone_idx);
		}
		if (!detect) {
			is_appropriate = false;
		}
		bone_idx = -1;
	}
	if (right_shoulder != -1) {
		bone_idx = skeleton->get_bone_parent(right_shoulder);
		bool detect = false;
		while (bone_idx != hips && bone_idx >= 0) {
			if (bone_idx == chest_or_upper_chest) {
				detect = true;
				break;
			}
			bone_idx = skeleton->get_bone_parent(bone_idx);
		}
		if (!detect) {
			is_appropriate = false;
		}
		bone_idx = -1;
	}
	if (!is_appropriate) {
		if (skeleton->get_bone_parent(left_shoulder) == skeleton->get_bone_parent(right_shoulder)) {
			chest_or_upper_chest = skeleton->get_bone_parent(left_shoulder);
		} else {
			chest_or_upper_chest = -1;
		}
	}
	if (chest_or_upper_chest == -1) {
		WARN_PRINT("Auto Mapping couldn't guess Chest or UpperChest. Abort auto mapping.");
		return; // Will be not able to guess Spines.
	}

	// 9. Guess Spines
	bone_idx = skeleton->get_bone_parent(chest_or_upper_chest);
	while (bone_idx != hips && bone_idx >= 0) {
		search_path.push_back(bone_idx);
		bone_idx = skeleton->get_bone_parent(bone_idx);
	}
	search_path.reverse();
	if (search_path.size() == 0) {
		p_bone_map[("Spine")] = StringName(skeleton->get_bone_name(chest_or_upper_chest)); // p_bone_map->_set_skeleton_bone_name("Spine", skeleton->get_bone_name(chest_or_upper_chest)); // Maybe chibi model...?
	} else if (search_path.size() == 1) {
		p_bone_map[("Spine")] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name("Spine", skeleton->get_bone_name(search_path[0]));
		p_bone_map[("Chest")] = StringName(skeleton->get_bone_name(chest_or_upper_chest)); // p_bone_map->_set_skeleton_bone_name("Chest", skeleton->get_bone_name(chest_or_upper_chest));
	} else if (search_path.size() >= 2) {
		p_bone_map[("Spine")] = StringName(skeleton->get_bone_name(search_path[0])); // p_bone_map->_set_skeleton_bone_name("Spine", skeleton->get_bone_name(search_path[0]));
		p_bone_map[("Chest")] = StringName(skeleton->get_bone_name(search_path[search_path.size() - 1])); // p_bone_map->_set_skeleton_bone_name("Chest", skeleton->get_bone_name(search_path[search_path.size() - 1])); // Probably UppeChest's parent is appropriate.
		p_bone_map[("UpperChest")] = StringName(skeleton->get_bone_name(chest_or_upper_chest)); // p_bone_map->_set_skeleton_bone_name("UpperChest", skeleton->get_bone_name(chest_or_upper_chest));
	}
	bone_idx = -1;
	search_path.clear();

	// 检测是否骨骼翻转
	{
		Vector<String> right_bones;

		right_bones.push_back("RightThumbMetacarpal");
		right_bones.push_back("RightThumbProximal");
		right_bones.push_back("RightThumbDistal");
		right_bones.push_back("RightIndexProximal");
		right_bones.push_back("RightIndexIntermediate");
		right_bones.push_back("RightIndexDistal");
		right_bones.push_back("RightMiddleProximal");
		right_bones.push_back("RightMiddleIntermediate");
		right_bones.push_back("RightMiddleDistal");
		right_bones.push_back("RightRingProximal");
		right_bones.push_back("RightRingIntermediate");
		right_bones.push_back("RightRingDistal");
		right_bones.push_back("RightLittleProximal");
		right_bones.push_back("RightLittleIntermediate");
		right_bones.push_back("RightLittleDistal");
		right_bones.push_back("RightShoulder");
		right_bones.push_back("RightUpperArm");
		right_bones.push_back("RightLowerArm");
		right_bones.push_back("RightHand");
		right_bones.push_back("RightEye");
		right_bones.push_back("RightShoulder");
		right_bones.push_back("RightUpperArm");
		right_bones.push_back("RightLowerArm");
		right_bones.push_back("RightHand");
		right_bones.push_back("RightUpperLeg");
		right_bones.push_back("RightLowerLeg");
		right_bones.push_back("RightFoot");
		right_bones.push_back("RightToes");

		Vector<String> left_bones;
		left_bones.push_back("LeftThumbMetacarpal");
		left_bones.push_back("LeftThumbProximal");
		left_bones.push_back("LeftThumbDistal");
		left_bones.push_back("LeftIndexProximal");
		left_bones.push_back("LeftIndexIntermediate");
		left_bones.push_back("LeftIndexDistal");
		left_bones.push_back("LeftMiddleProximal");
		left_bones.push_back("LeftMiddleIntermediate");
		left_bones.push_back("LeftMiddleDistal");
		left_bones.push_back("LeftRingProximal");
		left_bones.push_back("LeftRingIntermediate");
		left_bones.push_back("LeftRingDistal");
		left_bones.push_back("LeftLittleProximal");
		left_bones.push_back("LeftLittleIntermediate");
		left_bones.push_back("LeftLittleDistal");
		left_bones.push_back("LeftShoulder");
		left_bones.push_back("LeftUpperArm");
		left_bones.push_back("LeftLowerArm");
		left_bones.push_back("LeftHand");
		left_bones.push_back("LeftEye");
		left_bones.push_back("LeftShoulder");
		left_bones.push_back("LeftUpperArm");
		left_bones.push_back("LeftLowerArm");
		left_bones.push_back("LeftHand");
		left_bones.push_back("LeftUpperLeg");
		left_bones.push_back("LeftLowerLeg");
		left_bones.push_back("LeftFoot");
		left_bones.push_back("LeftToes");
		bone_idx = skeleton->get_bone_parent(p_bone_map[("Spine")]);
		Vector3 center = skeleton->get_bone_global_pose(chest_or_upper_chest).origin;
		for (int i = 0; i < right_bones.size(); i++) {
			int left_bone_idx = -1, right_bones_idx = -1;
			String left_bone_name, right_bone_name;

			if (p_bone_map.has(right_bones[i])) {
				right_bone_name = p_bone_map[right_bones[i]];
				right_bones_idx = skeleton->find_bone(right_bone_name);
			}
			if (p_bone_map.has(left_bones[i])) {
				left_bone_name = p_bone_map[left_bones[i]];
				left_bone_idx = skeleton->find_bone(left_bone_name);
			}
			Vector3 right_bone;
			Vector3 left_bone;
			if (right_bones_idx != -1) {
				right_bone = skeleton->get_bone_global_pose(right_bones_idx).origin - center;
				if (right_bone.x > 0) {
					if (left_bone_idx != -1) {
					}
				}
			}
			if (left_bone_idx != -1) {
				left_bone = skeleton->get_bone_global_pose(left_bone_idx).origin - center;
			}
			if (left_bone.x < 0 || right_bone.x > 0) {
				if (left_bone_idx != -1) {
					p_bone_map[right_bones[i]] = left_bone_name;
				} else {
					p_bone_map.erase(right_bones[i]);
				}

				if (right_bones_idx != -1) {
					p_bone_map[left_bones[i]] = right_bone_name;
				} else {
					p_bone_map.erase(left_bones[i]);
				}
			}
		}
	}

	//WARN_PRINT("Finish auto mapping.");
}

void Skeleton3D::change_to_human_bone(int p_bone) {
	while (p_bone != -1) {
		bones[p_bone].is_human_bone = true;
		p_bone = bones[p_bone].parent;
	}
}
const HashSet<String> &get_human_bones() {
	static HashSet<String> human_bones;
	static bool is_init = false;
	if (!is_init) {
		is_init = true;
		human_bones.insert("LeftThumbMetacarpal");
		human_bones.insert("LeftThumbProximal");
		human_bones.insert("LeftThumbDistal");
		human_bones.insert("LeftIndexProximal");
		human_bones.insert("LeftIndexIntermediate");
		human_bones.insert("LeftIndexDistal");
		human_bones.insert("LeftMiddleProximal");
		human_bones.insert("LeftMiddleIntermediate");
		human_bones.insert("LeftMiddleDistal");
		human_bones.insert("LeftRingProximal");
		human_bones.insert("LeftRingIntermediate");
		human_bones.insert("LeftRingDistal");
		human_bones.insert("LeftLittleProximal");
		human_bones.insert("LeftLittleIntermediate");
		human_bones.insert("LeftLittleDistal");
		human_bones.insert("RightThumbMetacarpal");
		human_bones.insert("RightThumbProximal");
		human_bones.insert("RightThumbDistal");
		human_bones.insert("RightIndexProximal");
		human_bones.insert("RightIndexIntermediate");
		human_bones.insert("RightIndexDistal");
		human_bones.insert("RightMiddleProximal");
		human_bones.insert("RightMiddleIntermediate");
		human_bones.insert("RightMiddleDistal");
		human_bones.insert("RightRingProximal");
		human_bones.insert("RightRingIntermediate");
		human_bones.insert("RightRingDistal");
		human_bones.insert("RightLittleProximal");
		human_bones.insert("RightLittleIntermediate");
		human_bones.insert("RightLittleDistal");

		human_bones.insert("Spine");
		human_bones.insert("Chest");
		human_bones.insert("UpperChest");
		human_bones.insert("Neck");
		human_bones.insert("Head");
		human_bones.insert("LeftEye");
		human_bones.insert("RightEye");
		human_bones.insert("Jaw");
		human_bones.insert("LeftShoulder");
		human_bones.insert("LeftUpperArm");
		human_bones.insert("LeftLowerArm");
		human_bones.insert("LeftHand");
		human_bones.insert("RightShoulder");
		human_bones.insert("RightUpperArm");
		human_bones.insert("RightLowerArm");
		human_bones.insert("RightHand");
		human_bones.insert("LeftUpperLeg");
		human_bones.insert("LeftLowerLeg");
		human_bones.insert("LeftFoot");
		human_bones.insert("LeftToes");
		human_bones.insert("RightUpperLeg");
		human_bones.insert("RightLowerLeg");
		human_bones.insert("RightFoot");
		human_bones.insert("RightToes");
	}
	return human_bones;
}
Dictionary Skeleton3D::get_human_bone_mapping() {
	Dictionary human_bone_mapping;
	auto_mapping_process(this, human_bone_mapping);
	Dictionary rs;
	LocalVector<Variant> keys = human_bone_mapping.get_key_list();

	for (const Variant &E : keys) {
		rs[human_bone_mapping[E]] = E;
	}
	return rs;
}

void Skeleton3D::set_human_bone_mapping(const Dictionary &p_human_bone_mapping) {
	LocalVector<Variant> keys = p_human_bone_mapping.get_key_list();
	const HashSet<String> &human_bones = get_human_bones();
	for (const Variant &E : keys) {
		int bone_index = this->find_bone(E);
		auto &bone = p_human_bone_mapping[E];
		this->set_bone_name(bone_index, bone);
		if (human_bones.has(bone)) {
			change_to_human_bone(bone_index);
		}
	}
}
Vector<String> Skeleton3D::get_bone_names() const {
	Vector<String> bone_names;
	for (uint32_t i = 0; i < bones.size(); i++) {
		bone_names.push_back(get_bone_name(i));
	}
	return bone_names;
}
