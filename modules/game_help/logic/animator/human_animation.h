#pragma once

#include "scene/3d/skeleton_3d.h"
#include "scene/resources/animation.h"

namespace HumanAnim {
// 骨骼配置
struct HumanSkeleton {
	HashMap<StringName, Quaternion> real_local_pose;
	HashMap<StringName, Transform3D> real_global_pose;
	HashMap<StringName, Vector3> bone_global_lookat;
	HashMap<StringName, float> bone_global_roll;

	HashMap<StringName, Quaternion> bone_global_rotation;
	HashMap<StringName, Vector3> root_position;
	HashMap<StringName, Vector3> root_lookat;

	HashMap<StringName, Basis> root_global_rotation;
	HashMap<StringName, Vector3> root_global_move_add;
	HashMap<StringName, Basis> root_global_rotation_add;

	void rest(HumanBoneConfig &p_config) {
		clear();
		for (auto &it : p_config.virtual_pose) {
			real_local_pose[it.key] = it.value.rotation;
			Transform3D &trans = real_global_pose[it.key];
			trans = it.value.local_pose;
			//trans.basis = Basis(it.value.rotation);
			//trans.origin = it.value.position;
		}

		for (auto &it : p_config.root_bone) {
			root_position[it] = Vector3();
			root_global_rotation[it] = Quaternion();
			root_global_move_add[it] = Vector3();
			root_global_rotation_add[it] = Quaternion();
		}
	}

	void clear() {
		real_local_pose.clear();
		real_global_pose.clear();
		bone_global_lookat.clear();
	}

	void set_human_lookat(StringName p_bone, const Vector3 &p_lookat) {
		StringName name;
		HumanAnimationBoneNameMapping *mapping = HumanAnimationBoneNameMapping::get_singleton();
		if (mapping != nullptr) {
			name = mapping->get_bone_name(p_bone);
		} else {
			name = p_bone.substr(5);
		}
		bone_global_lookat[name] = p_lookat;
	}

	void set_human_roll(StringName p_bone, float p_roll) {
		StringName name;
		HumanAnimationBoneNameMapping *mapping = HumanAnimationBoneNameMapping::get_singleton();
		if (mapping != nullptr) {
			name = mapping->get_bone_name(p_bone);
		} else {
			name = p_bone.substr(5);
		}
		bone_global_roll[name] = p_roll;
	}
	static Basis retarget_root_direction(const Vector3 &p_start_direction, const Vector3 &p_end_direction) {
		Basis basis;
		const Vector3 axis = p_start_direction.cross(p_end_direction).normalized();
		if (axis.length_squared() != 0) {
			real_t dot = p_start_direction.dot(p_end_direction);
			dot = CLAMP(dot, -1.0f, 1.0f);
			const real_t angle_rads = Math::acos(dot);
			basis = Basis(axis, angle_rads);
		}
		return basis;
	}

	static Basis compute_lookat_rotation_add(Ref<Animation> p_animation, int track_index, double time_start, double time_end) {
		Vector3 loc, loc2;
		p_animation->try_position_track_interpolate(track_index, time_start, &loc);
		p_animation->try_position_track_interpolate(track_index, time_end, &loc2);
		return retarget_root_direction(loc, loc2);
	}

	static Vector3 compute_lookat_position_add(Ref<Animation> p_animation, int track_index, double time_start, double time_end) {
		Vector3 loc, loc2;
		p_animation->try_position_track_interpolate(track_index, time_start, &loc);
		p_animation->try_position_track_interpolate(track_index, time_end, &loc2);
		return loc2 - loc;
	}

	void set_root_lookat(Ref<Animation> p_animation, StringName p_bone, int track_index, double time, double delta) {
		if (delta == 0) {
			return;
		}

		Basis q;

		double last_time = time - delta;

		if (delta >= 0) {
			if (last_time < 0) {
				q = compute_lookat_rotation_add(p_animation, track_index, 0, time) * compute_lookat_rotation_add(p_animation, track_index, p_animation->get_length() + last_time, p_animation->get_length());
			} else {
				q = compute_lookat_rotation_add(p_animation, track_index, last_time, time);
			}
		} else {
			if (last_time > p_animation->get_length()) {
				q = compute_lookat_rotation_add(p_animation, track_index, time, p_animation->get_length()) * compute_lookat_rotation_add(p_animation, track_index, last_time - p_animation->get_length(), 0);
			} else {
				q = compute_lookat_rotation_add(p_animation, track_index, last_time, time);
			}
		}
		StringName name;

		HumanAnimationBoneNameMapping *mapping = HumanAnimationBoneNameMapping::get_singleton();
		if (mapping != nullptr) {
			name = mapping->get_bone_name(p_bone);
		} else {
			name = p_bone.substr(5);
		}
		root_global_rotation_add[name] = q;
		//         Vector3 loc;
		//         p_animation->try_position_track_interpolate(track_index, time, &loc);
		//root_lookat[name] = loc;
	}

