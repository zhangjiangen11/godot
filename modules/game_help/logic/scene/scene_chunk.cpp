#include "scene_chunk.h"

#include "scene/3d/physics/collision_object_3d.h"

#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/sphere_shape_3d.h"

#include "scene/main/viewport.h"

void SceneDataCompoent::show(int p_show_level, const Transform3D &p_parent, SceneChunkGroupInstance *instance) {
	if (instance == nullptr) {
		return;
	}
	Transform3D trans = p_parent * transform;
	if (resource_type == ResourceType::RT_Mesh) {
		if (!resource_path.is_empty()) {
			instance->_add_mesh_instance(resource_path, trans);
		}
	}
	if (collision_shape_type == CollisionShapeType::SHAPE_NONE) {
		return;
	} else if (collision_shape_type == CollisionShapeType::SHAPE_MESH) {
		if (!collision_mesh_path.is_empty()) {
			instance->_add_mesh_collision_instance(collision_mesh_path, trans);
		}
	} else if (collision_shape_type != CollisionShapeType::SHAPE_SPHERE) {
		instance->_add_collision_instance(trans, collision_shape_type, collosion_box_size, 0, collision_sphere_radius);
	} else if (collision_shape_type == CollisionShapeType::SHAPE_BOX) {
		instance->_add_collision_instance(trans, collision_shape_type, collosion_box_size, 0, 0);
	} else if (collision_shape_type == CollisionShapeType::SHAPE_CAPSULE) {
		instance->_add_collision_instance(trans, collision_shape_type, collosion_box_size, collision_capsule_height, collision_capsule_radius);
	} else if (collision_shape_type == CollisionShapeType::SHAPE_CYLINDER) {
		instance->_add_collision_instance(trans, collision_shape_type, collosion_box_size, collision_cylinder_height, collision_cylinder_radius);
	}
}
/*********************************************************************************************************/
void SceneDataCompoentBlock::show(int p_show_level, const Transform3D &p_parent, SceneChunkGroupInstance *instance) {
	SceneDataCompoent::show(p_show_level, p_parent, instance);
	Transform3D world = p_parent * transform;
	for (uint32_t i = 0; i < compoents.size(); i++) {
		if (p_show_level >= compoents[i].second) {
			compoents[i].first->show(p_show_level, world, instance);
		}
	}
}
/*********************************************************************************************************/
void SceneBlock::show(int p_show_level, const Transform3D &p_parent, SceneChunkGroupInstance *instance) {
	Transform3D trans = p_parent * transform;
	for (uint32_t i = 0; i < blocks.size(); i++) {
		if (p_show_level >= blocks[i].second) {
			blocks[i].first->show(p_show_level, trans, instance);
		}
	}
}
/*********************************************************************************************************/
void SceneResource::show(int lod, int p_show_level, const Transform3D &p_parent, SceneChunkGroupInstance *instance) {
	uint32_t curr_lod = lod;
	if (curr_lod >= scene_lods.size()) {
		curr_lod = scene_lods.size() - 1;
	}
	if (p_show_level >= scene_lods[curr_lod].second) {
		return;
	}
	scene_lods[curr_lod].first->show(p_show_level, p_parent, instance);
}
/*********************************************************************************************************/

