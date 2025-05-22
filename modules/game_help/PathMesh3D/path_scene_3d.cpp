#include "path_scene_3d.hpp"

void PathScene3D::set_scene(const Ref<PackedScene> &p_scene) {
	if (p_scene != scene) {
		if (scene.is_valid() && scene->is_connected("changed", callable_mp(this, &PathScene3D::_on_scene_changed))) {
			scene->disconnect("changed", callable_mp(this, &PathScene3D::_on_scene_changed));
		}
		scene = p_scene;
		if (scene.is_valid()) {
			scene->connect("changed", callable_mp(this, &PathScene3D::_on_scene_changed));
			_on_scene_changed();
		}
	}
}

Ref<PackedScene> PathScene3D::get_scene() const {
	return scene;
}

void PathScene3D::set_path_3d(Path3D *p_path) {
	if (p_path != path3d) {
		if (path3d != nullptr && path3d->is_connected("curve_changed", callable_mp(this, &PathScene3D::_on_curve_changed))) {
			path3d->disconnect("curve_changed", callable_mp(this, &PathScene3D::_on_curve_changed));
		}
		path3d = p_path;
		if (path3d != nullptr) {
			path3d->connect("curve_changed", callable_mp(this, &PathScene3D::_on_curve_changed));
		}
		_on_curve_changed();
	}
}

Path3D *PathScene3D::get_path_3d() const {
	return path3d;
}

void PathScene3D::set_scene_transform(const SceneTransform p_transform) {
	if (scene_transform != p_transform) {
		scene_transform = p_transform;
		queue_rebuild();
	}
}

PathScene3D::SceneTransform PathScene3D::get_scene_transform() const {
	return scene_transform;
}

void PathScene3D::set_distribution(Distribution p_distribution) {
	if (distribution != p_distribution) {
		distribution = p_distribution;
		queue_rebuild();
		notify_property_list_changed();
	}
}

PathScene3D::Distribution PathScene3D::get_distribution() const {
	return distribution;
}

void PathScene3D::set_alignment(Alignment p_alignment) {
	if (alignment != p_alignment) {
		alignment = p_alignment;
		queue_rebuild();
	}
}

PathScene3D::Alignment PathScene3D::get_alignment() const {
	return alignment;
}

void PathScene3D::set_count(uint64_t p_count) {
	if (count != p_count) {
		count = p_count;
		queue_rebuild();
	}
}

uint64_t PathScene3D::get_count() const {
	return count;
}

void PathScene3D::set_distance(double p_distance) {
	if (distance != p_distance) {
		distance = p_distance;
		queue_rebuild();
	}
}

double PathScene3D::get_distance() const {
	return distance;
}

void PathScene3D::set_rotation_mode(Rotation p_rotation_mode) {
	if (rotation_mode != p_rotation_mode) {
		rotation_mode = p_rotation_mode;
		queue_rebuild();
		notify_property_list_changed();
	}
}

PathScene3D::Rotation PathScene3D::get_rotation_mode() const {
	return rotation_mode;
}

void PathScene3D::set_rotation(const Vector3 &p_rotation) {
	if (rotation != p_rotation) {
		rotation = p_rotation;
		queue_rebuild();
	}
}

Vector3 PathScene3D::get_rotation() const {
	return rotation;
}

void PathScene3D::set_sample_cubic(bool p_cubic) {
	if (sample_cubic != p_cubic) {
		sample_cubic = p_cubic;
		queue_rebuild();
	}
}

bool PathScene3D::get_sample_cubic() const {
	return sample_cubic;
}

void PathScene3D::queue_rebuild() {
	dirty = true;
}

TypedArray<Node3D> PathScene3D::bake_instances() {
	TypedArray<Node3D> out;
	for (Node3D *instance : instances) {
		remove_child(instance);
		instance->set_name("");
		out.push_back(instance);
	}
	instances.clear();

	queue_rebuild();

	return out;
}