	void set_root_position_add(Ref<Animation> p_animation, StringName p_bone, int track_index, double time, double delta) {
		if (delta == 0) {
			return;
		}

		Vector3 q;

		double last_time = time - delta;

		if (delta >= 0) {
			if (last_time < 0) {
				q = compute_lookat_position_add(p_animation, track_index, 0, time) + compute_lookat_position_add(p_animation, track_index, p_animation->get_length() + last_time, p_animation->get_length());
			} else {
				q = compute_lookat_position_add(p_animation, track_index, last_time, time);
			}
		} else {
			if (last_time > p_animation->get_length()) {
				q = compute_lookat_position_add(p_animation, track_index, time, p_animation->get_length()) + compute_lookat_position_add(p_animation, track_index, last_time - p_animation->get_length(), 0);
			} else {
				q = compute_lookat_position_add(p_animation, track_index, last_time, time);
			}
		}
		StringName name;

		HumanAnimationBoneNameMapping *mapping = HumanAnimationBoneNameMapping::get_singleton();
		if (mapping != nullptr) {
			name = mapping->get_bone_name(p_bone);
		} else {
			name = p_bone.substr(5);
		}
		root_global_move_add[name] = q;
		//         Vector3 loc;
		//         p_animation->try_position_track_interpolate(track_index, time, &loc);
		//root_position[name] = loc;
	}

	void blend(HumanSkeleton &p_other, float p_weight) {
		for (auto &it : p_other.real_local_pose) {
			if (real_local_pose.has(it.key)) {
				Quaternion &q = real_local_pose[it.key];
				q = q.slerp(it.value, p_weight);
			}
		}
		for (auto &it : p_other.root_position) {
			if (root_global_move_add.has(it.key)) {
				Vector3 &v = root_position[it.key];
				v = v.lerp(it.value, p_weight);
			}
		}
		for (auto &it : p_other.root_global_rotation) {
			if (root_global_move_add.has(it.key)) {
				Basis &v = root_global_rotation[it.key];
				v = v.lerp(it.value, p_weight);
			}
		}

		for (auto &it : p_other.root_global_move_add) {
			if (root_global_move_add.has(it.key)) {
				Vector3 &v = root_global_move_add[it.key];
				v = v.lerp(it.value, p_weight);
			}
		}
		for (auto &it : p_other.root_global_rotation_add) {
			if (root_global_rotation_add.has(it.key)) {
				Basis &q = root_global_rotation_add[it.key];
				q = q.slerp(it.value, p_weight);
			}
		}
	}

	void apply_root_motion(Node3D *node) {
		Transform3D curr_trans = node->get_transform();

		Transform3D add_trans;
		if (root_global_rotation_add.size() > 0) {
			add_trans.basis = root_global_rotation_add.begin()->value;
		}

		if (root_global_move_add.size() > 0) {
			add_trans.origin = add_trans.basis.xform(root_global_move_add.begin()->value);
		}

		node->set_transform(add_trans * curr_trans);
	}
	static const HashMap<String, float> &get_bone_blend_weight() {
		static HashMap<String, float> label_map = {

			{ "LeftShoulder", 0.3f },
			{ "RightShoulder", 0.3f }

		};
		return label_map;
	}

	void apply(Skeleton3D *p_skeleton, const HashMap<String, float> &bone_blend_weight, float p_weight) {
		for (auto &it : real_local_pose) {
			int bone_index = p_skeleton->find_bone(it.key);
			if (bone_index >= 0) {
				float weight = 1.0f;
				if (bone_blend_weight.has(it.key)) {
					weight = bone_blend_weight[it.key];
				}
				p_skeleton->set_bone_pose_rotation(bone_index, p_skeleton->get_bone_pose_rotation(bone_index).slerp(it.value, p_weight * weight),false);
			}
		}
	}