void SceneChunk::MeshInstance::update_mesh_instance(RID p_world_3d_scenario) {
	if (!dirty) {
		return;
	}
	if (multimesh.is_null()) {
		Ref<Resource> resource;
		if (load_token.is_valid()) {
			if (ResourceLoader::load_threaded_get_status(load_token->local_path, resource) != ResourceLoader::THREAD_LOAD_LOADED) {
				return;
			}
		}
		Ref<Mesh> mesh = resource;
		if (mesh.is_null()) {
			return;
		}
		multimesh = memnew(MultiMesh);
		multimesh->set_mesh(mesh);
		multimesh->set_use_colors(true);
		multimesh->set_use_custom_data(true);
		multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
	}
	dirty = false;
	if (instance.is_null()) {
		instance = RenderingServer::get_singleton()->instance_create();
		RenderingServer::get_singleton()->instance_set_base(instance, multimesh->get_rid());
		RenderingServer::get_singleton()->instance_set_scenario(instance, p_world_3d_scenario);
		// 设置裁剪距离
		RS::get_singleton()->instance_geometry_set_visibility_range(instance, 0, 8192, 20, 100, RS::VISIBILITY_RANGE_FADE_SELF);
		set_gi_mode(gi_mode);
		set_shadow_setting(shadow_setting);
		RenderingServer::get_singleton()->instance_geometry_set_flag(instance, RenderingServer::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, false);
	}

	int block_count = 0;
	for (auto &it : blocks) {
		if (it == nullptr) {
			continue;
		}
		block_count += it->get_instance_count();
	}
	multimesh->set_instance_count(mesh_transforms.size() + block_count);
	tmp_transform_data.resize(multimesh->get_instance_count() * 12);
	tmp_color_data.resize(multimesh->get_instance_count() * 4);
	tmp_custom_data.resize(multimesh->get_instance_count() * 4);

	float *transform_data = tmp_transform_data.ptrw();
	float *color_data = tmp_color_data.ptrw();
	float *custom_data = tmp_custom_data.ptrw();
	bool is_update = false;
	int i = 0;
	for (auto &it : mesh_transforms) {
		if (it.value.last_index != it.key) {
			const Transform3D &p_transform = it.value.transform;
			transform_data[0] = p_transform.basis.rows[0][0];
			transform_data[1] = p_transform.basis.rows[0][1];
			transform_data[2] = p_transform.basis.rows[0][2];
			transform_data[3] = p_transform.origin.x;
			transform_data[4] = p_transform.basis.rows[1][0];
			transform_data[5] = p_transform.basis.rows[1][1];
			transform_data[6] = p_transform.basis.rows[1][2];
			transform_data[7] = p_transform.origin.y;
			transform_data[8] = p_transform.basis.rows[2][0];
			transform_data[9] = p_transform.basis.rows[2][1];
			transform_data[10] = p_transform.basis.rows[2][2];
			transform_data[11] = p_transform.origin.z;

			color_data[0] = it.value.color.r;
			color_data[1] = it.value.color.g;
			color_data[2] = it.value.color.b;
			color_data[3] = it.value.color.a;

			custom_data[0] = it.value.custom_data.r;
			custom_data[1] = it.value.custom_data.g;
			custom_data[2] = it.value.custom_data.b;
			custom_data[3] = it.value.custom_data.a;
			it.value.last_index = it.key;
			is_update = true;
		}
		transform_data += 12;
		color_data += 4;
		custom_data += 4;
		++i;
	}
	for (auto &it : blocks) {
		if (it == nullptr) {
			continue;
		}
		if (it->last_index != i) {
			const LocalVector<Transform3D> &trans = it->get_transform();
			const LocalVector<Color> &color = it->get_color();
			const LocalVector<Color> &curstom_colo = it->get_curstom_color();

			for (uint32_t j = 0; j < trans.size(); j++) {
				const Transform3D &t = trans[j];
				const Color &cv = color[j];
				const Color &c = curstom_colo[j];
				transform_data[0] = t.basis.rows[0][0];
				transform_data[1] = t.basis.rows[0][1];
				transform_data[2] = t.basis.rows[0][2];
				transform_data[3] = t.origin.x;
				transform_data[4] = t.basis.rows[1][0];
				transform_data[5] = t.basis.rows[1][1];
				transform_data[6] = t.basis.rows[1][2];
				transform_data[7] = t.origin.y;
				transform_data[8] = t.basis.rows[2][0];
				transform_data[9] = t.basis.rows[2][1];
				transform_data[10] = t.basis.rows[2][2];
				transform_data[11] = t.origin.z;

				color_data[0] = cv.r;
				color_data[1] = cv.g;
				color_data[2] = cv.b;
				color_data[3] = cv.a;

				custom_data[0] = c.r;
				custom_data[1] = c.g;
				custom_data[2] = c.b;
				custom_data[3] = c.a;

				transform_data += 12;
				color_data += 4;
				custom_data += 4;
				++i;
				is_update = true;
			}

		} else {
			i += it->get_transform().size();
			transform_data += it->get_transform().size() * 12;
			color_data += it->get_transform().size() * 4;
			custom_data += it->get_transform().size() * 4;
		}
	}

	if (is_update) {
		Ref<RDMultimeshUpdate> up = multimesh->get_update();
		up->update_static_instance(0, multimesh->get_instance_count(), tmp_transform_data, tmp_color_data, tmp_custom_data);
	}
}
void SceneChunk::MeshInstance::set_mesh_transform(int mesh_id, const Transform3D &t) {
	auto it = mesh_transforms.find(mesh_id);
	if (it != mesh_transforms.end()) {
		it->value.transform = t;
		it->value.last_index = -1;
		dirty = true;
	} else {
		mesh_transforms[mesh_id] = MeshInstanceInfo();
		mesh_transforms[mesh_id].transform = t;
		dirty = true;
	}
}
void SceneChunk::MeshInstance::set_mesh_color(int mesh_id, const Color &color) {
	auto it = mesh_transforms.find(mesh_id);
	if (it != mesh_transforms.end()) {
		it->value.color = color;
		it->value.last_index = -1;
		dirty = true;
	}
}
void SceneChunk::MeshInstance::set_mesh_custom_data(int mesh_id, const Color &color) {
	auto it = mesh_transforms.find(mesh_id);
	if (it != mesh_transforms.end()) {
		it->value.custom_data = color;
		it->value.last_index = -1;
		dirty = true;
	}
}