void PathScene3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("queue_rebuild"), &PathScene3D::queue_rebuild);
	ClassDB::bind_method(D_METHOD("bake_instances"), &PathScene3D::bake_instances);

	ClassDB::bind_method(D_METHOD("set_scene", "scene"), &PathScene3D::set_scene);
	ClassDB::bind_method(D_METHOD("get_scene"), &PathScene3D::get_scene);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_scene", "get_scene");

	ClassDB::bind_method(D_METHOD("set_path_3d", "path"), &PathScene3D::set_path_3d);
	ClassDB::bind_method(D_METHOD("get_path_3d"), &PathScene3D::get_path_3d);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "path_3d", PROPERTY_HINT_NODE_TYPE, "Path3D"), "set_path_3d", "get_path_3d");

	ClassDB::bind_method(D_METHOD("set_scene_transform", "transform"), &PathScene3D::set_scene_transform);
	ClassDB::bind_method(D_METHOD("get_scene_transform"), &PathScene3D::get_scene_transform);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "scene_transform", PROPERTY_HINT_ENUM, "Transform Scene Local,Transform Scene to Path Node"), "set_scene_transform", "get_scene_transform");

	ClassDB::bind_method(D_METHOD("set_distribution", "distribution"), &PathScene3D::set_distribution);
	ClassDB::bind_method(D_METHOD("get_distribution"), &PathScene3D::get_distribution);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "distribution", PROPERTY_HINT_ENUM, "By Count,By Distance"), "set_distribution", "get_distribution");

	ClassDB::bind_method(D_METHOD("set_alignment", "alignment"), &PathScene3D::set_alignment);
	ClassDB::bind_method(D_METHOD("get_alignment"), &PathScene3D::get_alignment);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment", PROPERTY_HINT_ENUM, "From Start,Centered,From End"), "set_alignment", "get_alignment");

	ClassDB::bind_method(D_METHOD("set_count", "count"), &PathScene3D::set_count);
	ClassDB::bind_method(D_METHOD("get_count"), &PathScene3D::get_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "count", PROPERTY_HINT_RANGE, "0,1000000,1"), "set_count", "get_count");

	ClassDB::bind_method(D_METHOD("set_distance", "distance"), &PathScene3D::set_distance);
	ClassDB::bind_method(D_METHOD("get_distance"), &PathScene3D::get_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance", PROPERTY_HINT_RANGE, "0.01,1000000.0,0.01,or_greater"), "set_distance", "get_distance");

	ClassDB::bind_method(D_METHOD("set_rotation_mode", "rotation_mode"), &PathScene3D::set_rotation_mode);
	ClassDB::bind_method(D_METHOD("get_rotation_mode"), &PathScene3D::get_rotation_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "rotation_mode", PROPERTY_HINT_ENUM, "Fixed,Path,Random"), "set_rotation_mode", "get_rotation_mode");

	ClassDB::bind_method(D_METHOD("set_mesh_rotation", "rotation"), &PathScene3D::set_rotation);
	ClassDB::bind_method(D_METHOD("get_mesh_rotation"), &PathScene3D::get_rotation);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "rotation", PROPERTY_HINT_RANGE, "0.0,360.0,0.01,radians_as_degrees"), "set_mesh_rotation", "get_mesh_rotation");

	ClassDB::bind_method(D_METHOD("set_sample_cubic", "sample_cubic"), &PathScene3D::set_sample_cubic);
	ClassDB::bind_method(D_METHOD("get_sample_cubic"), &PathScene3D::get_sample_cubic);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sample_cubic"), "set_sample_cubic", "get_sample_cubic");

	ADD_SIGNAL(MethodInfo("scene_changed"));
	ADD_SIGNAL(MethodInfo("curve_changed"));

	BIND_ENUM_CONSTANT(TRANSFORM_SCENE_LOCAL);
	BIND_ENUM_CONSTANT(TRANSFORM_SCENE_PATH_NODE);
	BIND_ENUM_CONSTANT(DISTRIBUTE_BY_COUNT);
	BIND_ENUM_CONSTANT(DISTRIBUTE_BY_DISTANCE);
	BIND_ENUM_CONSTANT(DISTRIBUTE_MAX);
	BIND_ENUM_CONSTANT(ROTATE_FIXED);
	BIND_ENUM_CONSTANT(ROTATE_PATH);
	BIND_ENUM_CONSTANT(ROTATE_RANDOM);
	BIND_ENUM_CONSTANT(ROTATE_MAX);
	BIND_ENUM_CONSTANT(ALIGN_FROM_START);
	BIND_ENUM_CONSTANT(ALIGN_CENTERED);
	BIND_ENUM_CONSTANT(ALIGN_FROM_END);
	BIND_ENUM_CONSTANT(ALIGN_MAX);
}

void PathScene3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_process_internal(true);
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			bool local_transform_dirty = local_transform != get_global_transform();
			bool path_transform_dirty = path3d != nullptr && path3d->get_global_transform() != path_transform;
			bool transform_dirty = scene_transform == TRANSFORM_SCENE_PATH_NODE && (local_transform_dirty || path_transform_dirty);

			dirty |= transform_dirty;

			if (dirty) {
				_rebuild_scene();
			}
		} break;
	}
}

PackedStringArray PathScene3D::get_configuration_warnings() const {
	PackedStringArray warnings;
	if (scene.is_valid()) {
		if (!scene->can_instantiate()) {
			warnings.push_back("Cannot instantiate PackedScene.");
		} else {
			Node *tmp = scene->instantiate();
			if (Object::cast_to<Node3D>(tmp) == nullptr) {
				warnings.push_back("Cannot instantiate PackedScene as Node3D.");
			}
			tmp->queue_free();
		}
	}

	return warnings;
}