	void apply_root_motion(Vector3 &p_position, Quaternion &p_rotation, Vector3 &p_position_add, Quaternion &p_rotation_add, float p_weight) {
		if (root_global_rotation.size() > 0) {
			p_rotation = p_rotation.slerp(root_global_rotation.begin()->value.get_rotation_quaternion(), p_weight);
		}

		if (root_global_rotation_add.size() > 0) {
			p_rotation_add = p_rotation.slerp(root_global_rotation_add.begin()->value.get_rotation_quaternion(), p_weight);
		}

		if (root_position.size() > 0) {
			p_position = p_position.lerp(root_position.begin()->value, p_weight);
		}

		if (root_global_move_add.size() > 0) {
			p_position_add = p_position_add.lerp(root_global_move_add.begin()->value, p_weight);
		}
	}
};

class HumanAnimmation {
public:
	/**
	提取动画姿势:
	把动画文件的骨骼旋转应用到虚拟骨骼,获取到世界位置,河道世界位置


	虚拟骨骼:
	长度为1的骨骼,姿势保持和原始骨骼一致


	应用动画姿势:
	用当前角色的虚拟骨骼从父节点到子节点进行世界空间的Lookat计算
	用虚拟骨骼的世界变换得到局部旋转
	应用虚拟骨骼的局部旋转到真实骨骼
	Hip骨骼的世界朝向应用到角色的朝向,Hip的世界位置应用到角色的位置
	*/
	// 构建虚拟姿势
	static void computer_bone_right(HumanBoneConfig &p_config, BonePose &parent_pose, BonePose &pose) {
		Vector3 forward;
		if (pose.child_bones.size() == 0) {
			forward = pose.global_pose.origin - parent_pose.global_pose.origin;
		} else {
			BonePose &child_pose = p_config.virtual_pose[pose.child_bones[0]];
			forward = child_pose.global_pose.origin - parent_pose.global_pose.origin;
		}

		Vector3::Axis min_axis = forward.min_axis_index();

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
		Vector3 right = up.cross(forward.normalized());

		pose.right = pose.global_pose.basis.inverse().xform(right);
		pose.right.normalize();
	}
	static void build_virtual_pose(Skeleton3D *p_skeleton, HumanBoneConfig &p_config, HashMap<String, String> &p_human_bone_label) {
		Vector<int> root_bones = p_skeleton->get_root_bones();
		p_skeleton->_update_bones_nested_set();
		p_skeleton->force_update_all_bone_transforms(false);

		auto rbs = p_skeleton->get_root_bones();

		for (int root_bone = 0; root_bone < rbs.size(); ++root_bone) {
			StringName bone_name = p_skeleton->get_bone_name(root_bone);
			BonePose pose;
			Transform3D trans = p_skeleton->get_bone_global_pose(root_bone);

			Vector<int> children = p_skeleton->get_bone_children(root_bone);
			for (int j = 0; j < children.size(); j++) {
				StringName child_name = p_skeleton->get_bone_name(children[j]);
				if (p_human_bone_label.has(child_name)) {
					pose.child_bones.push_back(child_name);
				}
			}
			pose.child_bones.sort_custom<SortStringName>();

			pose.position = trans.origin;
			pose.rotation = trans.basis.get_rotation_quaternion();
			pose.bone_index = root_bone;
			p_config.virtual_pose[bone_name] = pose;

			// 构建所有子骨骼的姿势
			for (int j = 0; j < pose.child_bones.size(); j++) {
				build_virtual_pose(p_config, p_skeleton, trans, p_skeleton->find_bone(pose.child_bones[j]), p_human_bone_label);
			}
			p_config.root_bone.push_back(bone_name);
		}

		// 根据骨骼的高度计算虚拟姿势
		for (int i = 0; i < p_skeleton->get_bone_count(); i++) {
			String bone_name = p_skeleton->get_bone_name(i);
			if (!p_config.virtual_pose.has(bone_name)) {
				continue;
			}
			int parent = p_skeleton->get_bone_parent(i);
			if (parent < 0) {
				continue;
			}
			String parent_bone_name = p_skeleton->get_bone_name(parent);
			if (!p_config.virtual_pose.has(parent_bone_name)) {
				continue;
			}
			BonePose &child_pose = p_config.virtual_pose[bone_name];
			child_pose.position.normalize(); // *= child_pose.scale;
			child_pose.local_pose = Transform3D(child_pose.rotation, child_pose.position);
		}
		// 计算骨骼的世界空间姿势
		for (int i = 0; i < p_config.root_bone.size(); i++) {
			BonePose &pose = p_config.virtual_pose[p_config.root_bone[i]];
			pose.global_pose = Transform3D(Basis(pose.rotation), Vector3(0, 0, 0));
			for (int j = 0; j < pose.child_bones.size(); j++) {
				BonePose &child_pose = p_config.virtual_pose[pose.child_bones[j]];
				build_virtual_pose_global(p_config, pose.global_pose, child_pose, p_human_bone_label);
				computer_bone_right(p_config, pose, child_pose);
			}
		}
	}
	static void build_virtual_pose_global(HumanBoneConfig &p_config, Transform3D &parent_trans, BonePose &pose, HashMap<String, String> &p_human_bone_label) {
		pose.global_pose = parent_trans * Transform3D(Basis(pose.rotation), pose.position);
		for (int j = 0; j < pose.child_bones.size(); j++) {
			BonePose &child_pose = p_config.virtual_pose[pose.child_bones[j]];
			build_virtual_pose_global(p_config, pose.global_pose, child_pose, p_human_bone_label);
			computer_bone_right(p_config, pose, child_pose);
		}
	}

