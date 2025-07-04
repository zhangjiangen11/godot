#include "mmesh.h"

#include "mhlod_scene.h"
#include <mutex>

#define RSS RenderingServer::get_singleton()

void MMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("surface_set_name", "sidx", "name"), &MMesh::surface_set_name);
	ClassDB::bind_method(D_METHOD("surface_get_name", "sidx"), &MMesh::surface_get_name);

	ClassDB::bind_method(D_METHOD("create_from_mesh", "mesh"), &MMesh::create_from_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh_rid"), &MMesh::get_mesh_rid);
	ClassDB::bind_method(D_METHOD("get_mesh"), &MMesh::get_mesh);

	//ClassDB::bind_method(D_METHOD("get_surface_count"), &MMesh::get_surface_count);
	//ClassDB::bind_method(D_METHOD("get_aabb"), &MMesh::get_aabb);
	//ClassDB::bind_method(D_METHOD("material_set_get_count"), &MMesh::material_set_get_count);
	ClassDB::bind_method(D_METHOD("material_set_get", "set_id"), &MMesh::material_set_get);
	ClassDB::bind_method(D_METHOD("material_get", "set_id", "surface_index"), &MMesh::material_get);
	ClassDB::bind_method(D_METHOD("surfaceset_material", "set_id", "surface_index", "material_path"), &MMesh::surface_set_material);
	ClassDB::bind_method(D_METHOD("add_material_set"), &MMesh::add_material_set);
	ClassDB::bind_method(D_METHOD("material_set_resize", "size"), &MMesh::material_set_resize);
	ClassDB::bind_method(D_METHOD("clear_material_set", "set_id"), &MMesh::clear_material_set);

	ClassDB::bind_method(D_METHOD("is_same_mesh", "other"), &MMesh::is_same_mesh);

	ClassDB::bind_method(D_METHOD("_set_surfaces", "surfaces"), &MMesh::_set_surfaces);
	ClassDB::bind_method(D_METHOD("_get_surfaces"), &MMesh::_get_surfaces);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "_surfaces", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_surfaces", "_get_surfaces");
}

void MMesh::_create_if_not_exist() {
	if (!mesh.is_valid()) {
		mesh = RSS->mesh_create();
	}
}

MMesh::MaterialSet::MaterialSet(int surface_count) {
	PackedStringArray _str_paths;
	_str_paths.resize(surface_count);
	set_surface_materials_paths(_str_paths);
}

MMesh::MaterialSet::MaterialSet(const PackedStringArray &_material_paths) {
	set_surface_materials_paths(_material_paths);
}

MMesh::MaterialSet::MaterialSet(const PackedByteArray &_material_paths) {
	surface_materials_paths = _material_paths;
}

MMesh::MaterialSet::~MaterialSet() {
}

static String get_string_from_ascii(const PackedByteArray &p_buffer) {
	String s;
	if (p_buffer.size() > 0) {
		const uint8_t *r = p_buffer.ptr();
		CharString cs;
		cs.resize_uninitialized(p_buffer.size() + 1);
		memcpy(cs.ptrw(), r, p_buffer.size());
		cs[(int)p_buffer.size()] = 0;

		s = cs.get_data();
	}
	return s;
}
PackedStringArray MMesh::MaterialSet::get_surface_materials_paths() const {
	return get_string_from_ascii(surface_materials_paths).split(";");
}

void MMesh::MaterialSet::set_surface_materials_paths(const PackedStringArray &paths) {
	if (paths.size() == 0) {
		surface_materials_paths.clear();
		return;
	}
	String _str = paths[0];
	for (int i = 1; i < paths.size(); i++) {
		_str += ";" + paths[i];
	}
	surface_materials_paths = _str.to_ascii_buffer();
}

void MMesh::MaterialSet::clear() {
	PackedStringArray empty_path;
	empty_path.resize(get_surface_count());
	set_surface_materials_paths(empty_path);
}

bool MMesh::MaterialSet::has_cache() const {
	return materials_cache.size() != 0;
}

int MMesh::MaterialSet::get_surface_count() const {
	if (surface_materials_paths.size() == 0) {
		return 1;
	}
	return surface_materials_paths.count(MPATH_DELIMTER) + 1;
}

