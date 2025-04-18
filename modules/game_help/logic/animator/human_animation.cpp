//#include "human_animation.h"
//
//#pragma once
//
//#include "scene/resources/animation.h"
//#include "scene/3d/skeleton_3d.h"
//
//
//namespace HumanAnim
//{
//	// 骨骼配置
//
//
//    void HumanSkeleton::rest(HumanBoneConfig& p_config) {
//        clear();
//
//		for (auto& it : p_config.virtual_pose) {
//			HumanAnimationBoneResult& result = bone_result[it.key];
//			result.real_local_pose = it.value.local_pose.basis;
//		}
//        for(auto& it : p_config.root_bone) {
//            root_position[it] = Vector3();
//            root_global_rotation[it] = Quaternion();
//            root_global_move_add[it] = Vector3();
//            root_global_rotation_add[it] = Quaternion();
//        }
//
//    }
//
//    void HumanSkeleton::clear() {
//		bone_result.clear();
//    }
//
//    void HumanSkeleton::set_human_lookat(StringName p_bone,const Vector3& p_lookat) {
//
//        HumanAnimationBoneNameMapping * mapping = HumanAnimationBoneNameMapping::get_singleton();
//        StringName name = mapping->get_bone_name(p_bone);
//        if(mapping == nullptr) return;
//        Vector4& lookat = bone_result[name].bone_global_lookat;
//        lookat.x = p_lookat.x;
//        lookat.y = p_lookat.y;
//        lookat.z = p_lookat.z;
//    }
//    void HumanSkeleton::set_human_roll(StringName p_bone, float p_roll) {
//        HumanAnimationBoneNameMapping * mapping = HumanAnimationBoneNameMapping::get_singleton();
//        if(mapping == nullptr) return;
//        StringName name = mapping->get_bone_name(p_bone);
//		Vector4& lookat = bone_result[name].bone_global_lookat;
//        lookat.w = p_roll;
//    }
//    Basis HumanSkeleton::retarget_root_direction(const Vector3& p_start_direction,const Vector3& p_end_direction) {
//        Basis basis;
//        const Vector3 axis = p_start_direction.cross(p_end_direction).normalized();
//        if (axis.length_squared() != 0) {
//            real_t dot = p_start_direction.dot(p_end_direction);
//            dot = CLAMP(dot, -1.0f, 1.0f);
//            const real_t angle_rads = Math::acos(dot);
//            basis = Basis(axis, angle_rads);
//        }
//        return basis;
//
//    }
//
//    Basis HumanSkeleton::compute_lookat_rotation_add(Ref<Animation> p_animation,int track_index , double time_start, double time_end) {
//            Vector3 loc,loc2;
//            Error err = p_animation->try_position_track_interpolate(track_index, time_start, &loc);
//            err = p_animation->try_position_track_interpolate(track_index, time_end, &loc2);
//            return retarget_root_direction(loc,loc2);
//
//    }
//
//    Vector3 HumanSkeleton::compute_lookat_position_add(Ref<Animation> p_animation,int track_index , double time_start, double time_end) {
//            Vector3 loc,loc2;
//            Error err = p_animation->try_position_track_interpolate(track_index, time_start, &loc);
//            err = p_animation->try_position_track_interpolate(track_index, time_end, &loc2);
//            return loc2 - loc;
//
//    }
//
//    void HumanSkeleton::set_root_lookat(Ref<Animation> p_animation,StringName p_bone, int track_index ,double time,double delta) {
//        if(delta == 0) return;
//
//        Basis q;
//
//        double last_time = time - delta;
//
//        if(delta >= 0) {
//            if(last_time < 0) {
//                q = compute_lookat_rotation_add(p_animation,track_index, 0, time) * compute_lookat_rotation_add(p_animation,track_index, p_animation->get_length() + last_time, p_animation->get_length());
//            }
//            else {
//                q = compute_lookat_rotation_add(p_animation,track_index, last_time, time);
//            }
//        } else {
//            if(last_time > p_animation->get_length()) {
//                q = compute_lookat_rotation_add(p_animation,track_index, time, p_animation->get_length()) * compute_lookat_rotation_add(p_animation,track_index, last_time - p_animation->get_length(), 0);
//            }
//            else {
//                q = compute_lookat_rotation_add(p_animation,track_index, last_time , time);
//            }
//
//        }
//        StringName name;
//
//        HumanAnimationBoneNameMapping * mapping = HumanAnimationBoneNameMapping::get_singleton();
//        if(mapping != nullptr) {
//            name = mapping->get_bone_name(p_bone);
//        }
//        else {
//            name = p_bone.substr(5);
//        }
//        root_global_rotation_add[name] = q;
//    //         Vector3 loc;
//    //         p_animation->try_position_track_interpolate(track_index, time, &loc);
//        //root_lookat[name] = loc;
//    }
//    void HumanSkeleton::set_root_lookat_roll(Ref<Animation> p_animation,StringName p_bone, float p_roll) {
//
//    //         Vector3 loc;
//    //         p_animation->try_position_track_interpolate(track_index, time, &loc);
//        //root_lookat[name] = loc;
//    }
//
//    void HumanSkeleton::set_root_position_add(Ref<Animation> p_animation, StringName p_bone, int track_index ,double time,double delta) {
//        if(delta == 0) return;
//
//        Vector3 q;
//
//        double last_time = time - delta;
//
//        if(delta >= 0) {
//            if(last_time < 0) {
//                q = compute_lookat_position_add(p_animation,track_index, 0, time) + compute_lookat_position_add(p_animation,track_index, p_animation->get_length() + last_time, p_animation->get_length());
//            }
//            else {
//                q = compute_lookat_position_add(p_animation,track_index, last_time, time);
//            }
//        } else {
//            if(last_time > p_animation->get_length()) {
//                q = compute_lookat_position_add(p_animation,track_index, time, p_animation->get_length()) + compute_lookat_position_add(p_animation,track_index, last_time - p_animation->get_length(), 0);
//            }
//            else {
//                q = compute_lookat_position_add(p_animation,track_index, last_time , time);
//            }
//
//        }
//        StringName name;
//
//        HumanAnimationBoneNameMapping * mapping = HumanAnimationBoneNameMapping::get_singleton();
//        if(mapping != nullptr) {
//            name = mapping->get_bone_name(p_bone);
//        }
//        else {
//            name = p_bone.substr(5);
//        }
//        root_global_move_add[name] = q;
//    //         Vector3 loc;
//    //         p_animation->try_position_track_interpolate(track_index, time, &loc);
//        //root_position[name] = loc;
//
//    }
//
//    void HumanSkeleton::blend(HumanSkeleton& p_other,float p_weight) {
//        for(auto& it : p_other.bone_result) {
//            if(bone_result.has(it.key)) {
//				HumanAnimationBoneResult& r = bone_result[it.key];
//                Quaternion& q = r.real_local_pose;
//                q = q.slerp(it.value.real_local_pose, p_weight);
//            }
//        }
//        for(auto& it : p_other.root_position) {
//            if(root_global_move_add.has(it.key)) {
//                Vector3& v = root_position[it.key];
//                v = v.lerp(it.value, p_weight);
//            }
//        }
//        for(auto& it : p_other.root_global_rotation) {
//            if(root_global_move_add.has(it.key)) {
//                Basis& v = root_global_rotation[it.key];
//                v = v.lerp(it.value, p_weight);
//            }
//        }
//
//        for(auto& it : p_other.root_global_move_add) {
//            if(root_global_move_add.has(it.key)) {
//                Vector3& v = root_global_move_add[it.key];
//                v = v.lerp(it.value, p_weight);
//            }
//        }
//        for(auto& it : p_other.root_global_rotation_add) {
//            if(root_global_rotation_add.has(it.key)) {
//                Basis& q = root_global_rotation_add[it.key];
//                q = q.slerp(it.value, p_weight);
//            }
//        }
//
//    }
//
//    void HumanSkeleton::apply_root_motion(Node3D* node) {
//
//        Transform3D curr_trans = node->get_transform();
//
//        Transform3D add_trans;
//        if (root_global_rotation_add.size() > 0) {
//            add_trans.basis = root_global_rotation_add.begin()->value;
//        }
//
//        if (root_global_move_add.size() > 0) {
//            add_trans.origin = add_trans.basis.xform(root_global_move_add.begin()->value);
//        }
//
//        node->set_transform(add_trans * curr_trans);
//    }
//    const HashMap<String, float>& HumanSkeleton::get_bone_blend_weight() {
//        static HashMap<String, float> label_map = {
//
//            {"LeftShoulder",0.3f},
//            {"RightShoulder",0.3f}
//
//        };
//        return label_map;
//    }
//
//
//    void HumanSkeleton::apply(Skeleton3D *p_skeleton,const HashMap<String, float>& bone_blend_weight,float p_weight) {
//        for(auto& it : bone_result) {
//            int bone_index = p_skeleton->find_bone(it.key);
//            if (bone_index >= 0) {
//                float weight = 1.0f;
//                if(bone_blend_weight.has(it.key)) {
//                    weight = bone_blend_weight[it.key];
//                }
//				Basis rot = p_skeleton->get_bone_pose_rotation(bone_index).slerp(it.value.real_local_pose, p_weight * weight);
//                p_skeleton->set_bone_pose_rotation(bone_index, rot.get_rotation_quaternion());
//            }
//        }
//    }
//
//    void HumanSkeleton::apply_root_motion(Vector3& p_position,Quaternion& p_rotation,Vector3& p_position_add,Quaternion & p_rotation_add,float p_weight) {
//
//        if(root_global_rotation.size() > 0) {
//            p_rotation = p_rotation.slerp(root_global_rotation.begin()->value.get_rotation_quaternion(),p_weight);
//        }
//
//        if (root_global_rotation_add.size() > 0) {
//            p_rotation_add = p_rotation.slerp(root_global_rotation_add.begin()->value.get_rotation_quaternion(),p_weight);
//        }
//
//        if(root_position.size() > 0) {
//            p_position = p_position.lerp(root_position.begin()->value,p_weight);
//        }
//
//        if(root_global_move_add.size() > 0) {
//            p_position_add = p_position_add.lerp(root_global_move_add.begin()->value,p_weight);
//        }
//    }
//
//
//
//    /********************************************************************************************************************************/
//
//    static void computer_bone_right(HumanBoneConfig& p_config,BonePose& parent_pose,BonePose& pose) {
//
//        Vector3 forward;
//        if(pose.child_bones.size() == 0) {
//            forward = pose.global_pose.origin - parent_pose.global_pose.origin;
//        }
//        else {
//            BonePose& child_pose = p_config.virtual_pose[pose.child_bones[0]];
//            forward = child_pose.global_pose.origin - parent_pose.global_pose.origin;
//        }
//
//        Vector3::Axis min_axis = forward.min_axis_index();
//
//        Vector3 up = Vector3(0, 1, 0);
//        switch (min_axis)
//        {
//        case Vector3::AXIS_X:
//            up = Vector3(1, 0, 0);
//            break;
//        case Vector3::AXIS_Y:
//            up = Vector3(0, 1, 0);
//            break;
//        case Vector3::AXIS_Z:
//            up = Vector3(0, 0, 1);
//            break;
//        }
//        Vector3 right = up.cross(forward.normalized());
//
//        pose.right = pose.global_pose.basis.inverse().xform(right);
//        pose.right.normalize();
//    }
//    // 构建虚拟姿势
//    void HumanAnimmation::build_virtual_pose(Skeleton3D *p_skeleton,HumanBoneConfig& p_config,HashMap<String, String>& p_human_bone_label) {
//        Vector<int> root_bones = p_skeleton->get_root_bones();
//        p_skeleton->_update_bones_nested_set();
//        p_skeleton->force_update_all_bone_transforms(false);
//
//        auto rbs = p_skeleton->get_root_bones();
//
//
//        for(int root_bone = 0; root_bone < rbs.size(); ++root_bone){
//
//            StringName bone_name = p_skeleton->get_bone_name(root_bone);
//            BonePose &pose = p_config.virtual_pose[bone_name];
//            Transform3D trans = p_skeleton->get_bone_global_pose(root_bone);
//            float height = 1.0;
//            Vector<int> children = p_skeleton->get_bone_children(root_bone);
//            for(int j=0;j<children.size();j++) {
//                StringName child_name = p_skeleton->get_bone_name(children[j]);
//                if(p_human_bone_label.has(child_name)) {
//                    pose.child_bones.push_back(child_name);
//                }
//            }
//            pose.child_bones.sort_custom<SortStringName>();
//
//
//            pose.position = trans.origin;
//            pose.rotation = trans.basis.get_rotation_quaternion();
//            pose.bone_index = root_bone;
//
//            // 构建所有子骨骼的姿势
//            for(int j=0;j<pose.child_bones.size();j++) {
//				BonePose& child_pose = p_config.virtual_pose[pose.child_bones[j]];
//                child_pose.bone_index = p_skeleton->find_bone(pose.child_bones[j]);
//                build_virtual_pose(p_skeleton, p_config, child_pose, p_human_bone_label);
//            }
//            p_config.root_bone.push_back(bone_name);
//        }
//
//        // 计算骨骼的世界空间姿势
//        for(int i=0;i<p_config.root_bone.size();i++) {
//            BonePose& pose = p_config.virtual_pose[p_config.root_bone[i]];
//            pose.global_pose = Transform3D(Basis(pose.rotation),Vector3(0,0,0));
//			pose.local_pose = pose.global_pose;
//            for(int j=0;j<pose.child_bones.size();j++) {
//                BonePose& child_pose = p_config.virtual_pose[pose.child_bones[j]];
//				child_pose.global_pose = pose.global_pose * Transform3D(Basis(child_pose.rotation), child_pose.position);
//                build_virtual_pose_global(p_config,child_pose,p_human_bone_label);
//
//                // 計算骨骼的右方向
//                computer_bone_right(p_config,pose,child_pose);
//            }
//        }
//    }
//	void HumanAnimmation::build_virtual_pose(Skeleton3D* p_skeleton, HumanBoneConfig& p_config, BonePose& pose,  HashMap<String, String>& p_human_bone_label) {
//
//		//Vector<int> child_bones = p_skeleton->get_bone_children(bone_index);
//		//for(int i=0; i < child_bones.size(); i++)
//		{
//			float height = 1.0;
//			Vector<int> children = p_skeleton->get_bone_children(pose.bone_index);
//
//			for (int j = 0; j < children.size(); j++) {
//				StringName bone_name = p_skeleton->get_bone_name(children[j]);
//				if (!p_human_bone_label.has(bone_name)) {
//					return;
//				}
//				pose.child_bones.push_back(bone_name);
//			}
//
//
//			Transform3D local_trans = p_skeleton->get_bone_pose(pose.bone_index);
//			pose.rotation = local_trans.basis.get_rotation_quaternion();
//			pose.position = local_trans.origin.normalized();
//			for (int j = 0; j < pose.child_bones.size(); j++) {
//				BonePose& child_pose = p_config.virtual_pose[pose.child_bones[j]];
//				child_pose.bone_index = p_skeleton->find_bone(pose.child_bones[j]);
//				build_virtual_pose(p_skeleton, p_config, child_pose,p_human_bone_label);
//			}
//		}
//
//	}
//
//
//
//	void HumanAnimmation::build_virtual_pose_global(HumanBoneConfig& p_config, BonePose& p_parent_pose, HashMap<String, String>& p_human_bone_label) {
//
//        for(int j=0;j< p_parent_pose.child_bones.size();j++) {
//            BonePose& child_pose = p_config.virtual_pose[p_parent_pose.child_bones[j]];
//			child_pose.local_pose = Transform3D(Basis(child_pose.rotation), child_pose.position);
//			child_pose.global_pose = p_parent_pose.global_pose * child_pose.local_pose;
//
//            build_virtual_pose_global(p_config,child_pose,p_human_bone_label);
//            // 計算骨骼的右方向
//            computer_bone_right(p_config, p_parent_pose,child_pose);
//        }
//
//    }
//
//
//    int HumanAnimmation::get_bone_human_index(Skeleton3D* p_skeleton, Dictionary& p_bone_map,const NodePath& path) {
//        if (path.get_subname_count() == 1) {
//            // 获取骨骼映射
//            StringName bone_name = path.get_subname(0);
//            if (p_bone_map.has(bone_name)) {
//                bone_name = p_bone_map[bone_name];
//            }
//            return p_skeleton->find_bone(bone_name);
//        }
//        return -1;
//
//    }
//
//    Ref<Animation> HumanAnimmation::build_human_animation(Skeleton3D* p_skeleton,HumanBoneConfig& p_config,Ref<Animation> p_animation,Dictionary & p_bone_map, bool position_by_hip ) {
//        int key_count = p_animation->get_length() * 100 + 1;
//        Vector3 loc,scale;
//        Quaternion rot;
//        HumanSkeleton skeleton_config;
//        Vector<HashMap<StringName, HumanAnimationBoneResult>> animation_lookat;
//
//        //  根节点的位置
//        Vector<HashMap<StringName, Vector3>> animation_root_position;
//        Vector<HashMap<StringName, Vector4>> animation_root_lookat;
//        animation_lookat.resize(key_count);
//        animation_root_position.resize(key_count);
//        animation_root_lookat.resize(key_count);
//        Vector<Animation::Track*> tracks = p_animation->get_tracks();
//
//        // 获取非人型骨骼的轨迹
//        List<Animation::Track*> other_tracks;
//        for (int j = 0; j < tracks.size(); j++) {
//            Animation::Track* track = tracks[j];
//            if (track->type == Animation::TYPE_POSITION_3D) {
//                Animation::PositionTrack* track_cache = static_cast<Animation::PositionTrack*>(track);
//                NodePath path = track_cache->path;
//                StringName bone_name;
//                if (path.get_subname_count() == 1) {
//                    // 获取骨骼映射
//                    bone_name = path.get_subname(0);
//                    if (p_bone_map.has(bone_name)) {
//                        bone_name = p_bone_map[bone_name];
//                    }
//                }
//                if (bone_name != "Root" && !p_config.virtual_pose.has(bone_name)) {
//                    other_tracks.push_back(track);
//                    continue;
//                }
//            }
//            else if (track->type == Animation::TYPE_ROTATION_3D) {
//                Animation::RotationTrack* track_cache = static_cast<Animation::RotationTrack*>(track);
//
//                NodePath path = track_cache->path;
//                StringName bone_name;
//                if (path.get_subname_count() == 1) {
//                    // 获取骨骼映射
//                    bone_name = path.get_subname(0);
//                    if (p_bone_map.has(bone_name)) {
//                        bone_name = p_bone_map[bone_name];
//                    }
//                }
//                if (bone_name != "Root" && !p_config.virtual_pose.has(bone_name)) {
//                    other_tracks.push_back(track);
//                    continue;
//                }
//            }
//            else if (track->type == Animation::TYPE_SCALE_3D) {
//                Animation::ScaleTrack* track_cache = static_cast<Animation::ScaleTrack*>(track);
//
//                NodePath path = track_cache->path;
//                StringName bone_name;
//                if (path.get_subname_count() == 1) {
//                    // 获取骨骼映射
//                    bone_name = path.get_subname(0);
//                    if (p_bone_map.has(bone_name)) {
//                        bone_name = p_bone_map[bone_name];
//                    }
//                }
//                if (bone_name != "Root" && !p_config.virtual_pose.has(bone_name)) {
//                    other_tracks.push_back(track);
//                    continue;
//                }
//            }
//            else
//            {
//                other_tracks.push_back(track);
//            }
//        }
//
//
//
//        for(int i = 0; i < key_count; i++) {
//            double time = double(i) / 100.0;
//            for(int j = 0; j < tracks.size(); j++) {
//                Animation::Track* track = tracks[j];
//                if(track->type == Animation::TYPE_POSITION_3D) {
//                    Animation::PositionTrack* track_cache = static_cast<Animation::PositionTrack*>(track);
//                    int bone_index = get_bone_human_index(p_skeleton, p_bone_map,track_cache->path);
//                    if(bone_index < 0) {
//                        continue;
//                    }
//                    Error err = p_animation->try_position_track_interpolate(j, time, &loc);
//                    p_skeleton->set_bone_pose_position(bone_index, loc);
//                }
//                else if(track->type == Animation::TYPE_ROTATION_3D) {
//                    Animation::RotationTrack* track_cache = static_cast<Animation::RotationTrack*>(track);
//                    int bone_index = get_bone_human_index(p_skeleton, p_bone_map, track_cache->path);
//                    if(bone_index < 0) {
//                        continue;
//                    }
//                    Error err = p_animation->try_rotation_track_interpolate(j, time, &rot);
//                    p_skeleton->set_bone_pose_rotation(bone_index, rot);
//                }
//                else if(track->type == Animation::TYPE_SCALE_3D) {
//                    Animation::ScaleTrack* track_cache = static_cast<Animation::ScaleTrack*>(track);
//                    int bone_index = get_bone_human_index(p_skeleton, p_bone_map, track_cache->path);
//                    if(bone_index < 0) {
//                        continue;
//                    }
//                    Error err = p_animation->try_scale_track_interpolate(j, time, &scale);
//                    p_skeleton->set_bone_pose_scale(bone_index, scale);
//                }
//            }
//            // 转换骨骼姿势到动画
//            build_skeleton_pose(p_skeleton,p_config,skeleton_config);
//            // 存储动画
//            animation_lookat.set(i,skeleton_config.bone_result);
//            animation_root_position.set(i,skeleton_config.root_position);
//            animation_root_lookat.set(i,skeleton_config.root_lookat);
//
//        }
//
//        Ref<Animation> out_anim;
//        out_anim.instantiate();
//        out_anim->set_is_human_animation(true);
//        if(animation_lookat.size() > 0) {
//
//            auto& keys = animation_lookat[0];
//
//            for(auto& it : keys) {
//                int track_index = out_anim->add_track(Animation::TYPE_POSITION_3D);
//                int value_track_index = out_anim->add_track(Animation::TYPE_VALUE);
//                Animation::PositionTrack* track = static_cast<Animation::PositionTrack*>(out_anim->get_track(track_index));
//                Animation::ValueTrack* value_track = static_cast<Animation::ValueTrack*>(out_anim->get_track(value_track_index));
//                track->path = String("hm.a.") + it.key;
//                track->interpolation = Animation::INTERPOLATION_LINEAR;
//                track->positions.resize(animation_lookat.size());
//
//
//                value_track->path = String("hm.r.") + it.key;
//                value_track->interpolation = Animation::INTERPOLATION_LINEAR;
//                value_track->values.resize(animation_lookat.size());
//
//                for(int i = 0;i < animation_lookat.size();i++) {
//                    double time = double(i) / 100.0;
//                    Animation::TKey<Vector3> key;
//                    key.time = time;
//                    const Vector4& lookat = animation_lookat[i][it.key].bone_global_lookat;
//                    key.value = Vector3(lookat.x, lookat.y, lookat.z);
//                    track->positions.set(i,key);
//
//
//                    Animation::TKey<Variant> value_key;
//                    value_key.time = time;
//                    value_key.value = lookat.w;
//                    value_track->values.set(i, value_key);
//
//                }
//            }
//
//            auto& root_keys = animation_root_position[0];
//            for(auto& it : root_keys) {
//                int track_index = out_anim->add_track(Animation::TYPE_POSITION_3D);
//                Animation::PositionTrack* track = static_cast<Animation::PositionTrack*>(out_anim->get_track(track_index));
//                track->path = String("hm.p.") + it.key;
//                track->interpolation = Animation::INTERPOLATION_LINEAR;
//                track->positions.resize(animation_root_position.size());
//                for(int i = 0;i < animation_root_position.size();i++) {
//                    double time = double(i) / 100.0;
//                    Animation::TKey<Vector3> key;
//                    key.time = time;
//                    key.value = animation_root_position[i][it.key];
//                    track->positions.set(i,key);
//                }
//            }
//            // 根节点的朝向
//            auto& root_look_keys = animation_root_lookat[0];
//            for(auto& it : root_look_keys) {
//                int track_index = out_anim->add_track(Animation::TYPE_POSITION_3D);
//                int value_track_index = out_anim->add_track(Animation::TYPE_VALUE);
//                Animation::PositionTrack* track = static_cast<Animation::PositionTrack*>(out_anim->get_track(track_index));
//                track->path = String("hm.v.") + it.key;
//                track->interpolation = Animation::INTERPOLATION_LINEAR;
//                track->positions.resize(animation_root_lookat.size());
//
//                Animation::ValueTrack* value_track = static_cast<Animation::ValueTrack*>(out_anim->get_track(value_track_index));
//                value_track->path = String("hm.vr.") + it.key;
//                value_track->interpolation = Animation::INTERPOLATION_LINEAR;
//                value_track->values.resize(animation_root_lookat.size());
//                for(int i = 0;i < animation_root_lookat.size();i++) {
//                    double time = double(i) / 100.0;
//
//                    Vector4 lookat = animation_root_lookat[i][it.key];
//                    Animation::TKey<Vector3> key;
//                    key.time = time;
//                    key.value = Vector3(lookat.x, lookat.y, lookat.z);
//                    track->positions.set(i,key);
//
//
//                    Animation::TKey<Variant> value_key;
//                    value_key.time = time;
//                    value_key.value = lookat.w;
//                    value_track->values.set(i, value_key);
//                }
//            }
//
//        }
//        // 拷贝轨迹
//        for(auto& it : other_tracks) {
//            out_anim->add_track_ins(it->duplicate());
//        }
//
//        //
//        return out_anim;
//
//
//    }
//
//
//    // 重定向根骨骼朝向
//    void HumanAnimmation::retarget_root_motion(HumanBoneConfig& p_config,HumanSkeleton& p_skeleton_config) {
//        for(auto& it : p_config.root_bone) {
//            BonePose& pose = p_config.virtual_pose[it];
//			HumanAnimationBoneResult& result = p_skeleton_config.bone_result[it];
//            Transform3D& trans = result.real_global_pose;
//            Transform3D local_trans;
//            local_trans.basis = trans.basis;
//
//            Transform3D child_trans = trans * p_skeleton_config.bone_result[pose.child_bones[0]].real_global_pose;
//            Vector3 forward = (child_trans.origin - trans.origin).normalized();
//            Vector4& lookat = result.bone_global_lookat;
//            trans.basis.rotate_to_align(forward, Vector3(lookat.x, lookat.y, lookat.z) - trans.origin);
//			result.real_local_pose = trans.basis.get_rotation_quaternion();
//
//
//			p_skeleton_config.root_global_rotation[it] = (local_trans.basis.inverse() * trans.basis).get_rotation_quaternion();
//
//        }
//
//    }
//
//    // 重定向骨骼
//    void HumanAnimmation::retarget(HumanBoneConfig& p_config,HumanSkeleton& p_skeleton_config) {
//		Transform3D local_trans;
//        for(auto& it : p_config.root_bone) {
//            BonePose& pose = p_config.virtual_pose[it];
//            Transform3D& trans = p_skeleton_config.bone_result[it].real_global_pose;
//
//
//			HumanAnimationBoneResult& result = p_skeleton_config.bone_result[it];
//			result.real_local_pose = pose.local_pose.basis;
//			result.real_global_pose = pose.local_pose;
//            retarget(p_config, pose, local_trans,p_skeleton_config);
//
//
//        }
//
//    }
//
//
//    const HashMap<String, String>& HumanAnimmation::get_bone_label() {
//        static HashMap<String, String> label_map = {
//            {"Hips",L"臀部"},
//
//            {"LeftUpperLeg",L"左上腿"},
//            {"RightUpperLeg",L"右上腿"},
//
//            {"LeftLowerLeg",L"左下腿"},
//            {"RightLowerLeg",L"右下腿"},
//
//            {"LeftFoot",L"左脚"},
//            {"RightFoot",L"右脚"},
//
//            {"Spine",L"脊柱"},
//            {"Chest",L"颈部"},
//            {"UpperChest",L"上胸部"},
//            {"Neck",L"颈部"},
//            {"Head",L"头部"},
//
//            {"LeftShoulder",L"左肩"},
//            {"RightShoulder",L"右肩"},
//
//            {"LeftUpperArm",L"左上臂"},
//            {"RightUpperArm",L"右上臂"},
//
//            {"LeftLowerArm",L"左下臂"},
//            {"RightLowerArm",L"右下臂"},
//
//            {"LeftHand",L"左手"},
//            {"RightHand",L"右手"},
//
//            {"LeftToes",L"左足"},
//            {"RightToes",L"右足"},
//
//            {"LeftEye",L"左眼"},
//            {"RightEye",L"右眼"},
//
//            {"Jaw",L"下巴"},
//
//            {"LeftThumbMetacarpal",L"左拇指"},
//            {"LeftThumbProximal",L"左拇指近端"},
//            {"LeftThumbDistal",L"左拇指远端"},
//
//            {"LeftIndexProximal",L"左食指近端"},
//            {"LeftIndexIntermediate",L"左食指中间"},
//            {"LeftIndexDistal",L"左食指远端"},
//
//            {"LeftMiddleProximal",L"左中指近端"},
//            {"LeftMiddleIntermediate",L"左中指中间"},
//            {"LeftMiddleDistal",L"左中指远端"},
//
//            {"LeftRingProximal",L"左无名指近端"},
//            {"LeftRingIntermediate",L"左无名指中间"},
//            {"LeftRingDistal",L"左无名指远端"},
//
//            {"LeftLittleProximal",L"左小拇指近端"},
//            {"LeftLittleIntermediate",L"左小拇指中间"},
//            {"LeftLittleDistal",L"左小拇指远端"},
//
//            {"RightThumbMetacarpal",L"右拇指"},
//            {"RightThumbProximal",L"右拇指近端"},
//            {"RightThumbDistal",L"右拇指远端"},
//
//            {"RightIndexProximal",L"右食指近端"},
//            {"RightIndexIntermediate",L"右食指中间"},
//            {"RightIndexDistal",L"右食指远端"},
//
//            {"RightMiddleProximal",L"右中指近端"},
//            {"RightMiddleIntermediate",L"右中指中间"},
//            {"RightMiddleDistal",L"右中指远端"},
//
//            {"RightRingProximal",L"右无名指近端"},
//            {"RightRingIntermediate",L"右无名指中间"},
//            {"RightRingDistal",L"右无名指远端"},
//
//            {"RightLittleProximal",L"右小拇指近端"},
//            {"RightLittleIntermediate",L"右小拇指中间"},
//            {"RightLittleDistal",L"右小拇指远端"},
//
//        };
//        return label_map;
//    }
//
//    // 构建真实姿势
//    void HumanAnimmation::build_skeleton_pose(Skeleton3D* p_skeleton, HumanBoneConfig& p_config, HumanSkeleton& p_skeleton_config, bool position_by_hip ) {
//        p_skeleton->_update_bones_nested_set();
//        p_skeleton->force_update_all_bone_transforms(false);
//        {
//            //Transform3D local_trans;
//            Transform3D local_trans,zero_trans  ;
//            for (auto& it : p_config.root_bone) {
//                BonePose& pose = p_config.virtual_pose[it];
//                Transform3D& trans = p_skeleton_config.bone_result[it].real_global_pose;
//                trans = p_skeleton->get_bone_global_pose(pose.bone_index);
//				//p_skeleton_config.root_lookat[it] = BonePose::get_root_lookat(Basis(pose.rotation),trans.basis,pose.forward,pose.right);
//				p_skeleton_config.root_position[it] = (trans.origin - pose.position);
//				p_skeleton_config.root_global_rotation[it] = trans.basis.get_rotation_quaternion();
//
//                zero_trans.basis = Basis(pose.rotation);
//                build_skeleton_lookat(p_skeleton, p_config, pose, zero_trans, p_skeleton_config);
//            }
//
//        }
//
//    }
//
//    float compute_self_roll( HumanBoneConfig& p_config, Transform3D& parent_pose,BonePose& bone_pose,const Basis& trans, const Vector3& lookat,const Vector3& right) {
//
//        Transform3D rest_trans = parent_pose * Transform3D(Basis(bone_pose.rotation),bone_pose.position);
//
//        Vector3 rest_forward;
//        if(bone_pose.child_bones.size() == 0) {
//            // 貌似末端骨骼没有啥自身轴的旋转,比如手指,脚趾
//            rest_forward = rest_trans.origin -parent_pose.origin;
//        }else {
//            rest_forward = rest_trans.xform(p_config.virtual_pose[bone_pose.child_bones[0]].position) - rest_trans.origin;
//        }
//        Vector3 curr_forward = lookat - rest_trans.origin;
//        curr_forward.normalized();
//        rest_trans.basis.rotate_to_align(rest_forward, curr_forward);
//
//        Vector3 rest_right = rest_trans.basis.xform(right);
//
//        Transform3D curr_trans = parent_pose * trans;
//        Vector3 curr_right = curr_trans.basis.xform(right);
//
//
//
//		Plane plane = Plane(curr_forward, 0.0);
//		Vector3 intersect;
//		plane.intersects_ray(curr_right - curr_forward,-curr_forward, &intersect);
//
//
//		if(intersect.x + intersect.y + intersect.z == 0)
//		{
//			return 0;
//		}
//
//		return rest_right.signed_angle_to(intersect.normalized(), curr_forward);
//    }
//
//    void HumanAnimmation::build_skeleton_lookat(Skeleton3D* p_skeleton,HumanBoneConfig& p_config, BonePose& bone_pose,Transform3D& parent_pose,HumanSkeleton& p_skeleton_config) {
//
//        for(auto& it : bone_pose.child_bones) {
//            BonePose& child_pose = p_config.virtual_pose[it];
//
//			Basis trans = p_skeleton->get_bone_pose(bone_pose.bone_index).basis;
//
//			HumanAnimationBoneResult& result = p_skeleton_config.bone_result[it];
//            result.real_global_pose = parent_pose * child_pose.local_pose;
//
//			build_skeleton_lookat(p_skeleton,p_config, child_pose, result.real_global_pose, p_skeleton_config);
//
//            Vector3 forward;
//            if(child_pose.child_bones.size() == 0) {
//				forward = child_pose.global_pose.origin - parent_pose.origin;
//            }
//            else {
//				BonePose& first_child_pose = p_config.virtual_pose[child_pose.child_bones[0]];
//				forward = first_child_pose.global_pose.origin - child_pose.global_pose.origin;
//            }
//			forward = child_pose.global_pose.origin + forward.normalized();
//			result.bone_global_lookat.x = forward.x;
//			result.bone_global_lookat.y = forward.y;
//			result.bone_global_lookat.z = forward.z;
//
//            // 计算自身轴旋转
//			result.bone_global_lookat.w = compute_self_roll(p_config, parent_pose, child_pose, trans, Vector3(result.bone_global_lookat.x, result.bone_global_lookat.y, result.bone_global_lookat.z), child_pose.right);
//        }
//
//    }
//
//    static void lookat_to_bone_pose(HumanBoneConfig& p_config,BonePose& pose,Transform3D& parent_trans,HumanAnimationBoneResult& result) {
//        Vector3 lookat = Vector3(result.bone_global_lookat.x, result.bone_global_lookat.y, result.bone_global_lookat.z);
//
//		result.real_global_pose = parent_trans * pose.local_pose;
//        // 计算当前朝向
//        Vector3 curr_forward = lookat - result.real_global_pose.origin;
//		curr_forward.normalize();
//        if(pose.child_bones.size() == 0) {
//            Vector3 rest_forward = result.real_global_pose.origin - parent_trans.origin;
//			result.real_global_pose.basis.rotate_to_align(rest_forward, curr_forward);
//        }
//        else {
//
//			BonePose& first_child_pose = p_config.virtual_pose[pose.child_bones[0]];
//			Transform3D child_trans = result.real_global_pose * first_child_pose.local_pose;
//            Vector3 rest_forward = child_trans.origin - result.real_global_pose.origin;
//			result.real_global_pose.basis.rotate_to_align(rest_forward, curr_forward);
//        }
//        // 计算自身轴旋转
//		if (result.bone_global_lookat.w != 0) {
//			Basis rot = Basis(curr_forward, result.bone_global_lookat.w);
//			result.real_global_pose.basis = rot * result.real_global_pose.basis;
//		}
//        // 计算出本地旋转
//        Transform3D local_trans = parent_trans.inverse() * result.real_global_pose;
//        result.real_local_pose = local_trans.basis;
//    }
//
//    void HumanAnimmation::retarget(HumanBoneConfig& p_config,BonePose& parent_pose,Transform3D& parent_trans,HumanSkeleton& p_skeleton_config) {
//
//        // 重定向骨骼的世界坐标
//        for(auto& it : parent_pose.child_bones) {
//            BonePose& pose = p_config.virtual_pose[it];
//            if (p_skeleton_config.bone_result.has(it)) {
//			    HumanAnimationBoneResult& result = p_skeleton_config.bone_result[it];
//                Vector4& lookat = result.bone_global_lookat;
//				lookat_to_bone_pose(p_config, pose, parent_trans, result);
//                retarget(p_config,pose, result.real_global_pose,p_skeleton_config);
//            }
//            else {
//                Transform3D trans = Transform3D(Basis(pose.rotation),pose.position);
//                Transform3D global_trans = parent_trans * trans;
//                retarget(p_config,pose,global_trans,p_skeleton_config);
//            }
//        }
//
//    }
//
//
//
//}
//