void PathScene3D::_validate_property(PropertyInfo &property) const {
	if (property.name == StringName("count") && distribution != DISTRIBUTE_BY_COUNT) {
		property.usage = PROPERTY_USAGE_STORAGE;
	} else if (property.name == StringName("rotation") && rotation_mode == ROTATE_RANDOM) {
		property.usage = PROPERTY_USAGE_STORAGE;
	} else if (property.name == StringName("alignment") && distribution == DISTRIBUTE_BY_COUNT) {
		property.usage = PROPERTY_USAGE_STORAGE;
	} else if (property.name == StringName("distance") && distribution != DISTRIBUTE_BY_DISTANCE) {
		property.usage = PROPERTY_USAGE_STORAGE;
	}
}

void PathScene3D::_on_scene_changed() {
	update_configuration_warnings();
	queue_rebuild();
	emit_signal("scene_changed");
}

void PathScene3D::_on_curve_changed() {
	queue_rebuild();
	emit_signal("curve_changed");
}

void PathScene3D::_rebuild_scene() {
	if (path3d == nullptr || path3d->get_curve().is_null() || !path3d->is_inside_tree() || scene.is_null() || !dirty) {
		return;
	}

	dirty = false;

	for (Node3D *child : instances) {
		if (child != nullptr) {
			remove_child(child);
			child->queue_free();
		}
	}
	instances.clear();

	if (!scene->can_instantiate()) {
		return;
	}
	Node *tmp = scene->instantiate();
	if (Object::cast_to<Node3D>(tmp) == nullptr) {
		return;
	}
	tmp->queue_free();

	local_transform = get_global_transform();
	path_transform = path3d->get_global_transform();
	Transform3D mod_transform = local_transform.affine_inverse() * path_transform;

	Ref<Curve3D> curve = path3d->get_curve();
	if (curve->get_point_count() < 2) {
		return;
	}

	double baked_l = curve->get_baked_length();
	if (baked_l == 0.0) {
		return;
	}

	uint64_t n_instances = 0;
	double separation = 0.0;
	switch (distribution) {
		case DISTRIBUTE_BY_COUNT: {
			n_instances = count;
			separation = baked_l / (count - 1);
		} break;
		case DISTRIBUTE_BY_DISTANCE: {
			separation = distance;
			n_instances = Math::floor(baked_l / separation) + 1;
		} break;
		default:
			ERR_FAIL();
	}

	double offset = 0.0;
	if (distribution != DISTRIBUTE_BY_COUNT) {
		switch (alignment) {
			case ALIGN_FROM_START: {
				offset = 0.0;
			} break;
			case ALIGN_CENTERED: {
				offset = (baked_l - separation * (n_instances - 1)) * 0.5;
			} break;
			case ALIGN_FROM_END: {
				offset = baked_l - separation * (n_instances - 1);
			} break;
			default:
				ERR_FAIL();
		}
	}

	instances.resize(n_instances);
	for (uint64_t i = 0; i < n_instances; ++i) {
		Transform3D transform;
		switch (rotation_mode) {
			case ROTATE_FIXED: {
				transform.origin = curve->sample_baked(offset, sample_cubic);
				transform.basis = Basis::from_euler(rotation);
			} break;
			case ROTATE_PATH: {
				transform = curve->sample_baked_with_rotation(offset, sample_cubic, true);
				transform.basis.rotate(rotation);
			} break;
			case ROTATE_RANDOM: {
				transform = curve->sample_baked_with_rotation(offset, sample_cubic, true);
				transform.basis.rotate(Vector3(0.0, 1.0, 0.0), Math::random(0.0, Math::TAU));
			} break;
			default:
				ERR_FAIL();
		}

		if (scene_transform == TRANSFORM_SCENE_PATH_NODE) {
			transform = mod_transform * transform;
		}

		Node3D *instance = Object::cast_to<Node3D>(scene->instantiate());
		if (instance == nullptr) {
			continue;
		}
		add_child(instance, false, InternalMode::INTERNAL_MODE_BACK);
		instance->set_transform(transform);
		instances.write[i] = instance;

		offset += separation;
	}
}

PathScene3D::~PathScene3D() {
	if (scene.is_valid()) {
		if (scene->is_connected("changed", callable_mp(this, &PathScene3D::_on_scene_changed))) {
			scene->disconnect("changed", callable_mp(this, &PathScene3D::_on_scene_changed));
		}
		scene.unref();
	}

	if (path3d != nullptr) {
		if (ObjectDB::get_instance(path3d->get_instance_id()) &&
				path3d->is_connected("curve_changed", callable_mp(this, &PathScene3D::_on_curve_changed))) {
			path3d->disconnect("curve_changed", callable_mp(this, &PathScene3D::_on_curve_changed));
		}
		path3d = nullptr;
	}

	instances.clear();
}