	static int get_bone_human_index(Skeleton3D *p_skeleton, Dictionary &p_bone_map, const NodePath &path) {
		if (path.get_subname_count() == 1) {
			// 获取骨骼映射
			StringName bone_name = path.get_subname(0);
			if (p_bone_map.has(bone_name)) {
				bone_name = p_bone_map[bone_name];
			}
			return p_skeleton->find_bone(bone_name);
		}
		return -1;
	}

	static Ref<Animation> build_human_animation(Skeleton3D *p_skeleton, HumanBoneConfig &p_config, Ref<Animation> p_animation, Dictionary &p_bone_map, bool position_by_hip = false) {
		int key_count = p_animation->get_length() * 100 + 1;
		Vector3 loc, scale;
		Quaternion rot;
		HumanSkeleton skeleton_config;
		Vector<HashMap<StringName, Vector3>> animation_lookat;
		Vector<HashMap<StringName, float>> animation_roll;

		//  根节点的位置
		Vector<HashMap<StringName, Vector3>> animation_root_position;
		Vector<HashMap<StringName, Vector3>> animation_root_lookat;
		animation_lookat.resize(key_count);
		animation_roll.resize(key_count);
		animation_root_position.resize(key_count);
		animation_root_lookat.resize(key_count);
		Vector<Animation::Track *> tracks = p_animation->get_tracks();

		// 获取非人型骨骼的轨迹
		List<Animation::Track *> other_tracks;
		for (int j = 0; j < tracks.size(); j++) {
			Animation::Track *track = tracks[j];
			if (track->type == Animation::TYPE_POSITION_3D) {
				Animation::PositionTrack *track_cache = static_cast<Animation::PositionTrack *>(track);
				NodePath path = track_cache->path;
				StringName bone_name;
				if (path.get_subname_count() == 1) {
					// 获取骨骼映射
					bone_name = path.get_subname(0);
					if (p_bone_map.has(bone_name)) {
						bone_name = p_bone_map[bone_name];
					}
				}
				if (bone_name != "Root" && !p_config.virtual_pose.has(bone_name)) {
					other_tracks.push_back(track);
					continue;
				}
			} else if (track->type == Animation::TYPE_ROTATION_3D) {
				Animation::RotationTrack *track_cache = static_cast<Animation::RotationTrack *>(track);

				NodePath path = track_cache->path;
				StringName bone_name;
				if (path.get_subname_count() == 1) {
					// 获取骨骼映射
					bone_name = path.get_subname(0);
					if (p_bone_map.has(bone_name)) {
						bone_name = p_bone_map[bone_name];
					}
				}
				if (bone_name != "Root" && !p_config.virtual_pose.has(bone_name)) {
					other_tracks.push_back(track);
					continue;
				}
			} else if (track->type == Animation::TYPE_SCALE_3D) {
				Animation::ScaleTrack *track_cache = static_cast<Animation::ScaleTrack *>(track);

				NodePath path = track_cache->path;
				StringName bone_name;
				if (path.get_subname_count() == 1) {
					// 获取骨骼映射
					bone_name = path.get_subname(0);
					if (p_bone_map.has(bone_name)) {
						bone_name = p_bone_map[bone_name];
					}
				}
				if (bone_name != "Root" && !p_config.virtual_pose.has(bone_name)) {
					other_tracks.push_back(track);
					continue;
				}
			} else {
				other_tracks.push_back(track);
			}
		}

		for (int i = 0; i < key_count; i++) {
			double time = double(i) / 100.0;
			for (int j = 0; j < tracks.size(); j++) {
				Animation::Track *track = tracks[j];
				if (track->type == Animation::TYPE_POSITION_3D) {
					Animation::PositionTrack *track_cache = static_cast<Animation::PositionTrack *>(track);
					int bone_index = get_bone_human_index(p_skeleton, p_bone_map, track_cache->path);
					if (bone_index < 0) {
						continue;
					}
					p_animation->try_position_track_interpolate(j, time, &loc);
					p_skeleton->set_bone_pose_position(bone_index, loc);
				} else if (track->type == Animation::TYPE_ROTATION_3D) {
					Animation::RotationTrack *track_cache = static_cast<Animation::RotationTrack *>(track);
					int bone_index = get_bone_human_index(p_skeleton, p_bone_map, track_cache->path);
					if (bone_index < 0) {
						continue;
					}
					p_animation->try_rotation_track_interpolate(j, time, &rot);
					p_skeleton->set_bone_pose_rotation(bone_index, rot);
				} else if (track->type == Animation::TYPE_SCALE_3D) {
					Animation::ScaleTrack *track_cache = static_cast<Animation::ScaleTrack *>(track);
					int bone_index = get_bone_human_index(p_skeleton, p_bone_map, track_cache->path);
					if (bone_index < 0) {
						continue;
					}
					p_animation->try_scale_track_interpolate(j, time, &scale);
					p_skeleton->set_bone_pose_scale(bone_index, scale);
				}
			}
			// 转换骨骼姿势到动画
			build_skeleton_pose(p_skeleton, p_config, skeleton_config);
			// 存储动画
			animation_lookat.set(i, skeleton_config.bone_global_lookat);
			animation_roll.set(i, skeleton_config.bone_global_roll);
			animation_root_position.set(i, skeleton_config.root_position);
			animation_root_lookat.set(i, skeleton_config.root_lookat);
		}

		Ref<Animation> out_anim;
		out_anim.instantiate();
		out_anim->set_is_human_animation(true);
		if (animation_lookat.size() > 0) {
			auto &keys = animation_lookat[0];

			for (auto &it : keys) {
				int track_index = out_anim->add_track(Animation::TYPE_POSITION_3D);
				int value_track_index = out_anim->add_track(Animation::TYPE_VALUE);
				Animation::PositionTrack *track = static_cast<Animation::PositionTrack *>(out_anim->get_track(track_index));
				track->path = String("hm.a.") + it.key;
				track->interpolation = Animation::INTERPOLATION_LINEAR;
				track->positions.resize(animation_lookat.size());

				Animation::ValueTrack *value_track = static_cast<Animation::ValueTrack *>(out_anim->get_track(value_track_index));
				value_track->path = String("hm.r.") + it.key;
				value_track->interpolation = Animation::INTERPOLATION_LINEAR;
				value_track->values.resize(animation_lookat.size());

				for (int i = 0; i < animation_lookat.size(); i++) {
					double time = double(i) / 100.0;
					Animation::TKey<Vector3> key;
					key.time = time;
					key.value = animation_lookat[i][it.key];
					track->positions.set(i, key);

					Animation::TKey<Variant> value_key;
					value_key.time = time;
					value_key.value = animation_roll[i][it.key];
					value_track->values.set(i, value_key);
				}
			}

			auto &root_keys = animation_root_position[0];
			for (auto &it : root_keys) {
				int track_index = out_anim->add_track(Animation::TYPE_POSITION_3D);
				Animation::PositionTrack *track = static_cast<Animation::PositionTrack *>(out_anim->get_track(track_index));
				track->path = String("hm.p.") + it.key;
				track->interpolation = Animation::INTERPOLATION_LINEAR;
				track->positions.resize(animation_root_position.size());
				for (int i = 0; i < animation_root_position.size(); i++) {
					double time = double(i) / 100.0;
					Animation::TKey<Vector3> key;
					key.time = time;
					key.value = animation_root_position[i][it.key];
					track->positions.set(i, key);
				}
			}
			// 根节点的朝向
			auto &root_look_keys = animation_root_lookat[0];
			for (auto &it : root_look_keys) {
				int track_index = out_anim->add_track(Animation::TYPE_POSITION_3D);
				Animation::PositionTrack *track = static_cast<Animation::PositionTrack *>(out_anim->get_track(track_index));
				track->path = String("hm.v.") + it.key;
				track->interpolation = Animation::INTERPOLATION_LINEAR;
				track->positions.resize(animation_root_lookat.size());
				for (int i = 0; i < animation_root_lookat.size(); i++) {
					double time = double(i) / 100.0;
					Animation::TKey<Vector3> key;
					key.time = time;
					key.value = animation_root_lookat[i][it.key];
					track->positions.set(i, key);
				}
			}
		}
		// 拷贝轨迹
		for (auto &it : other_tracks) {
			out_anim->add_track_ins(it->duplicate());
		}

		//
		return out_anim;
	}