void MMesh::MaterialSet::set_material(int surface_index, Ref<Material> material) {
	ERR_FAIL_COND_MSG(material.is_valid() && material->get_path().is_empty(), "Material should saved first to be added on MMesh");
	String path;
	if (material.is_valid()) {
		path = material->get_path();
		ERR_FAIL_COND_MSG(path.find("::") != -1, "Material should save as independent file current path is: " + path);
	}
	PackedStringArray _str_paths = get_surface_materials_paths();
	ERR_FAIL_INDEX(surface_index, _str_paths.size());
	_str_paths.set(surface_index, path);
	set_surface_materials_paths(_str_paths);
}

void MMesh::MaterialSet::set_material_no_error(int surface_index, Ref<Material> material) {
	if (material.is_valid() && material->get_path().is_empty()) {
		return;
	}
	String path;
	if (material.is_valid()) {
		path = material->get_path();
		if (path.find("::") != -1) {
			return;
		}
	}
	PackedStringArray _str_paths = get_surface_materials_paths();
	ERR_FAIL_INDEX(surface_index, _str_paths.size());
	_str_paths.set(surface_index, path);
	set_surface_materials_paths(_str_paths);
}

// This function should be called only in main loop
// Usually used for editor stuff
Ref<Material> MMesh::MaterialSet::get_material_no_user(int surface_index) const {
	std::lock_guard<std::mutex> lock(MHlodScene::update_mutex);
	if (has_cache()) {
	}
	PackedStringArray _str_paths = get_surface_materials_paths();
	ERR_FAIL_INDEX_V(surface_index, _str_paths.size(), Ref<Material>());
	if (_str_paths[surface_index].is_empty() || !_str_paths[surface_index].is_absolute_path()) {
		return Ref<Material>();
	}
	return ResourceLoader::load(_str_paths[surface_index]);
}

void MMesh::MaterialSet::get_materials_add_user(Vector<RID> &materials_rid) {
	add_user();
	for (int i = 0; i < materials_cache.size(); i++) {
		if (materials_cache[i].is_null()) {
			materials_rid.push_back(RID());
		} else {
			materials_rid.push_back(materials_cache[i]->get_rid());
		}
	}
}

void MMesh::MaterialSet::get_materials(Vector<RID> &materials_rid) {
	ERR_FAIL_COND(materials_cache.size() == 0);
	for (int i = 0; i < materials_cache.size(); i++) {
		if (materials_cache[i].is_null()) {
			materials_rid.push_back(RID());
		} else {
			materials_rid.push_back(materials_cache[i]->get_rid());
		}
	}
}

void MMesh::MaterialSet::update_cache() {
	if (!has_cache()) {
		return;
	}
	materials_cache.clear();
	PackedStringArray _str_paths = get_surface_materials_paths();
	for (int i = 0; i < _str_paths.size(); i++) {
		if (_str_paths[i].is_empty()) {
			materials_cache.push_back(Ref<Material>());
			continue;
		}
		materials_cache.push_back(ResourceLoader::load(_str_paths[i]));
	}
}

void MMesh::MaterialSet::add_user() {
	user_count++;
	if (!has_cache()) {
		materials_cache.clear();
		PackedStringArray _str_paths = get_surface_materials_paths();
		for (int i = 0; i < _str_paths.size(); i++) {
			if (_str_paths[i].is_empty()) {
				materials_cache.push_back(Ref<Material>());
				continue;
			}
			materials_cache.push_back(ResourceLoader::load(_str_paths[i]));
		}
	}
}

void MMesh::MaterialSet::remove_user() {
	ERR_FAIL_COND(user_count == 0); // this should not happen, but will happen when you call extra remove_user
	user_count--; // I hope this happen in only one thread
	if (user_count == 0) {
		materials_cache.clear();
	}
}

PackedStringArray MMesh::surfaces_get_names() const {
	return get_string_from_ascii(surfaces_names).split(";");
}

void MMesh::surfaces_set_names(const PackedStringArray &_surfaces_names) {
	if (_surfaces_names.size() == 0) {
		surfaces_names.clear();
		return;
	}
	String _str = _surfaces_names[0];
	for (int i = 1; i < _surfaces_names.size(); i++) {
		_str += ";" + _surfaces_names[i];
	}
	surfaces_names = _str.to_ascii_buffer();
}

MMesh::MMesh() {
}

MMesh::~MMesh() {
	if (mesh.is_valid()) {
		RSS->free(mesh);
	}
}

void MMesh::surface_set_name(int surface_index, const String &new_name) {
	PackedStringArray _snames = surfaces_get_names();
	_snames.set(surface_index, new_name);
	surfaces_set_names(_snames);
}

