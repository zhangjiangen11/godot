#include "foliage_proto_type.h"
#include "foliage_engine.h"

namespace Foliage {
void FoliagePrototype::load(Ref<FileAccess> &file, bool big_endian) {
	guid = file->get_utf8_string_buffer();
	name = file->get_utf8_string_buffer();
	lod0PrefabPath = file->get_utf8_string_buffer();
	lod1PrefabPath = file->get_utf8_string_buffer();
	lod2PrefabPath = file->get_utf8_string_buffer();
	lod3PrefabPath = file->get_utf8_string_buffer();
	colliderPath = file->get_utf8_string_buffer();
	is_convex_collider = file->get_8();
	lod0EndDistance = file->get_float();
	lod1EndDistance = file->get_float();
	lod2EndDistance = file->get_float();
	lod3EndDistance = file->get_float();
	mobileLod0EndDistance = file->get_float();
	mobileLod1EndDistance = file->get_float();
	mobileLod2EndDistance = file->get_float();
	boxOS.position = Vector3(file->get_float(), file->get_float(), file->get_float());
	boxOS.size = Vector3(file->get_float(), file->get_float(), file->get_float());
	foliage_position_xz_roots_size = Vector3(file->get_float(), file->get_float(), file->get_float());
}

void FoliagePrototype::save(Ref<FileAccess> &file) {
	file->store_utf8_string_buffer(guid);
	file->store_utf8_string_buffer(name);
	file->store_utf8_string_buffer(lod0PrefabPath);
	file->store_utf8_string_buffer(lod1PrefabPath);
	file->store_utf8_string_buffer(lod2PrefabPath);
	file->store_utf8_string_buffer(lod3PrefabPath);
	file->store_utf8_string_buffer(colliderPath);
	file->store_8(is_convex_collider);
	file->store_float(lod0EndDistance);
	file->store_float(lod1EndDistance);
	file->store_float(lod2EndDistance);
	file->store_float(lod3EndDistance);
	file->store_float(mobileLod0EndDistance);
	file->store_float(mobileLod1EndDistance);
	file->store_float(mobileLod2EndDistance);
	file->store_float(boxOS.position.x);
	file->store_float(boxOS.position.y);
	file->store_float(boxOS.position.z);
	file->store_float(boxOS.size.x);
	file->store_float(boxOS.size.y);
	file->store_float(boxOS.size.z);
	file->store_float(foliage_position_xz_roots_size.x);
	file->store_float(foliage_position_xz_roots_size.y);
	file->store_float(foliage_position_xz_roots_size.z);
}

void FoliagePrototype::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_guid", "guid"), &FoliagePrototype::set_guid);
	ClassDB::bind_method(D_METHOD("get_guid"), &FoliagePrototype::get_guid);

	ClassDB::bind_method(D_METHOD("set_name", "name"), &FoliagePrototype::set_name);
	ClassDB::bind_method(D_METHOD("get_name"), &FoliagePrototype::get_name);

	ClassDB::bind_method(D_METHOD("set_lod0PrefabPath", "path"), &FoliagePrototype::set_lod0PrefabPath);
	ClassDB::bind_method(D_METHOD("get_lod0PrefabPath"), &FoliagePrototype::get_lod0PrefabPath);

	ClassDB::bind_method(D_METHOD("set_lod1PrefabPath", "path"), &FoliagePrototype::set_lod1PrefabPath);
	ClassDB::bind_method(D_METHOD("get_lod1PrefabPath"), &FoliagePrototype::get_lod1PrefabPath);

	ClassDB::bind_method(D_METHOD("set_lod2PrefabPath", "path"), &FoliagePrototype::set_lod2PrefabPath);
	ClassDB::bind_method(D_METHOD("get_lod2PrefabPath"), &FoliagePrototype::get_lod2PrefabPath);

	ClassDB::bind_method(D_METHOD("set_lod3PrefabPath", "path"), &FoliagePrototype::set_lod3PrefabPath);
	ClassDB::bind_method(D_METHOD("get_lod3PrefabPath"), &FoliagePrototype::get_lod3PrefabPath);

	ClassDB::bind_method(D_METHOD("set_colliderPath", "path"), &FoliagePrototype::set_colliderPath);
	ClassDB::bind_method(D_METHOD("get_colliderPath"), &FoliagePrototype::get_colliderPath);

	ClassDB::bind_method(D_METHOD("set_is_convex_collider", "is_convex_collider"), &FoliagePrototype::set_is_convex_collider);
	ClassDB::bind_method(D_METHOD("get_is_convex_collider"), &FoliagePrototype::get_is_convex_collider);