/*********************************************************************************************************/

int SceneChunkGroupInstance::_add_mesh_instance(const String &p_path, const Transform3D &t) {
	SceneChunk *chunk = Object::cast_to<SceneChunk>(ObjectDB::get_instance(chunk_id));
	if (chunk) {
		return chunk->add_multimesh_instance(p_path, t);
	}
	return -1;
}
int SceneChunkGroupInstance::_add_collision_instance(const Transform3D &t, SceneDataCompoent::CollisionShapeType type, const Vector3 &box_size, float height, float radius) {
	SceneChunk *chunk = Object::cast_to<SceneChunk>(ObjectDB::get_instance(chunk_id));
	if (chunk) {
		return chunk->add_collision_instance(t, type, box_size, height, radius);
	}
	return -1;
}
int SceneChunkGroupInstance::_add_mesh_collision_instance(const String &p_path, const Transform3D &t) {
	SceneChunk *chunk = Object::cast_to<SceneChunk>(ObjectDB::get_instance(chunk_id));
	if (chunk) {
		return chunk->add_collision_instance(t, SceneDataCompoent::CollisionShapeType::SHAPE_MESH, Vector3(), 0, 0);
	}
	return -1;
}

void SceneChunkGroupInstance::set_lod(int p_lod) {
	if (curr_lod == p_lod) {
		return;
	}
	curr_lod = p_lod;
	clear_show_instance_ids();
	SceneChunk *chunk = Object::cast_to<SceneChunk>(ObjectDB::get_instance(chunk_id));
	if (chunk) {
		if (resource.is_null()) {
			return;
		}
		resource->show(curr_lod, p_lod, global_transform, this);
	}
}
void SceneChunkGroupInstance::init(Node *p_node) {
	Node *parent = p_node;
	SceneChunk *chunk = Object::cast_to<SceneChunk>(parent);
	if (parent == nullptr) {
		return;
	}
	while (chunk == nullptr) {
		parent = parent->get_parent();
		if (parent == nullptr) {
			break;
		}
		chunk = Object::cast_to<SceneChunk>(parent);
	}
	init_chunk(chunk->get_instance_id());
}
SceneChunk *SceneChunkGroupInstance::get_chunk() {
	return Object::cast_to<SceneChunk>(ObjectDB::get_instance(chunk_id));
}
void SceneChunkGroupInstance::clear_show_instance_ids() {
	SceneChunk *chunk = Object::cast_to<SceneChunk>(ObjectDB::get_instance(chunk_id));
	if (chunk) {
		for (auto &it : curr_show_meshinstance_ids) {
			chunk->remove_multimesh_instance(it.value, it.key);
		}
		for (auto &it : curr_show_collision_ids) {
			chunk->remove_collision_instance(it);
		}
		for (auto &it : curr_show_mesh_collision_ids) {
			chunk->remove_mesh_collision_instance(it.key, it.value);
		}
	}
}