String MMesh::surface_get_name(int surface_index) const {
	PackedStringArray _snames = surfaces_get_names();
	return _snames[surface_index];
}

Array MMesh::surface_get_arrays(int surface_index) const {
	Array out;
	if (!mesh.is_valid()) {
		return out;
	}
	return RSS->mesh_surface_get_arrays(mesh, surface_index);
}

RID MMesh::get_mesh_rid() {
	_create_if_not_exist();
	return mesh;
}

void MMesh::create_from_mesh(Ref<Mesh> input) {
	ERR_FAIL_COND(input.is_null());
	int surface_count = input->get_surface_count();
	ERR_FAIL_COND(surface_count == 0);
	materials_set.clear();
	Array _surfaces;
	RID input_rid = input->get_rid();
	MaterialSet _fms(surface_count);
	for (int i = 0; i < surface_count; i++) {
		_fms.set_material_no_error(i, input->surface_get_material(i));
	}
	// Material Path
	PackedByteArray _bpaths = _fms.surface_materials_paths;
	Array ___m;
	___m.push_back(_bpaths);
	_surfaces.push_back(___m);
	// Mesh data
	for (int s = 0; s < surface_count; s++) {
		RS::SurfaceData sd = RSS->mesh_get_surface(input_rid, s);

		Dictionary d;
		d["primitive"] = sd.primitive;
		d["format"] = sd.format;
		d["vertex_data"] = sd.vertex_data;
		if (sd.attribute_data.size()) {
			d["attribute_data"] = sd.attribute_data;
		}
		if (sd.skin_data.size()) {
			d["skin_data"] = sd.skin_data;
		}
		d["vertex_count"] = sd.vertex_count;
		if (sd.index_count) {
			d["index_data"] = sd.index_data;
			d["index_count"] = sd.index_count;
		}
		d["aabb"] = sd.aabb;
		d["uv_scale"] = sd.uv_scale;

		if (sd.lods.size()) {
			Array lods;
			for (int l = 0; l < sd.lods.size(); l++) {
				Dictionary ld;
				ld["edge_length"] = sd.lods[l].edge_length;
				ld["index_data"] = sd.lods[l].index_data;
				lods.push_back(lods);
			}
			d["lods"] = lods;
		}

		if (sd.bone_aabbs.size()) {
			Array aabbs;
			for (int i = 0; i < sd.bone_aabbs.size(); i++) {
				aabbs.push_back(sd.bone_aabbs[i]);
			}
			d["bone_aabbs"] = aabbs;
		}

		if (sd.blend_shape_data.size()) {
			d["blend_shape_data"] = sd.blend_shape_data;
		}

		if (sd.material.is_valid()) {
			d["material"] = sd.material;
		}
		d["name"] = input->call("surface_get_name", s);
		_surfaces.push_back(d);
	}
	_set_surfaces(_surfaces);
}

Ref<ArrayMesh> MMesh::get_mesh() const {
	Array _surfaces = _get_surfaces();
	// Removing Material Section
	if (_surfaces.size() > 0 && _surfaces[0].get_type() == Variant::Type::ARRAY) {
		_surfaces.remove_at(0);
	}
	Ref<ArrayMesh> out;
	out.instantiate();
	out->set("_surfaces", _surfaces);
	ERR_FAIL_COND_V(materials_set.size() == 0, out);
	for (int i = 0; i < _surfaces.size(); i++) {
		out->surface_set_material(i, materials_set[0].get_material_no_user(i));
		out->surface_set_name(i, surface_get_name(i));
	}
	return out;
}

int MMesh::get_surface_count() const {
	if (!mesh.is_valid()) {
		return 0;
	}
	ERR_FAIL_COND_V(materials_set.size() == 0, 0);
	return materials_set[0].get_surface_count();
}

AABB MMesh::get_aabb() const {
	return aabb;
}

int MMesh::material_set_get_count() const {
	return materials_set.size();
}

PackedStringArray MMesh::material_set_get(int set_id) const {
	ERR_FAIL_INDEX_V(set_id, materials_set.size(), PackedStringArray());
	return materials_set[set_id].get_surface_materials_paths();
}

String MMesh::material_get(int set_id, int surface_index) const {
	ERR_FAIL_INDEX_V(set_id, materials_set.size(), String(""));
	PackedStringArray __surfaces = materials_set[set_id].get_surface_materials_paths();
	ERR_FAIL_INDEX_V(surface_index, __surfaces.size(), String(""));
	return __surfaces[surface_index];
}