	// 重定向根骨骼朝向
	static void retarget_root_motion(HumanBoneConfig &p_config, HumanSkeleton &p_skeleton_config) {
		for (auto &it : p_config.root_bone) {
			BonePose &pose = p_config.virtual_pose[it];
			Transform3D &trans = p_skeleton_config.real_global_pose[it];
			Transform3D local_trans;
			local_trans.basis = trans.basis;

			Transform3D child_trans = trans * p_skeleton_config.real_global_pose[pose.child_bones[0]];
			Vector3 forward = (child_trans.origin - trans.origin).normalized();

			trans.basis.rotate_to_align(forward, p_skeleton_config.bone_global_lookat[it] - trans.origin);
			p_skeleton_config.real_local_pose[it] = trans.basis.get_rotation_quaternion();

			p_skeleton_config.root_global_rotation[it] = (local_trans.basis.inverse() * trans.basis).get_rotation_quaternion();
		}
	}

	// 重定向骨骼
	static void retarget(HumanBoneConfig &p_config, HumanSkeleton &p_skeleton_config) {
		RetargetTemp temp;
		for (auto &it : p_config.root_bone) {
			BonePose &pose = p_config.virtual_pose[it];
			Transform3D &trans = p_skeleton_config.real_global_pose[it];
			Transform3D local_trans;
			local_trans.basis = trans.basis;

			retarget(p_config, pose, local_trans, p_skeleton_config, temp);
		}
	}