/*********************************************************************************************************/

SceneChunk::SceneChunk() {
	set_process(true);
}
int SceneChunk::add_multimesh_instance(const String &res_path, const Transform3D &t) {
	Ref<MeshInstance> mesh_instance;
	if (!mult_mesh_instances.has(res_path)) {
		Ref<ResourceLoader::LoadToken> token = ResourceLoader::_load_start(res_path, "", ResourceLoader::LOAD_THREAD_FROM_CURRENT, ResourceFormatLoader::CACHE_MODE_IGNORE);
		if (token.is_null()) {
			return -1;
		}
		mesh_instance = Ref<MeshInstance>(memnew(MeshInstance));
		mesh_instance->load_token = token;
		mult_mesh_instances[res_path] = mesh_instance;
	} else {
		mesh_instance = mult_mesh_instances[res_path];
	}
	int id = get_free_id();

	mesh_instance->set_mesh_transform(id, t);
	return id;
}
void SceneChunk::remove_multimesh_instance(const String &res_path, int id) {
	if (mult_mesh_instances.has(res_path)) {
		Ref<MeshInstance> mesh_instance = mult_mesh_instances[res_path];
		mesh_instance->remove_instance(id);
		unuse_id_list.push_back(id);
	}
}
void SceneChunk::add_multmesh_instance_block(const String &res_path, const Ref<Foliage::SceneInstanceBlock> &t) {
	Ref<MeshInstance> mesh_instance;
	if (!mult_mesh_instances.has(res_path)) {
		Ref<ResourceLoader::LoadToken> token = ResourceLoader::_load_start(res_path, "", ResourceLoader::LOAD_THREAD_FROM_CURRENT, ResourceFormatLoader::CACHE_MODE_IGNORE);
		if (token.is_null()) {
			return;
		}
		mesh_instance = Ref<MeshInstance>(memnew(MeshInstance));
		mesh_instance->load_token = token;
		mult_mesh_instances[res_path] = mesh_instance;
	} else {
		mesh_instance = mult_mesh_instances[res_path];
	}
	mesh_instance->blocks.insert(t);
}
void SceneChunk::remove_multmesh_instance_block(const String &res_path, const Ref<Foliage::SceneInstanceBlock> &t) {
	if (mult_mesh_instances.has(res_path)) {
		Ref<MeshInstance> mesh_instance = mult_mesh_instances[res_path];
		mesh_instance->blocks.erase(t);
	}
}