void MMesh::surface_set_material(int set_id, int surface_index, const String &path) {
	ERR_FAIL_INDEX(set_id, materials_set.size());
	PackedStringArray __surfaces = materials_set[set_id].get_surface_materials_paths();
	ERR_FAIL_INDEX(surface_index, __surfaces.size());
	__surfaces.set(surface_index, path);
	materials_set.ptrw()[set_id].set_surface_materials_paths(__surfaces);
	materials_set.ptrw()[set_id].update_cache();
	update_material_override();
}

int MMesh::add_material_set() {
	MaterialSet new_set(get_surface_count());
	materials_set.push_back(new_set);
	return materials_set.size() - 1;
}

void MMesh::material_set_resize(int size) {
	ERR_FAIL_COND(size < 1);
	if (size < materials_set.size()) {
		materials_set.resize(size);
	} else {
		while (size != materials_set.size()) {
			add_material_set();
		}
	}
}

void MMesh::clear_material_set(int set_id) {
	ERR_FAIL_INDEX(set_id, materials_set.size());
	materials_set.ptrw()[set_id].clear();
}

bool MMesh::has_material_override() {
	return materials_set.size() > 1;
}

void MMesh::update_material_override() {
	// Setting mesh material if we have only one set
	if (!has_material_override() && materials_set.size() != 0 && mesh.is_valid()) {
		Vector<RID> materials_rid;
		materials_set.ptrw()[0].get_materials_add_user(materials_rid);
		for (int i = 0; i < materials_rid.size(); i++) {
			if (materials_rid[i].is_valid()) {
				RSS->mesh_surface_set_material(mesh, i, materials_rid[i]);
			}
		}
		return;
	}
	if (mesh.is_valid()) {
		int surface_count = get_surface_count();
		for (int i = 0; i < surface_count; i++) {
			RSS->mesh_surface_set_material(mesh, i, RID());
		}
	}
}

void MMesh::get_materials_add_user(int material_set_id, Vector<RID> &materials_rid) {
	ERR_FAIL_INDEX(material_set_id, materials_set.size());
	materials_set.ptrw()[material_set_id].get_materials_add_user(materials_rid);
}

void MMesh::get_materials(int material_set_id, Vector<RID> &materials_rid) {
	ERR_FAIL_INDEX(material_set_id, materials_set.size());
	materials_set.ptrw()[material_set_id].get_materials(materials_rid);
}

void MMesh::add_user(int material_set_id) {
	ERR_FAIL_INDEX(material_set_id, materials_set.size());
	materials_set.ptrw()[material_set_id].add_user();
}

void MMesh::remove_user(int material_set_id) {
	ERR_FAIL_INDEX(material_set_id, materials_set.size());
	materials_set.ptrw()[material_set_id].remove_user();
}

bool MMesh::is_same_mesh(Ref<MMesh> other) {
	if (other.is_null()) {
		return false;
	}
	if (get_surface_count() != other->get_surface_count()) {
		return false;
	}
	if (material_set_get_count() != other->material_set_get_count()) {
		return false;
	}
	if (surfaces_get_names() != other->surfaces_get_names()) {
		return false;
	}
	for (int s = 0; s < get_surface_count(); s++) {
		Array my_info = surface_get_arrays(s);
		Array other_info = other->surface_get_arrays(s);
		for (int i = 0; i < Mesh::ARRAY_MAX; i++) {
			if (my_info[i] != other_info[i]) {
				return false;
			}
		}
	}
	return true;
}