	ClassDB::bind_method(D_METHOD("set_lodEndDistance", "distance"), &FoliagePrototype::set_lodEndDistance);
	ClassDB::bind_method(D_METHOD("get_lodEndDistance"), &FoliagePrototype::get_lodEndDistance);

	ClassDB::bind_method(D_METHOD("set_mobileLodEndDistance", "distance"), &FoliagePrototype::set_mobileLodEndDistance);
	ClassDB::bind_method(D_METHOD("get_mobileLodEndDistance"), &FoliagePrototype::get_mobileLodEndDistance);

	ClassDB::bind_method(D_METHOD("set_boxOS", "box"), &FoliagePrototype::set_boxOS);
	ClassDB::bind_method(D_METHOD("get_boxOS"), &FoliagePrototype::get_boxOS);

	ClassDB::bind_method(D_METHOD("set_foliage_position_xz_roots_size", "size"), &FoliagePrototype::set_foliage_position_xz_roots_size);
	ClassDB::bind_method(D_METHOD("get_foliage_position_xz_roots_size"), &FoliagePrototype::get_foliage_position_xz_roots_size);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "guid"), "set_guid", "get_guid");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lod0PrefabPath"), "set_lod0PrefabPath", "get_lod0PrefabPath");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lod1PrefabPath"), "set_lod1PrefabPath", "get_lod1PrefabPath");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lod2PrefabPath"), "set_lod2PrefabPath", "get_lod2PrefabPath");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lod3PrefabPath"), "set_lod3PrefabPath", "get_lod3PrefabPath");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "colliderPath"), "set_colliderPath", "get_colliderPath");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_convex_collider"), "set_is_convex_collider", "get_is_convex_collider");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR4, "lodEndDistance"), "set_lodEndDistance", "get_lodEndDistance");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "mobileLodEndDistance"), "set_mobileLodEndDistance", "get_mobileLodEndDistance");
	ADD_PROPERTY(PropertyInfo(Variant::AABB, "boxOS"), "set_boxOS", "get_boxOS");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "foliage_position_xz_roots_size"), "set_foliage_position_xz_roots_size", "get_foliage_position_xz_roots_size");
}

void FoliagePrototypeAsset::load_imp(Ref<FileAccess> &file, uint32_t version, bool is_big_endian) {
	region.position.x = file->get_float();
	region.position.y = file->get_float();
	region.size.x = file->get_float();
	region.size.y = file->get_float();

	uint32_t foliage_count = file->get_32();
	prototypes.resize(foliage_count);
	// 加載原型信息
	for (uint32_t i = 0; i < foliage_count; i++) {
		Ref<FoliagePrototype> proto = memnew(FoliagePrototype);
		prototypes.write[i] = proto;
		proto->load(file, is_big_endian);
	}

	uint32_t cell_count = file->get_32();
	String cell_file_name;
	FoliageCellPos pos;
	for (uint32_t i = 0; i < cell_count; i++) {
		pos.x = file->get_32();
		pos.z = file->get_32();
		FoliageCellAsset *cell_asset = memnew(FoliageCellAsset);
		cell_asset->set_region_offset(pos.x, pos.z);
		cell_file_name = file->get_utf8_string_buffer();
		cell_asset->load_file(cell_file_name);
		cellAssetConfig.insert(pos.DecodeInt(), cell_asset);
	}
}
void FoliagePrototypeAsset::save_imp(Ref<FileAccess> &file, uint32_t version) {
	file->store_32(region.position.x);
	file->store_32(region.position.y);
	file->store_32(region.size.x);
	file->store_32(region.size.y);
	file->store_32(prototypes.size());
	for (uint32_t i = 0; i < prototypes.size(); i++) {
		prototypes.write[i]->save(file);
	}
	file->store_32(cellAssetConfig.size());

	String cell_file_name;
	for (auto it : cellAssetConfig) {
		Vector2 pos = it.value->get_region_offset();
		file->store_32(pos.x);
		file->store_32(pos.y);
		file->store_utf8_string_buffer(it.value->get_file());

		it.value->save();
	}
}
void FoliagePrototypeAsset::unload_imp() {
	FoliageEngine::get_singleton().remove_prototype(this);
	for (auto it : cellAssetConfig) {
		it.value->clear();
	}
	cellAssetConfig.clear();

	prototypes.clear();
}
void FoliagePrototypeAsset::tick_imp() {
	if (loadState == LoadFinish) {
		for (auto it : cellAssetConfig) {
			it.value->tick();
		}
	}
}
void FoliagePrototypeAsset::on_load_finish() {
	FoliageEngine::get_singleton().add_prototype(this);
}
} //namespace Foliage