int SceneChunk::add_collision_instance(const Transform3D &t, SceneDataCompoent::CollisionShapeType type, const Vector3 &box_size, float height, float radius) {
	int id = -1;
	switch (type) {
		case SceneDataCompoent::CollisionShapeType::SHAPE_MESH:
			break;
		case SceneDataCompoent::CollisionShapeType::SHAPE_BOX: {
			id = get_free_id();
			RID box_shape = PhysicsServer3D::get_singleton()->box_shape_create();
			PhysicsServer3D::get_singleton()->shape_set_data(box_shape, box_size);
			RID owner_id = PhysicsServer3D::get_singleton()->body_create();

			Collision &collision = collision_instances[id];
			collision.node_id = owner_id;
			collision.shape = box_shape;
			collision.collision_layer = 1;
			collision.collision_mask = 1;
			PhysicsServer3D::get_singleton()->body_set_collision_layer(owner_id, collision.collision_layer);
			PhysicsServer3D::get_singleton()->body_set_collision_mask(owner_id, collision.collision_mask);
			PhysicsServer3D::get_singleton()->body_add_shape(owner_id, box_shape, t);
			PhysicsServer3D::get_singleton()->body_set_mode(owner_id, PhysicsServer3D::BODY_MODE_STATIC);
		} break;
		case SceneDataCompoent::CollisionShapeType::SHAPE_CAPSULE: {
			id = get_free_id();
			RID capsule_shape = PhysicsServer3D::get_singleton()->capsule_shape_create();
			Dictionary new_d;
			new_d["radius"] = radius;
			new_d["height"] = height;
			PhysicsServer3D::get_singleton()->shape_set_data(capsule_shape, new_d);
			RID owner_id = PhysicsServer3D::get_singleton()->body_create();

			Collision &collision = collision_instances[id];
			collision.node_id = owner_id;
			collision.shape = capsule_shape;
			collision.collision_layer = 1;
			collision.collision_mask = 1;
			PhysicsServer3D::get_singleton()->body_set_collision_layer(owner_id, collision.collision_layer);
			PhysicsServer3D::get_singleton()->body_set_collision_mask(owner_id, collision.collision_mask);
			PhysicsServer3D::get_singleton()->body_add_shape(owner_id, capsule_shape, t);
			PhysicsServer3D::get_singleton()->body_set_mode(owner_id, PhysicsServer3D::BODY_MODE_STATIC);
		} break;
		case SceneDataCompoent::CollisionShapeType::SHAPE_CYLINDER: {
			id = get_free_id();
			RID cylinder_shape = PhysicsServer3D::get_singleton()->cylinder_shape_create();
			Dictionary new_d;
			new_d["radius"] = radius;
			new_d["height"] = height;
			PhysicsServer3D::get_singleton()->shape_set_data(cylinder_shape, new_d);
			RID owner_id = PhysicsServer3D::get_singleton()->body_create();

			Collision &collision = collision_instances[id];
			collision.node_id = owner_id;
			collision.shape = cylinder_shape;
			collision.collision_layer = 1;
			collision.collision_mask = 1;
			PhysicsServer3D::get_singleton()->body_set_collision_layer(owner_id, collision.collision_layer);
			PhysicsServer3D::get_singleton()->body_set_collision_mask(owner_id, collision.collision_mask);
			PhysicsServer3D::get_singleton()->body_add_shape(owner_id, cylinder_shape, t);
			PhysicsServer3D::get_singleton()->body_set_mode(owner_id, PhysicsServer3D::BODY_MODE_STATIC);
		} break;
		case SceneDataCompoent::CollisionShapeType::SHAPE_SPHERE: {
			id = get_free_id();
			RID sphere_shape = PhysicsServer3D::get_singleton()->sphere_shape_create();
			PhysicsServer3D::get_singleton()->shape_set_data(sphere_shape, radius);
			RID owner_id = PhysicsServer3D::get_singleton()->body_create();

			Collision &collision = collision_instances[id];
			collision.node_id = owner_id;
			collision.shape = sphere_shape;
			collision.collision_layer = 1;
			collision.collision_mask = 1;
			PhysicsServer3D::get_singleton()->body_set_collision_layer(owner_id, collision.collision_layer);
			PhysicsServer3D::get_singleton()->body_set_collision_mask(owner_id, collision.collision_mask);
			PhysicsServer3D::get_singleton()->body_add_shape(owner_id, sphere_shape, t);
			PhysicsServer3D::get_singleton()->body_set_mode(owner_id, PhysicsServer3D::BODY_MODE_STATIC);
		} break;
		default:
			break;
	}
	return id;
}
void SceneChunk::remove_collision_instance(int id) {
	if (collision_instances.has(id)) {
		Collision &collision = collision_instances[id];
		PhysicsServer3D::get_singleton()->free(collision.shape);
		PhysicsServer3D::get_singleton()->free(collision.node_id);
		collision_instances.erase(id);
	}
}