static RS::SurfaceData _dict_to_surf(const Dictionary &p_dictionary) {
	ERR_FAIL_COND_V(!p_dictionary.has("primitive"), RS::SurfaceData());
	ERR_FAIL_COND_V(!p_dictionary.has("format"), RS::SurfaceData());
	ERR_FAIL_COND_V(!p_dictionary.has("vertex_data"), RS::SurfaceData());
	ERR_FAIL_COND_V(!p_dictionary.has("vertex_count"), RS::SurfaceData());
	ERR_FAIL_COND_V(!p_dictionary.has("aabb"), RS::SurfaceData());

	RS::SurfaceData sd;

	sd.primitive = RS::PrimitiveType(int(p_dictionary["primitive"]));
	sd.format = p_dictionary["format"];
	sd.vertex_data = p_dictionary["vertex_data"];
	if (p_dictionary.has("attribute_data")) {
		sd.attribute_data = p_dictionary["attribute_data"];
	}
	if (p_dictionary.has("skin_data")) {
		sd.skin_data = p_dictionary["skin_data"];
	}

	sd.vertex_count = p_dictionary["vertex_count"];

	if (p_dictionary.has("index_data")) {
		sd.index_data = p_dictionary["index_data"];
		ERR_FAIL_COND_V(!p_dictionary.has("index_count"), RS::SurfaceData());
		sd.index_count = p_dictionary["index_count"];
	}

	sd.aabb = p_dictionary["aabb"];
	if (p_dictionary.has("uv_scale")) {
		sd.uv_scale = p_dictionary["uv_scale"];
	}

	if (p_dictionary.has("lods")) {
		Array lods = p_dictionary["lods"];
		for (int i = 0; i < lods.size(); i++) {
			Dictionary lod = lods[i];
			ERR_CONTINUE(!lod.has("edge_length"));
			ERR_CONTINUE(!lod.has("index_data"));
			RS::SurfaceData::LOD l;
			l.edge_length = lod["edge_length"];
			l.index_data = lod["index_data"];
			sd.lods.push_back(l);
		}
	}

	if (p_dictionary.has("bone_aabbs")) {
		Array aabbs = p_dictionary["bone_aabbs"];
		for (int i = 0; i < aabbs.size(); i++) {
			AABB aabb = aabbs[i];
			sd.bone_aabbs.push_back(aabb);
		}
	}

	if (p_dictionary.has("blend_shape_data")) {
		sd.blend_shape_data = p_dictionary["blend_shape_data"];
	}

	if (p_dictionary.has("material")) {
		sd.material = p_dictionary["material"];
	}

	return sd;
}

void MMesh::_set_surfaces(Array _surfaces) {
	_create_if_not_exist();
	RSS->mesh_clear(mesh);
	materials_set.clear();
	// Material
	if (_surfaces.size() > 0 && _surfaces[0].get_type() == Variant::Type::ARRAY) {
		// Then the first element is materials_set
		Array _ms = _surfaces[0];
		for (int i = 0; i < _ms.size(); i++) {
			PackedByteArray _ms_path_ascci = _ms[i];
			materials_set.push_back(MaterialSet(_ms_path_ascci));
		}
		_surfaces.remove_at(0);
	}
	// Check Import
	int surface_count = _surfaces.size();
	ERR_FAIL_COND(surface_count == 0);
	if (materials_set.size() == 0) {
		MaterialSet _ms(surface_count);
		materials_set.push_back(_ms);
	}
	PackedStringArray _surfaces_names;
	// Mesh Data
	for (int i = 0; i < surface_count; i++) {
		Dictionary sdata = _surfaces[i];
		_surfaces_names.push_back(sdata["name"]);
		RSS->mesh_add_surface(mesh, _dict_to_surf(sdata));
		ERR_CONTINUE(!sdata.has("aabb"));
		if (i == 0) {
			aabb = sdata["aabb"];
		} else {
			aabb.merge_with(sdata["aabb"]);
		}
	}
	update_material_override();
	surfaces_set_names(_surfaces_names);
	notify_property_list_changed();
}

Array MMesh::_get_surfaces() const {
	Array _surfaces;
	Array _ms;
	for (int i = 0; i < materials_set.size(); i++) {
		_ms.push_back(materials_set[i].surface_materials_paths);
	}
	_surfaces.push_back(_ms);
	if (!mesh.is_valid()) {
		return _surfaces;
	}
	int surface_count = get_surface_count();
	PackedStringArray _surfaces_names = surfaces_get_names();
	for (int s = 0; s < surface_count; s++) {
		RS::SurfaceData sd = RSS->mesh_get_surface(mesh, s);
		Dictionary d;
		d["primitive"] = sd.primitive;
		d["format"] = sd.format;
		d["vertex_data"] = sd.vertex_data;
		if (sd.attribute_data.size()) {
			d["attribute_data"] = sd.attribute_data;
		}
		if (sd.skin_data.size()) {
			d["skin_data"] = sd.skin_data;
		}
		d["vertex_count"] = sd.vertex_count;
		if (sd.index_count) {
			d["index_data"] = sd.index_data;
			d["index_count"] = sd.index_count;
		}
		d["aabb"] = sd.aabb;
		d["uv_scale"] = sd.uv_scale;

		if (sd.lods.size()) {
			Array lods;
			for (int i = 0; i < sd.lods.size(); i++) {
				Dictionary ld;
				ld["edge_length"] = sd.lods[i].edge_length;
				ld["index_data"] = sd.lods[i].index_data;
				lods.push_back(lods);
			}
			d["lods"] = lods;
		}

		if (sd.bone_aabbs.size()) {
			Array aabbs;
			for (int i = 0; i < sd.bone_aabbs.size(); i++) {
				aabbs.push_back(sd.bone_aabbs[i]);
			}
			d["bone_aabbs"] = aabbs;
		}

		if (sd.blend_shape_data.size()) {
			d["blend_shape_data"] = sd.blend_shape_data;
		}

		if (sd.material.is_valid()) {
			d["material"] = sd.material;
		}
		d["name"] = _surfaces_names[s];
		_surfaces.push_back(d);
	}
	return _surfaces;
}