	static const HashMap<String, String> &get_bone_label() {
		static HashMap<String, String> label_map = {
			{ "Hips", L"臀部" },

			{ "LeftUpperLeg", L"左上腿" },
			{ "RightUpperLeg", L"右上腿" },

			{ "LeftLowerLeg", L"左下腿" },
			{ "RightLowerLeg", L"右下腿" },

			{ "LeftFoot", L"左脚" },
			{ "RightFoot", L"右脚" },

			{ "Spine", L"脊柱" },
			{ "Chest", L"颈部" },
			{ "UpperChest", L"上胸部" },
			{ "Neck", L"颈部" },
			{ "Head", L"头部" },

			{ "LeftShoulder", L"左肩" },
			{ "RightShoulder", L"右肩" },

			{ "LeftUpperArm", L"左上臂" },
			{ "RightUpperArm", L"右上臂" },

			{ "LeftLowerArm", L"左下臂" },
			{ "RightLowerArm", L"右下臂" },

			{ "LeftHand", L"左手" },
			{ "RightHand", L"右手" },

			{ "LeftToes", L"左足" },
			{ "RightToes", L"右足" },

			{ "LeftEye", L"左眼" },
			{ "RightEye", L"右眼" },

			{ "Jaw", L"下巴" },

			{ "LeftThumbMetacarpal", L"左拇指" },
			{ "LeftThumbProximal", L"左拇指近端" },
			{ "LeftThumbDistal", L"左拇指远端" },

			{ "LeftIndexProximal", L"左食指近端" },
			{ "LeftIndexIntermediate", L"左食指中间" },
			{ "LeftIndexDistal", L"左食指远端" },

			{ "LeftMiddleProximal", L"左中指近端" },
			{ "LeftMiddleIntermediate", L"左中指中间" },
			{ "LeftMiddleDistal", L"左中指远端" },

			{ "LeftRingProximal", L"左无名指近端" },
			{ "LeftRingIntermediate", L"左无名指中间" },
			{ "LeftRingDistal", L"左无名指远端" },

			{ "LeftLittleProximal", L"左小拇指近端" },
			{ "LeftLittleIntermediate", L"左小拇指中间" },
			{ "LeftLittleDistal", L"左小拇指远端" },

			{ "RightThumbMetacarpal", L"右拇指" },
			{ "RightThumbProximal", L"右拇指近端" },
			{ "RightThumbDistal", L"右拇指远端" },

			{ "RightIndexProximal", L"右食指近端" },
			{ "RightIndexIntermediate", L"右食指中间" },
			{ "RightIndexDistal", L"右食指远端" },

			{ "RightMiddleProximal", L"右中指近端" },
			{ "RightMiddleIntermediate", L"右中指中间" },
			{ "RightMiddleDistal", L"右中指远端" },

			{ "RightRingProximal", L"右无名指近端" },
			{ "RightRingIntermediate", L"右无名指中间" },
			{ "RightRingDistal", L"右无名指远端" },

			{ "RightLittleProximal", L"右小拇指近端" },
			{ "RightLittleIntermediate", L"右小拇指中间" },
			{ "RightLittleDistal", L"右小拇指远端" },

		};
		return label_map;
	}