int SceneChunk::add_mesh_collision_instance(const Transform3D &t, const String &p_path) {
	Ref<MeshCollisionInstance> mesh_collision_instance;
	if (mesh_collision_instances.has(p_path)) {
		mesh_collision_instance = mesh_collision_instances[p_path];
	} else {
		Ref<MeshCollisionResource> mesh_collision_resource = ResourceLoader::load(p_path);
		if (mesh_collision_resource.is_null()) {
			return -1;
		}
		mesh_collision_instance.instantiate();
		mesh_collision_instances[p_path] = mesh_collision_instance;
		RID shape = PhysicsServer3D::get_singleton()->concave_polygon_shape_create();
		PhysicsServer3D::get_singleton()->shape_set_data(shape, mesh_collision_resource->get_points());
		mesh_collision_instance->shape = shape;
	}
	RID owner_id = PhysicsServer3D::get_singleton()->body_create();

	int id = get_free_id();
	Collision &collision = mesh_collision_instance->mesh_transforms[id];
	PhysicsServer3D::get_singleton()->body_add_shape(owner_id, mesh_collision_instance->shape, t);
	PhysicsServer3D::get_singleton()->body_set_mode(owner_id, PhysicsServer3D::BODY_MODE_STATIC);
	PhysicsServer3D::get_singleton()->body_set_collision_layer(owner_id, collision.collision_layer);
	PhysicsServer3D::get_singleton()->body_set_collision_mask(owner_id, collision.collision_mask);
	return id;
}
void SceneChunk::remove_mesh_collision_instance(int id, const String &p_path) {
	if (mesh_collision_instances.has(p_path)) {
		Ref<MeshCollisionInstance> mesh_collision_resource = mesh_collision_instances[p_path];
		if (mesh_collision_resource.is_null()) {
			return;
		}
		if (mesh_collision_resource->mesh_transforms.has(id)) {
			Collision &collision = mesh_collision_resource->mesh_transforms[id];
			PhysicsServer3D::get_singleton()->free(collision.node_id);
			mesh_collision_resource->mesh_transforms.erase(id);
		}
	}
}

void SceneChunk::process(double p_delta) {
	// 處理裁剪
	//Camera3D *camera = get_viewport()->get_camera_3d();
	RID world_3d_scenario = get_world_3d()->get_scenario();

	for (auto it : mult_mesh_instances) {
		it.value->update_mesh_instance(world_3d_scenario);
	}
}

void SceneChunk::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_multimesh_instance", "res_path", "transform"), &SceneChunk::add_multimesh_instance);
	ClassDB::bind_method(D_METHOD("remove_multimesh_instance", "res_path", "id"), &SceneChunk::remove_multimesh_instance);

	ClassDB::bind_method(D_METHOD("add_collision_instance", "transform", "type"), &SceneChunk::add_collision_instance);
	ClassDB::bind_method(D_METHOD("remove_collision_instance", "id"), &SceneChunk::remove_collision_instance);

	ClassDB::bind_method(D_METHOD("add_mesh_collision_instance", "transform", "res_path"), &SceneChunk::add_mesh_collision_instance);
	ClassDB::bind_method(D_METHOD("remove_mesh_collision_instance", "id", "res_path"), &SceneChunk::remove_mesh_collision_instance);

	ClassDB::bind_method(D_METHOD("add_multmesh_instance_block", "res_path", "block"), &SceneChunk::add_multmesh_instance_block);
	ClassDB::bind_method(D_METHOD("remove_multmesh_instance_block", "res_path", "block"), &SceneChunk::remove_multmesh_instance_block);
}