bool MMesh::_set(const StringName &p_name, const Variant &p_value) {
	if (p_name.begins_with("materials/set_")) {
		String mname = p_name.replace("materials/set_", "");
		int e = mname.find("/");
		int e2 = mname.find("_");
		if (e == -1 || e2 == -1) {
			return false;
		}
		int set_index = mname.substr(0, e).to_int();
		if (set_index < 0 || set_index >= materials_set.size()) {
			return false;
		}
		e++;
		int surface_index = mname.substr(e, e2 - e).to_int();
		materials_set.ptrw()[set_index].set_material(surface_index, p_value);
		materials_set.ptrw()[set_index].update_cache();
		update_material_override();
		return true;
	}
	return false;
}

bool MMesh::_get(const StringName &p_name, Variant &r_ret) const {
	if (p_name.begins_with("materials/set_")) {
		String mname = p_name.replace("materials/set_", "");
		int e = mname.find("/");
		int e2 = mname.find("_");
		if (e == -1 || e2 == -1) {
			return false;
		}
		int set_index = mname.substr(0, e).to_int();
		if (set_index < 0 || set_index >= materials_set.size()) {
			return false;
		}
		e++;
		int surface_index = mname.substr(e, e2 - e).to_int();
		r_ret = materials_set[set_index].get_material_no_user(surface_index);
		return true;
	}
	return false;
}

void MMesh::_get_property_list(List<PropertyInfo> *p_list) const {
	if (!mesh.is_valid()) {
		return;
	}
	int surface_count = get_surface_count();
	String prefix = "materials/set_";
	PackedStringArray __surfaces_names = surfaces_get_names();
	ERR_FAIL_COND(__surfaces_names.size() != surface_count);
	for (int ms = 0; ms < materials_set.size(); ms++) {
		String prefix_ms = prefix + itos(ms) + String("/");
		for (int s = 0; s < surface_count; s++) {
			String pname = prefix_ms + itos(s) + String("_") + __surfaces_names[s];
			PropertyInfo prop(Variant::OBJECT, pname, PROPERTY_HINT_RESOURCE_TYPE, "BaseMaterial3D,ShaderMaterial", PROPERTY_USAGE_EDITOR);
			p_list->push_back(prop);
		}
	}
}

String MMesh::_to_string() {
	return String("<MMesh-,") + itos(get_instance_id()) + String(">");
}

RID MMesh::get_rid() const {
	return mesh;
}

int32_t MMesh::surface_get_array_len(int32_t p_index) const {
	ERR_FAIL_INDEX_V(p_index, surfaces.size(), 0);
	return surfaces[p_index].array_length;
}

int32_t MMesh::surface_get_array_index_len(int32_t p_index) const {
	ERR_FAIL_INDEX_V(p_index, surfaces.size(), 0);
	return surfaces[p_index].index_array_length;
}

BitField<Mesh::ArrayFormat> MMesh::surface_get_format(int32_t p_index) const {
	ERR_FAIL_INDEX_V(p_index, surfaces.size(), 0);
	return surfaces[p_index].format;
}

Mesh::PrimitiveType MMesh::surface_get_primitive_type(int32_t p_index) const {
	ERR_FAIL_INDEX_V(p_index, surfaces.size(), PRIMITIVE_POINTS);
	return surfaces[p_index].primitive;
}

int32_t MMesh::get_blend_shape_count() const {
	return 0;
}

/*
void MMesh::_surface_set_material(int32_t p_index, const Ref<Material> &p_material){

}

Ref<Material> MMesh::_surface_get_material(int32_t p_index) const{

}
*/