	// 构建真实姿势
	static void build_skeleton_pose(Skeleton3D *p_skeleton, HumanBoneConfig &p_config, HumanSkeleton &p_skeleton_config, bool position_by_hip = false) {
		p_skeleton->_update_bones_nested_set();
		p_skeleton->force_update_all_bone_transforms(false);

		Vector<StringName> root_bone = p_config.root_bone;
		Transform3D local_trans;
		for (auto &it : root_bone) {
			BonePose &pose = p_config.virtual_pose[it];
			Transform3D &trans = p_skeleton_config.real_global_pose[it];
			trans = p_skeleton->get_bone_global_pose(pose.bone_index);

			local_trans.basis = Basis(pose.rotation);
			build_skeleton_local_pose(p_skeleton, p_config, pose, local_trans, p_skeleton_config);
		}

		for (auto &it : root_bone) {
			Vector3 bone_foreard = Vector3(0, 0, 1);
			BonePose &pose = p_config.virtual_pose[it];
			{
				Transform3D &trans = p_skeleton_config.real_global_pose[it];

				local_trans.basis = trans.basis;
				p_skeleton_config.root_lookat[it] = local_trans.basis.xform(bone_foreard).normalized();
				p_skeleton_config.root_position[it] = (trans.origin - pose.position);
				p_skeleton_config.bone_global_rotation[it] = trans.basis.get_rotation_quaternion();
			}
			// 臀部的朝向计算到全身的旋转
			build_skeleton_global_lookat(p_config, pose, local_trans, p_skeleton_config);
		}
	}

private:
	struct SortStringName {
		bool operator()(const StringName &l, const StringName &r) const {
			return l.str() > r.str();
		}
	};

	static void build_virtual_pose(HumanBoneConfig &p_config, Skeleton3D *p_skeleton, Transform3D parent_trans, int bone_index, HashMap<String, String> &p_human_bone_label) {
		//Vector<int> child_bones = p_skeleton->get_bone_children(bone_index);
		//for(int i=0; i < child_bones.size(); i++)
		{
			String bone_name = p_skeleton->get_bone_name(bone_index);
			BonePose &pose = p_config.virtual_pose[bone_name];
			Transform3D trans = p_skeleton->get_bone_global_pose(bone_index);
			Vector<int> children = p_skeleton->get_bone_children(bone_index);

			for (int j = 0; j < children.size(); j++) {
				bone_name = p_skeleton->get_bone_name(children[j]);
				if (!p_human_bone_label.has(bone_name)) {
					return;
				}
				pose.child_bones.push_back(bone_name);
			}
			pose.child_bones.sort_custom<SortStringName>();

			Transform3D local_trans = p_skeleton->get_bone_pose(bone_index);
			pose.position = local_trans.origin;
			pose.rotation = local_trans.basis.get_rotation_quaternion();
			pose.bone_index = bone_index;
			for (int j = 0; j < pose.child_bones.size(); j++) {
				build_virtual_pose(p_config, p_skeleton, trans, p_skeleton->find_bone(pose.child_bones[j]), p_human_bone_label);
			}
		}
	}
	static void build_skeleton_local_pose(Skeleton3D *p_skeleton, HumanBoneConfig &p_config, BonePose &parent_pose, Transform3D &parent_trans, HumanSkeleton &p_skeleton_config) {
		for (auto &it : parent_pose.child_bones) {
			BonePose &pose = p_config.virtual_pose[it];
			Transform3D &trans = p_skeleton_config.real_global_pose[it];

			trans.basis = Basis(p_skeleton->get_bone_pose_rotation(pose.bone_index));
			trans.origin = pose.position;
			trans = parent_trans * trans;

			build_skeleton_local_pose(p_skeleton, p_config, pose, trans, p_skeleton_config);
		}
	}
	static float compute_self_roll(HumanBoneConfig &p_config, Transform3D &parent_pose, BonePose &bone_pose, const Transform3D &curr_global_pose, const Vector3 &lookat, const Vector3 &curr_forward) {
		Transform3D rest_trans = parent_pose * bone_pose.local_pose;

		Vector3 rest_forward;
		if (bone_pose.child_bones.size() == 0) {
			// 貌似末端骨骼没有啥自身轴的旋转,比如手指,脚趾
			rest_forward = rest_trans.origin - parent_pose.origin;
		} else {
			rest_forward = rest_trans.xform(p_config.virtual_pose[bone_pose.child_bones[0]].position) - rest_trans.origin;
		}
		rest_trans.basis.rotate_to_align(rest_forward, curr_forward);

		Vector3 rest_right = rest_trans.xform(bone_pose.right) - rest_trans.origin;

		Vector3 curr_right = curr_global_pose.xform(bone_pose.right) - rest_trans.origin;

		if (curr_right.dot(rest_right) > 0.999) {
			return 0;
		}
		Plane plane = Plane(curr_forward, 0.0);
		Vector3 intersect;
		plane.intersects_ray(curr_right + curr_forward, -curr_forward, &intersect);

		plane.intersects_ray(rest_right + curr_forward, -curr_forward, &rest_right);

		if (intersect.x + intersect.y + intersect.z == 0) {
			return 0;
		}
		intersect.normalize();
		curr_forward.dot(rest_right);
		curr_forward.dot(curr_right);
		curr_forward.dot(intersect);

		return rest_right.signed_angle_to(intersect, curr_forward);
	}
	static void build_skeleton_global_lookat(HumanBoneConfig &p_config, BonePose &bone_pose, Transform3D &parent_pose, HumanSkeleton &p_skeleton_config) {
		for (auto &it : bone_pose.child_bones) {
			BonePose &child_pose = p_config.virtual_pose[it];
			Transform3D &real_global_pose = p_skeleton_config.real_global_pose[it];
			Vector3 forward;
			if (child_pose.child_bones.size() > 0) {
				Transform3D &child_trans = p_skeleton_config.real_global_pose[child_pose.child_bones[0]];
				forward = child_trans.origin - real_global_pose.origin;
			} else {
				forward = real_global_pose.origin - parent_pose.origin;
			}
			p_skeleton_config.bone_global_lookat[it] = real_global_pose.origin + forward.normalized();
			p_skeleton_config.bone_global_rotation[it] = real_global_pose.basis.get_rotation_quaternion();
			float roll = compute_self_roll(p_config, parent_pose, child_pose, real_global_pose, p_skeleton_config.bone_global_lookat[it], forward);
			p_skeleton_config.bone_global_roll[it] = roll;
			build_skeleton_global_lookat(p_config, child_pose, real_global_pose, p_skeleton_config);
		}
	}
	struct RetargetTemp {
		Vector3 forward;
		Vector3 curr_forward;
		Transform3D child_trans;
		Basis local_trans;
		Basis roll_trans;
	};
	static void retarget(HumanBoneConfig &p_config, BonePose &p_pose, Transform3D &parent_trans, HumanSkeleton &p_skeleton_config, RetargetTemp &temp) {
		// 重定向骨骼的世界坐标
		for (auto &it : p_pose.child_bones) {
			BonePose &pose = p_config.virtual_pose[it];
			Transform3D &real_global_pose = p_skeleton_config.real_global_pose[it];
			real_global_pose = parent_trans * real_global_pose;
			if (pose.child_bones.size() > 0) {
				temp.child_trans = real_global_pose * p_skeleton_config.real_global_pose[pose.child_bones[0]];
				temp.forward = temp.child_trans.origin - real_global_pose.origin;
			} else {
				temp.forward = real_global_pose.origin - parent_trans.origin;
			}
			Vector3 *lookat = p_skeleton_config.bone_global_lookat.getptr(it);
			if (lookat != nullptr) {
				temp.curr_forward = (*lookat) - real_global_pose.origin;
				real_global_pose.basis.rotate_to_align(temp.forward, temp.curr_forward);
				float roll = p_skeleton_config.bone_global_roll.get(it, 0.0);
				if (roll != 0) {
					temp.curr_forward.normalize();
					temp.roll_trans.set_axis_angle(temp.curr_forward, roll);
					temp.roll_trans.xform(real_global_pose.basis, real_global_pose.basis);
				}
				temp.local_trans = parent_trans.basis.inverse() * real_global_pose.basis;
				p_skeleton_config.real_local_pose[it] = temp.local_trans.get_rotation_quaternion();
			} else {
				p_skeleton_config.real_local_pose[it] = pose.rotation;
			}
			retarget(p_config, pose, real_global_pose, p_skeleton_config, temp);
		}
	}
};

} //namespace HumanAnim
