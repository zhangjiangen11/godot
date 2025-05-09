#include "foliage_cell_asset.h"
#include "foliage_engine.h"

namespace Foliage {

// 移除隐藏的实例
void SceneInstanceBlock::remove_hiden_instances() {
	LocalVector<Transform3D> temp_transform = transform;
	LocalVector<Color> temp_color = color;
	LocalVector<Color> temp_curstom_color = curstom_color;

	transform.clear();
	color.clear();
	curstom_color.clear();
	for (uint32_t i = 0; i < transform.size(); i++) {
		if (render_level[i] >= 0) {
			transform.push_back(temp_transform[i]);
			color.push_back(temp_color[i]);
			curstom_color.push_back(temp_curstom_color[i]);
		}
	}
	render_level.resize(transform.size());
}
// 隐藏不显示的实例
void SceneInstanceBlock::hide_instance_by_cell_mask(const Vector3 &p_position_move, const Ref<FoliageCellMask> &p_cell_mask, uint8_t p_visble_value_min, uint8_t p_visble_value_max) {
	if (transform.size() != p_cell_mask->get_data_size()) {
		return;
	}

	for (uint32_t i = 0; i < transform.size(); i++) {
		if (render_level[i] >= 0) {
			Vector3 pos = transform[i].origin - p_position_move;
			int x = pos.x / p_cell_mask->get_width();
			int z = pos.z / p_cell_mask->get_height();
			x = CLAMP(x, 0, int32_t(p_cell_mask->get_width() - 1));
			z = CLAMP(z, 0, int32_t(p_cell_mask->get_height() - 1));

			uint8_t v = p_cell_mask->get_pixel(x, z);
			if (v < p_visble_value_min || v > p_visble_value_max) {
				int index = x + z * p_cell_mask->get_width();
				render_level[index] = -1;
			}
		}
	}
}
void SceneInstanceBlock::rendom_instance_rotation(float p_angle_min, float p_angle_max) {
	for (uint32_t i = 0; i < transform.size(); i++) {
		if (render_level[i] >= 0) {
			transform[i].basis = transform[i].basis.rotated(Vector3(0, 1, 0), p_angle_min + Math::randf() * (p_angle_max - p_angle_min));
		}
	}
}

void SceneInstanceBlock::rendom_instance_scale(float p_scale_min, float p_scale_max) {
	for (uint32_t i = 0; i < transform.size(); i++) {
		if (render_level[i] >= 0) {
			float scale = p_scale_min + Math::randf() * (p_scale_max - p_scale_min);
			transform[i].basis = transform[i].basis.scaled(Vector3(scale, scale, scale));
		}
	}
}
void SceneInstanceBlock::rendom_instance_move(float p_move_distance) {
	for (uint32_t i = 0; i < transform.size(); i++) {
		if (render_level[i] >= 0) {
			float range = p_move_distance * 0.5f;
			float x = Math::randf() * range - range * 0.5f;

			float z = Math::randf() * range - range * 0.5f;

			Vector3 move = Vector3(x, 0, z);
			transform[i].origin += move;
		}
	}
}

void SceneInstanceBlock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_instance_count", "cell_width", "cell_height"), &SceneInstanceBlock::set_instance_count);
	ClassDB::bind_method(D_METHOD("get_instance_count"), &SceneInstanceBlock::get_instance_count);

	ClassDB::bind_method(D_METHOD("set_instance_render_level", "index", "render_level"), &SceneInstanceBlock::set_instance_render_level);
	ClassDB::bind_method(D_METHOD("get_instance_render_level", "index"), &SceneInstanceBlock::get_instance_render_level);

	ClassDB::bind_method(D_METHOD("set_instance_color", "index", "color"), &SceneInstanceBlock::set_instance_color);
	ClassDB::bind_method(D_METHOD("get_instance_color", "index"), &SceneInstanceBlock::get_instance_color);

	ClassDB::bind_method(D_METHOD("set_instance_curstom_color", "index", "color"), &SceneInstanceBlock::set_instance_curstom_color);
	ClassDB::bind_method(D_METHOD("get_instance_curstom_color", "index"), &SceneInstanceBlock::get_instance_curstom_color);

	ClassDB::bind_method(D_METHOD("set_instance_transform", "index", "transform"), &SceneInstanceBlock::set_instance_transform);
	ClassDB::bind_method(D_METHOD("get_instance_transform", "index"), &SceneInstanceBlock::get_instance_transform);

	ClassDB::bind_method(D_METHOD("set_guid", "guid"), &SceneInstanceBlock::set_guid);
	ClassDB::bind_method(D_METHOD("get_guid"), &SceneInstanceBlock::get_guid);

	ClassDB::bind_method(D_METHOD("set_proto_type_index", "proto_type_index"), &SceneInstanceBlock::set_proto_type_index);
	ClassDB::bind_method(D_METHOD("get_proto_type_index"), &SceneInstanceBlock::get_proto_type_index);

	ClassDB::bind_method(D_METHOD("init_xz_position", "start_position", "cell_step_x", "cell_step_z"), &SceneInstanceBlock::init_xz_position);
	ClassDB::bind_method(D_METHOD("remove_hiden_instances"), &SceneInstanceBlock::remove_hiden_instances);
	ClassDB::bind_method(D_METHOD("compute_rotation", "p_index", "p_normal", "p_angle"), &SceneInstanceBlock::compute_rotation);
	ClassDB::bind_method(D_METHOD("hide_instance_by_cell_mask", "pos_move", "p_cell_mask", "p_visble_value_min", "p_visble_value_max"), &SceneInstanceBlock::hide_instance_by_cell_mask);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "guid"), "set_guid", "get_guid");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "proto_type_index"), "set_proto_type_index", "get_proto_type_index");
}

void FoliageCellAsset::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_region_offset", "x", "z"), &FoliageCellAsset::set_region_offset);
	ClassDB::bind_method(D_METHOD("get_region_offset"), &FoliageCellAsset::get_region_offset);

	ClassDB::bind_method(D_METHOD("add_data", "x", "z"), &FoliageCellAsset::add_data);

	ClassDB::bind_method(D_METHOD("create_instance_block", "cell_index", "proto_type_index", "render_level"), &FoliageCellAsset::create_instance_block);
	ClassDB::bind_method(D_METHOD("add_instance_block", "cell_index", "block"), &FoliageCellAsset::add_instance_block);
}

void FoliageCellAsset::load_imp(Ref<FileAccess> &file, uint32_t _version, bool is_big_endian) {
	region_offset.x = file->get_32();
	region_offset.y = file->get_32();

	x = file->get_32();
	z = file->get_32();

	int32_t count = file->get_32();
	cell_data.resize(count);
	for (int i = 0; i < count; i++) {
		cell_data.write[i].load(file, is_big_endian);
	}
}

Ref<SceneInstanceBlock> FoliageCellAsset::CellData::create_instance_block(const FoliageCellPos &cell_position, int proto_type_index, int render_level) {
	if (proto_type_index >= prototypes.size()) {
		return Ref<SceneInstanceBlock>();
	}
	const PrototypeData &prototype = prototypes[proto_type_index];
	Ref<SceneInstanceBlock> block = memnew(SceneInstanceBlock);
	Transform3D transform;
	for (int i = prototype.instanceRange.x; i < prototype.instanceRange.y; i++) {
		const InstanceData &instance = instances[i];
		if (instance.renderLodID <= render_level) {
			instance.create_transform(position, transform);
			transform.origin = instance.p.Decompress(cell_position);
			transform.basis = Basis(instance.r.Decompress(), instance.s.Decompress());
			block->add_instance(transform, instance.color, instance.custom_color);
		}
	}
	block->set_guid(prototype.guid);
	block->set_proto_type_index(proto_type_index);
	return block;
}
void FoliageCellAsset::CellData::add_instance_block(FoliageCellPos &_cellPos, const Ref<SceneInstanceBlock> &block) {
	PrototypeData prototype;
	prototype.guid = block->get_guid();
	prototype.instanceRange.x = instances.size();
	prototype.instanceRange.y = prototype.instanceRange.x + block->get_instance_count();
	prototypes.push_back(prototype);
	instances.resize(prototype.instanceRange.y);

	for (int i = 0; i < block->get_instance_count(); i++) {
		InstanceData &instance = instances.write[prototype.instanceRange.x + (uint64_t)i];
		const Transform3D &transform = block->get_instance_transform(i);
		instance.p = CompressedPosition(_cellPos, transform.origin);
		instance.s = CompressedScaling(transform.basis.get_scale());
		instance.r = CompressedRotation(transform.basis.get_rotation_quaternion());
		instance.color = block->get_instance_color(i);
		instance.custom_color = block->get_instance_curstom_color(i);
		instance.renderGroupID = block->get_instance_render_level(i);
	}
}

/// <summary>
/// 清除数据
/// </summary>
void FoliageCellAsset::unload_imp() {
	cell_data.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FoliageCellMask::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init", "width", "height", "is_bit"), &FoliageCellMask::init);
	ClassDB::bind_method(D_METHOD("set_pixel", "x", "y", "value"), &FoliageCellMask::set_pixel);
	ClassDB::bind_method(D_METHOD("set_rect_pixel", "x", "y", "width", "height", "value"), &FoliageCellMask::set_rect_pixel);
	ClassDB::bind_method(D_METHOD("set_circle_pixel", "x", "y", "radius", "value"), &FoliageCellMask::set_circle_pixel);
	ClassDB::bind_method(D_METHOD("set_form_texture_pixel", "texture", "dest_rect", "source_rect", "image_slot"), &FoliageCellMask::set_form_texture_pixel);
	ClassDB::bind_method(D_METHOD("get_pixel", "x", "y"), &FoliageCellMask::get_pixel);
	ClassDB::bind_method(D_METHOD("get_data_size"), &FoliageCellMask::get_data_size);

	ClassDB::bind_method(D_METHOD("set_data", "data"), &FoliageCellMask::set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &FoliageCellMask::get_data);

	ClassDB::bind_method(D_METHOD("set_width", "width"), &FoliageCellMask::set_width);
	ClassDB::bind_method(D_METHOD("get_width"), &FoliageCellMask::get_width);

	ClassDB::bind_method(D_METHOD("set_height", "height"), &FoliageCellMask::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &FoliageCellMask::get_height);

	ClassDB::bind_method(D_METHOD("set_real_width", "real_width"), &FoliageCellMask::set_real_width);
	ClassDB::bind_method(D_METHOD("get_real_width"), &FoliageCellMask::get_real_width);

	ClassDB::bind_method(D_METHOD("set_is_bit", "is_bit"), &FoliageCellMask::set_is_bit);
	ClassDB::bind_method(D_METHOD("get_is_bit"), &FoliageCellMask::get_is_bit);

	ClassDB::bind_method(D_METHOD("create_sub_mask", "x", "y", "width", "height"), &FoliageCellMask::create_sub_mask);
	ClassDB::bind_method(D_METHOD("apply_mask", "p_x", "p_y", "mask"), &FoliageCellMask::apply_mask);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"), "set_data", "get_data");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "real_width"), "set_real_width", "get_real_width");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_bit"), "set_is_bit", "get_is_bit");
}

void FoliageCellMask::init(uint32_t p_width, uint32_t p_height, bool p_is_bit) {
	width = p_width;
	height = p_height;
	is_bit = p_is_bit;
	// 每一个位存储一个状态
	if (is_bit) {
		if (width % 8 == 0) {
			real_width = width / 8;
		} else {
			real_width = width / 8 + 1;
		}
		data.resize((uint64_t)real_width * height);
		memset(data.ptrw(), 0, (uint64_t)real_width * height);
	} else {
		data.resize(width * height);
		memset(data.ptrw(), 0, width * height);
	}
	if (is_bit) {
		ERR_FAIL_COND(width % 8 != 0);
		ERR_FAIL_COND(p_height % 8 != 0);
	}
}
void FoliageCellMask::set_pixel(int p_x, int p_y, uint8_t p_value) {
	if (p_x < 0 || p_x >= int(width) || p_y < 0 || p_y >= int(height)) {
		return;
	}
	if (!is_bit) {
		data.write[p_x + p_y * width] = p_value;
	} else {
		int index = p_x / 8;
		int offset = p_x % 8;
		uint8_t v = data[index + p_y * (uint64_t)real_width];
		if (p_value == 0) {
			v &= ~(1 << offset);
		} else {
			v |= 1 << offset;
		}
		data.write[index + p_y * (uint64_t)real_width] = v;
	}
}

uint8_t FoliageCellMask::get_pixel(int p_x, int p_y) {
	if (parent.is_valid()) {
		return parent->get_pixel(use_parent_offset.x + p_x, use_parent_offset.y + p_y);
	}
	if (p_x < 0 || p_x >= int(width) || p_y < 0 || p_y >= int(height)) {
		return 0;
	}
	if (!is_bit) {
		return data[p_x + p_y * (uint64_t)width];
	} else {
		int index = p_x / 8;
		int offset = p_x % 8;
		uint8_t v = data[index + p_y * (uint64_t)real_width];
		return (v >> offset) & 1;
	}
}

void FoliageCellMask::set_rect_pixel(int p_x, int p_y, int p_width, int p_height, uint8_t p_value) {
	for (int w = p_x; w < p_x + p_width; w++) {
		for (int h = p_y; h < p_y + p_height; h++) {
			set_pixel(w, h, p_value);
		}
	}
}
void FoliageCellMask::set_circle_pixel(int p_x, int p_y, int p_radius, uint8_t p_value) {
	int square = p_radius * p_radius;
	for (int w = p_x - p_radius; w < p_x + p_radius; w++) {
		for (int h = p_y - p_radius; h < p_y + p_radius; h++) {
			if ((w - p_x) * (w - p_x) + (h - p_y) * (h - p_y) <= square) {
				set_pixel(w, h, p_value);
			}
		}
	}
}
void FoliageCellMask::set_form_texture_pixel(const Ref<Image> &p_texture, const Rect2 &p_dest_rect, const Rect2 &p_src_rect, int image_slot) {
	int dest_start_x = p_dest_rect.position.x * width;
	int dest_start_y = p_dest_rect.position.y * height;
	int src_start_x = p_src_rect.position.x * width;
	int src_start_y = p_src_rect.position.y * height;
	int src_width = p_src_rect.size.x * width;
	int src_height = p_src_rect.size.y * height;
	for (int w = 0; w < src_width; w++) {
		for (int h = 0; h < src_height; h++) {
			int dest_x = dest_start_x + w;
			int dest_y = dest_start_y + h;
			int src_x = src_start_x + w;
			int src_y = src_start_y + h;
			Color c = p_texture->get_pixel(src_x, src_y);
			if (is_bit) {
				if (c[image_slot] > 0) {
					set_pixel(dest_x, dest_y, 1);
				} else {
					set_pixel(dest_x, dest_y, 0);
				}

			} else {
				set_pixel(dest_x, dest_y, c[image_slot] * 255);
			}
		}
	}
}

Ref<FoliageCellMask> FoliageCellMask::create_sub_mask(int p_x, int p_y, int p_width, int p_height) {
	if (is_bit) {
		ERR_FAIL_COND_V(p_x % 8 != 0, Ref<FoliageCellMask>());
		ERR_FAIL_COND_V(p_y % 8 != 0, Ref<FoliageCellMask>());
		ERR_FAIL_COND_V(p_width % 8 != 0, Ref<FoliageCellMask>());
		ERR_FAIL_COND_V(p_height % 8 != 0, Ref<FoliageCellMask>());
	}
	Ref<FoliageCellMask> sub_mask = memnew(FoliageCellMask);
	sub_mask->init(p_width, p_height, is_bit);
	if (is_bit) {
		int64_t src_w_index = p_x / 8;
		int64_t src_h_index = p_y;
		int data_width = p_width / 8;
		for (int w = 0; w < data_width; w++) {
			for (int h = 0; h < p_height / 8; h++) {
				int64_t dest_index = h * data_width + w;
				int64_t src_index = (src_h_index + h) * real_width + src_w_index + w;
				sub_mask->data.write[dest_index] = data[src_index];
			}
		}
	} else {
		for (int w = 0; w < p_width; w++) {
			for (int h = 0; h < p_height; h++) {
				sub_mask->set_pixel(w, h, get_pixel(p_x + w, p_y + h));
			}
		}
	}
	return sub_mask;
}
Ref<FoliageCellMask> FoliageCellMask::create_ref_sub_mask(int p_x, int p_y, int p_width, int p_height) {
	if (parent.is_valid()) {
		ERR_FAIL_V_MSG(Ref<FoliageCellMask>(), "禁止套娃");
	}
	if (is_bit) {
		ERR_FAIL_COND_V(p_x % 8 != 0, Ref<FoliageCellMask>());
		ERR_FAIL_COND_V(p_y % 8 != 0, Ref<FoliageCellMask>());
		ERR_FAIL_COND_V(p_width % 8 != 0, Ref<FoliageCellMask>());
		ERR_FAIL_COND_V(p_height % 8 != 0, Ref<FoliageCellMask>());
	}
	Ref<FoliageCellMask> sub_mask = memnew(FoliageCellMask);
	sub_mask->width = p_width;
	sub_mask->height = p_height;
	sub_mask->is_bit = is_bit;
	if (is_bit) {
		sub_mask->real_width = p_width / 8;
	} else {
		sub_mask->real_width = real_width;
	}
	sub_mask->use_parent_offset = Vector2(p_x, p_y);
	sub_mask->parent = this;
	return sub_mask;
}
void FoliageCellMask::apply_mask(int p_x, int p_y, const Ref<FoliageCellMask> &p_mask) {
	if (p_mask.is_null()) {
		return;
	}
	if (is_bit) {
		ERR_FAIL_COND(p_x % 8 != 0);
		ERR_FAIL_COND(p_y % 8 != 0);
		ERR_FAIL_COND(p_mask->get_width() % 8 != 0);
		ERR_FAIL_COND(p_mask->get_height() % 8 != 0);
	}
	if (is_bit) {
		int64_t src_w_index = p_x / 8;
		int64_t src_h_index = p_y;
		int data_width = p_mask->get_width() / 8;
		for (int w = 0; w < data_width; w++) {
			for (uint64_t h = 0; h < p_mask->get_height() / 8; h++) {
				int64_t dest_index = h * data_width + w;
				int64_t src_index = (src_h_index + h) * p_mask->get_real_width() + src_w_index + w;
				data.write[dest_index] = (data[dest_index] | p_mask->data[src_index]);
			}
		}
	} else {
		for (int x = 0; x < int(p_mask->get_width()); x++) {
			for (int y = 0; y < int(p_mask->get_height()); y++) {
				uint8_t value = p_mask->get_pixel(p_x + x, p_y + y);
				uint8_t va = p_mask->get_pixel(x, y);
				
				value = MAX(value, va);
				set_pixel(p_x + x, p_y + y, value);
			}
		}
	}
}
void FoliageCellMask::scale_instance(const Ref<SceneInstanceBlock> &p_block, float p_sacle_min, float p_scale_max, bool is_invert) {
	if (is_bit) {
		return;
	}
	for (int x = 0; x < int(width); x++) {
		for (int y = 0; y < int(height); y++) {
			if (p_block->get_instance_render_level(y * width + x) == -1) {
				continue;
			}
			uint8_t value = data[y * (uint64_t)width + x];
			if (is_invert) {
				value = 255 - value;
			}
			float fvalue = value / 255.0;
			float scale = fvalue * (p_scale_max - p_sacle_min) + p_sacle_min;
			p_block->set_instance_scale(y * width + x, Vector3(scale, scale, scale));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FoliageHeightMap::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_width", "width"), &FoliageHeightMap::set_width);
	ClassDB::bind_method(D_METHOD("get_width"), &FoliageHeightMap::get_width);

	ClassDB::bind_method(D_METHOD("set_height", "height"), &FoliageHeightMap::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &FoliageHeightMap::get_height);

	ClassDB::bind_method(D_METHOD("set_data", "data"), &FoliageHeightMap::set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &FoliageHeightMap::get_data);

	ClassDB::bind_method(D_METHOD("init", "width", "height"), &FoliageHeightMap::init);
	ClassDB::bind_method(D_METHOD("init_form_image", "width", "height", "image", "rect"), &FoliageHeightMap::init_form_image);
	ClassDB::bind_method(D_METHOD("set_pixel", "x", "y", "value"), &FoliageHeightMap::set_pixel);
	ClassDB::bind_method(D_METHOD("get_pixel", "x", "y"), &FoliageHeightMap::get_pixel);
	ClassDB::bind_method(D_METHOD("hide_instance_by_height_range", "block", "min_height", "max_height", "instance_start_pos", "depend_job"), &FoliageHeightMap::hide_instance_by_height_range);
	ClassDB::bind_method(D_METHOD("hide_instance_by_flatland", "block", "instance_range", "flatland_height", "instance_start_pos", "depend_job"), &FoliageHeightMap::hide_instance_by_flatland);
	ClassDB::bind_method(D_METHOD("sample_height", "u", "v"), &FoliageHeightMap::sample_height);
	ClassDB::bind_method(D_METHOD("update_height", "block", "base_height", "height_range", "instance_start_pos", "depend_job"), &FoliageHeightMap::update_height);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"), "set_data", "get_data");
}
void FoliageHeightMap::init(uint32_t p_width, uint32_t p_height) {
	width = p_width;
	height = p_height;
	data.resize(height * (uint64_t)width);
}
void FoliageHeightMap::init_form_image(uint32_t p_width, uint32_t p_height, const Ref<Image> &p_image, const Rect2i &p_rect) {
	width = p_width;
	height = p_height;
	int start_x = p_rect.position.x;
	int start_y = p_rect.position.y;
	data.resize(height * (uint64_t)width);
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			data.write[y * (uint64_t)width + x] = p_image->get_pixel(start_x + x, start_y + y).r;
		}
	}
}
void FoliageHeightMap::init_form_half_data(uint32_t p_width, uint32_t p_height, const Vector<uint8_t> &p_data) {
	width = p_width;
	height = p_height;

	data.resize(height * width);
	const uint8_t *ptr = p_data.ptr();
	float *dst_ptr = data.ptrw();
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int ofs = (y * width + x) * 2;
			uint16_t r = ((uint16_t *)ptr)[ofs];
			dst_ptr[y * width + x] = Math::half_to_float(r);
		}
	}
}
static void thread_instance_by_height_range(int index, const Ref<FoliageHeightMap> &p_height_map, const Ref<SceneInstanceBlock> &p_block, float p_visble_height_min, float p_visble_height_max, const Vector2 &p_instance_start_pos) {
	if (p_block->get_instance_render_level(index) == -1) {
		return;
	}
	const Transform3D &transform = p_block->get_instance_transform(index);
	float u = (transform.origin.x - p_instance_start_pos.x) / p_height_map->get_width();
	float v = (transform.origin.z - p_instance_start_pos.y) / p_height_map->get_height();
	float _height = p_height_map->sample_height(u, v);
	if (_height < p_visble_height_min || _height > p_visble_height_max) {
		p_block->set_instance_render_level(index, -1);
	}
}
// 隐藏不在高度范围内的实例
Ref<TaskJobHandle> FoliageHeightMap::hide_instance_by_height_range(const Ref<SceneInstanceBlock> &p_block, float p_visble_height_min, float p_visble_height_max, const Vector2 &p_instance_start_pos, const Ref<TaskJobHandle> &depend_task) {
	return WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageHeightMap::hide_instance_by_height_range"),
			callable_mp_static(thread_instance_by_height_range).bind(this, p_block, p_visble_height_min, p_visble_height_max, p_instance_start_pos), p_block->get_instance_count(), 512, depend_task.ptr());
}
static void thread_hide_instance_by_flatland(int index, const Ref<FoliageHeightMap> &p_height_map, const Ref<SceneInstanceBlock> &p_block, float p_instance_range, float p_height_difference, const Vector2 &p_instance_start_pos) {
	if (p_block->get_instance_render_level(index) == -1) {
		return;
	}
	const Transform3D &transform = p_block->get_instance_transform(index);
	float u = (transform.origin.x - p_instance_start_pos.x) / p_height_map->get_width();
	float v = (transform.origin.z - p_instance_start_pos.y) / p_height_map->get_height();

	float start_u = u - p_instance_range;
	float start_v = v - p_instance_range;

	float end_u = u + p_instance_range;
	float end_v = v + p_instance_range;

	float step_u = 1.0f / p_height_map->get_width();
	float step_v = 1.0f / p_height_map->get_height();

	float min_height = 1000000.0f;
	float max_height = -1000000.0f;
	bool is_flat = true;
	for (float x = start_u; x <= end_u; x += step_u) {
		for (float y = start_v; y <= end_v; y += step_v) {
			float height = p_height_map->sample_height(x, y);
			if (height < 1000.0f) {
				continue;
			}
			if (x < 0 || x >= 1.0f || y < 0 || y >= 1.0f) {
				continue;
			}
			if (height < min_height) {
				min_height = height;
			}
			if (height > max_height) {
				max_height = height;
			}
		}
	}
	if (abs(max_height - min_height) > p_height_difference) {
		is_flat = false;
	}
	if (!is_flat) {
		p_block->set_instance_render_level(index, -1);
	}
}
// 隱藏非平地的实例
Ref<TaskJobHandle> FoliageHeightMap::hide_instance_by_flatland(const Ref<SceneInstanceBlock> &p_block, float p_instance_range, float p_height_difference, const Vector2 &p_instance_start_pos, const Ref<TaskJobHandle> &depend_task) {
	return WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageHeightMap::hide_instance_by_flatland"),
			callable_mp_static(thread_hide_instance_by_flatland).bind(this, p_block, p_instance_range, p_height_difference, p_instance_start_pos), p_block->get_instance_count(), 512, depend_task.ptr());
}

static void thread_update_height(int index, const Ref<FoliageHeightMap> &p_height_map, const Ref<SceneInstanceBlock> &p_block, float p_base_height, float p_height_range, const Vector2 &p_instance_start_pos) {
	if (p_block->get_instance_render_level(index) == -1) {
		return;
	}
	Transform3D transform = p_block->get_instance_transform(index);
	float u = (transform.origin.x - p_instance_start_pos.x) / p_height_map->get_width();
	float v = (transform.origin.z - p_instance_start_pos.y) / p_height_map->get_height();
	float height = p_height_map->sample_height(u, v);
	transform.origin.y = height;
	p_block->set_instance_transform(index, transform);
}

Ref<TaskJobHandle> FoliageHeightMap::update_height(const Ref<SceneInstanceBlock> &p_block, float p_base_height, float p_height_range, const Vector2 &p_instance_start_pos, const Ref<TaskJobHandle> &depend_task) {
	return WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageHeightMap::update_height"),
			callable_mp_static(thread_update_height).bind(this, p_block, p_base_height, p_height_range, p_instance_start_pos), p_block->get_instance_count(), 512, depend_task.ptr());
}

Vector3 FoliageHeightMap::get_height_map_normal(int x, int z, float p_scale_height, float stepX, float stepZ) const {
	Vector3 p0(x * stepX, get_pixel(x, z) * p_scale_height, z * stepZ);
	Vector3 sumNormal(0, 0, 0);

	int validTriangleCount = 0;

	// 左上三角形
	if (x > 0 && z > 0) {
		Vector3 pLeft((x - 1) * stepX, get_pixel(x - 1, z) * p_scale_height, z * stepZ);
		Vector3 pTop(x * stepX, get_pixel(x, z - 1) * p_scale_height, (z - 1) * stepZ);
		Vector3 normal = (pLeft - p0).cross(pTop - p0);
		normal.normalize();
		sumNormal = sumNormal + normal;
		validTriangleCount++;
	}

	// 右上三角形
	if (x < width - 1 && z > 0) {
		Vector3 pRight((x + 1) * stepX, get_pixel(x + 1, z) * p_scale_height, z * stepZ);
		Vector3 pTop(x * stepX, get_pixel(x, z - 1) * p_scale_height, (z - 1) * stepZ);
		Vector3 normal = (pTop - p0).cross(pRight - p0);
		normal.normalize();
		sumNormal = sumNormal + normal;
		validTriangleCount++;
	}

	// 右下三角形
	if (x < width - 1 && z < height - 1) {
		Vector3 pRight((x + 1) * stepX, get_pixel(x + 1, z) * p_scale_height, z * stepZ);
		Vector3 pBottom(x * stepX, get_pixel(x, z + 1) * p_scale_height, (z + 1) * stepZ);
		Vector3 normal = (pRight - p0).cross(pBottom - p0);
		normal.normalize();
		sumNormal = sumNormal + normal;
		validTriangleCount++;
	}

	// 左下三角形
	if (x > 0 && z < height - 1) {
		Vector3 pLeft((x - 1) * stepX, get_pixel(x - 1, z) * p_scale_height, z * stepZ);
		Vector3 pBottom(x * stepX, get_pixel(x, z + 1) * p_scale_height, (z + 1) * stepZ);
		Vector3 normal = (pBottom - p0).cross(pLeft - p0);
		normal.normalize();
		sumNormal = sumNormal + normal;
		validTriangleCount++;
	}

	if (validTriangleCount > 0) {
		sumNormal.normalize();
	} else {
		sumNormal = Vector3(0, 1, 0);
	}
	return sumNormal;
}
static float get_height_form_data(const uint8_t *p_data, int p_width, int p_height, int x, int z, float p_scale_height, float stepX, float stepZ) {
	uint16_t r = ((uint16_t *)p_data)[(z * p_width + x) * 2];
	return Math::half_to_float(r) * p_scale_height;
}
Vector3 FoliageHeightMap::get_height_map_normal_form_data(const Vector<uint8_t> &p_data, int width, int height, int x, int z, float p_scale_height, float stepX, float stepZ) {
	Vector3 p0(x * stepX, get_height_form_data(p_data.ptr(), width, height, x, z, p_scale_height, stepX, stepZ), z * stepZ);
	Vector3 sumNormal(0, 0, 0);

	int validTriangleCount = 0;

	// 左上三角形
	if (x > 0 && z > 0) {
		Vector3 pLeft((x - 1) * stepX, get_height_form_data(p_data.ptr(), width, height, x - 1, z, p_scale_height, stepX, stepZ), z * stepZ); //get_pixel(x - 1, z) * p_scale_height, z * stepZ);
		Vector3 pTop(x * stepX, get_height_form_data(p_data.ptr(), width, height, x, z - 1, p_scale_height, stepX, stepZ), (z - 1) * stepZ); //get_pixel(x, z - 1) * p_scale_height, (z - 1) * stepZ);
		Vector3 normal = (pLeft - p0).cross(pTop - p0);
		normal.normalize();
		sumNormal = sumNormal + normal;
		validTriangleCount++;
	}

	// 右上三角形
	if (x < width - 1 && z > 0) {
		Vector3 pRight((x + 1) * stepX, get_height_form_data(p_data.ptr(), width, height, x + 1, z, p_scale_height, stepX, stepZ), z * stepZ); //get_pixel(x + 1, z) * p_scale_height, z * stepZ);
		Vector3 pTop(x * stepX, get_height_form_data(p_data.ptr(), width, height, x, z - 1, p_scale_height, stepX, stepZ), (z - 1) * stepZ); //get_pixel(x, z - 1) * p_scale_height, (z - 1) * stepZ);
		Vector3 normal = (pTop - p0).cross(pRight - p0);
		normal.normalize();
		sumNormal = sumNormal + normal;
		validTriangleCount++;
	}

	// 右下三角形
	if (x < width - 1 && z < height - 1) {
		Vector3 pRight((x + 1) * stepX, get_height_form_data(p_data.ptr(), width, height, x + 1, z, p_scale_height, stepX, stepZ), z * stepZ); //get_pixel(x + 1, z) * p_scale_height, z * stepZ);
		Vector3 pBottom(x * stepX, get_height_form_data(p_data.ptr(), width, height, x, z + 1, p_scale_height, stepX, stepZ), (z + 1) * stepZ); //get_pixel(x, z + 1) * p_scale_height, (z + 1) * stepZ);
		Vector3 normal = (pRight - p0).cross(pBottom - p0);
		normal.normalize();
		sumNormal = sumNormal + normal;
		validTriangleCount++;
	}

	// 左下三角形
	if (x > 0 && z < height - 1) {
		Vector3 pLeft((x - 1) * stepX, get_height_form_data(p_data.ptr(), width, height, x - 1, z, p_scale_height, stepX, stepZ), z * stepZ); //get_pixel(x - 1, z) * p_scale_height, z * stepZ);
		Vector3 pBottom(x * stepX, get_height_form_data(p_data.ptr(), width, height, x, z + 1, p_scale_height, stepX, stepZ), (z + 1) * stepZ); //get_pixel(x, z + 1) * p_scale_height, (z + 1) * stepZ);
		Vector3 normal = (pBottom - p0).cross(pLeft - p0);
		normal.normalize();
		sumNormal = sumNormal + normal;
		validTriangleCount++;
	}

	if (validTriangleCount > 0) {
		sumNormal.normalize();
	} else {
		sumNormal = Vector3(0, 1, 0);
	}
	return sumNormal;
}
float FoliageHeightMap::sample_height(float p_u, float p_v) {
	if (parent.is_valid()) {
		float star_u = use_parent_offset.x / (float)use_parent_offset.x;
		float star_v = use_parent_offset.y / (float)use_parent_offset.y;
		float end_u = (use_parent_offset.x + width) / (float)use_parent_offset.x;
		float end_v = (use_parent_offset.y + height) / (float)use_parent_offset.y;
		p_u = Math::lerp(star_u, end_u, p_u);
		p_v = Math::lerp(star_v, end_v, p_v);
		return parent->sample_height(p_u, p_v);
	}

	p_u = CLAMP(Math::abs(p_u), 0.0, 1.0);
	p_v = CLAMP(Math::abs(p_v), 0.0, 1.0);
	float x = p_u * (width - 1);
	float y = p_v * (height - 1);

	int x1 = int(x);
	int x2 = x1 + 1;
	int y1 = int(y);
	int y2 = y1 + 1;
	// 计算插值系数
	float dx = x - x1;
	float dy = y - y1;

	float c00 = get_pixel(x1, y1);
	float c01 = get_pixel(x1, y2);
	float c10 = get_pixel(x2, y1);
	float c11 = get_pixel(x2, y2);

	float c0 = Math::lerp(c00, c10, dx);
	float c1 = Math::lerp(c01, c11, dx);

	return Math::lerp(c0, c1, dy);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FoliageNormalMap::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init", "width", "height"), &FoliageNormalMap::init);
	ClassDB::bind_method(D_METHOD("init_form_image", "width", "height", "image", "rect"), &FoliageNormalMap::init_form_image);
	ClassDB::bind_method(D_METHOD("init_form_height_image", "width", "height", "image", "rect", "p_scale_height", "stepX", "stepZ"), &FoliageNormalMap::init_form_height_image);
	ClassDB::bind_method(D_METHOD("init_form_height_map", "width", "height", "image", "rect", "p_scale_height", "stepX", "stepZ"), &FoliageNormalMap::init_form_height_map);
	ClassDB::bind_method(D_METHOD("init_form_half_data", "width", "height", "data", "rect", "p_scale_height", "stepX", "stepZ"), &FoliageNormalMap::init_form_half_data);
	ClassDB::bind_method(D_METHOD("set_pixel", "x", "y", "value"), &FoliageNormalMap::set_pixel);
	ClassDB::bind_method(D_METHOD("get_pixel", "x", "y"), &FoliageNormalMap::get_pixel);
	ClassDB::bind_method(D_METHOD("hide_instance_by_slope", "block", "visble_slope_min", "visble_slope_max", "instance_start_pos", "depend_task"), &FoliageNormalMap::hide_instance_by_slope);
	ClassDB::bind_method(D_METHOD("get_xz_normal_map_texture"), &FoliageNormalMap::get_xz_normal_map_texture);

	ClassDB::bind_method(D_METHOD("set_width", "width"), &FoliageNormalMap::set_width);
	ClassDB::bind_method(D_METHOD("get_width"), &FoliageNormalMap::get_width);

	ClassDB::bind_method(D_METHOD("get_height"), &FoliageNormalMap::get_height);
	ClassDB::bind_method(D_METHOD("set_height", "height"), &FoliageNormalMap::set_height);
	ClassDB::bind_method(D_METHOD("sample_normal", "u", "v"), &FoliageNormalMap::sample_normal);

	ClassDB::bind_method(D_METHOD("set_data", "data"), &FoliageNormalMap::set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &FoliageNormalMap::get_data);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "data"), "set_data", "get_data");
}
void FoliageNormalMap::init(int p_width, int p_height) {
	width = p_width;
	height = p_height;
	data.resize(height * (uint64_t)width);
}

static void thread_init_form_image(int index, int64_t dest_ptr, int64_t src_ptr, const Vector2i &p_size, const Rect2i &p_src_rect) {
	int y = index / p_size.x;
	int x = index - y * p_size.x;
	Image *src = (Image *)src_ptr;
	Vector3 *dest = (Vector3 *)dest_ptr;
	Color c = src->get_pixel(p_src_rect.position.x + x, p_src_rect.position.y + y);
	dest[index] = Vector3(c.r * 2.0 - 1.0, c.g * 2.0 - 1.0, c.b * 2.0 - 1.0);
}
void FoliageNormalMap::init_form_image(int p_width, int p_height, const Ref<Image> &p_image, const Rect2i &p_rect) {
	width = p_width;
	height = p_height;
	int start_x = p_rect.position.x;
	int start_y = p_rect.position.y;
	data.resize(height * (uint64_t)width);

	Vector3 *ptr = data.ptrw();
	if (data.size() > 500) {
		Ref<TaskJobHandle> task = WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageNormalMap::init_form_image"),
				callable_mp_static(thread_init_form_image).bind((int64_t)ptr, (int64_t)p_image.ptr(), Vector2i(width, height), p_rect), data.size(), 512, nullptr);
		task->wait_completion();
	} else {
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				Color c = p_image->get_pixel(start_x + x, start_y + y);
				ptr[y * width + x] = Vector3(c.r * 2.0 - 1.0, c.g * 2.0 - 1.0, c.b * 2.0 - 1.0);
			}
		}
	}
}

template <class T>
static void thread_init_form_height_map(int index, int64_t dest_ptr, int64_t src_ptr, const Vector2i &p_size, const Rect2i &p_src_rect, float p_scale_height, float stepX, float stepZ) {
	int y = index / p_size.x;
	int x = index - y * p_size.x;
	T *src = (T *)src_ptr;
	Vector3 *dest = (Vector3 *)dest_ptr;
	dest[index] = src->get_height_map_normal(p_src_rect.position.x + x, p_src_rect.position.y + y, p_scale_height, stepX, stepZ);
}

void FoliageNormalMap::init_form_height_image(int p_width, int p_height, const Ref<Image> &p_image, const Rect2i &p_rect, float p_scale_height, float stepX, float stepZ) {
	width = p_width;
	height = p_height;
	int start_x = p_rect.position.x;
	int start_y = p_rect.position.y;
	data.resize(height * (uint64_t)width);
	Vector3 *ptr = data.ptrw();
	if (data.size() > 500) {
		Ref<TaskJobHandle> task = WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageNormalMap::init_form_height_image"),
				callable_mp_static(thread_init_form_height_map<Image>).bind((int64_t)ptr, (int64_t)p_image.ptr(), Vector2i(width, height), p_rect, (float)p_scale_height, stepX, stepZ), data.size(), 512, nullptr);
		task->wait_completion();

	} else {
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				ptr[y * width + x] = p_image->get_height_map_normal(start_x + x, start_y + y, p_scale_height, stepX, stepZ);
			}
		}
	}
}
void FoliageNormalMap::init_form_height_map(int p_width, int p_height, const Ref<FoliageHeightMap> &p_image, const Rect2i &p_rect, float p_scale_height, float stepX, float stepZ) {
	width = p_width;
	height = p_height;
	int start_x = p_rect.position.x;
	int start_y = p_rect.position.y;
	data.resize(height * (uint64_t)width);
	Vector3 *ptr = data.ptrw();
	if (data.size() > 500) {
		Ref<TaskJobHandle> task = WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageNormalMap::init_form_height_map"),
				callable_mp_static(thread_init_form_height_map<FoliageHeightMap>).bind((int64_t)ptr, (int64_t)p_image.ptr(), Vector2i(width, height), p_rect, (float)p_scale_height, stepX, stepZ), data.size(), 512, nullptr);
		task->wait_completion();

	} else {
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				ptr[y * width + x] = p_image->get_height_map_normal(start_x + x, start_y + y, p_scale_height, stepX, stepZ);
			}
		}
	}
}
static void thread_init_form_half_data(int index, int64_t dest_ptr, int64_t src_ptr, const Vector2i &p_size, const Rect2i &p_src_rect, float p_scale_height, float stepX, float stepZ) {
	int y = index / p_size.x;
	int x = index - y * p_size.x;
	Vector<uint8_t> *src = (Vector<uint8_t> *)src_ptr;
	Vector3 *dest = (Vector3 *)dest_ptr;
	dest[index] = FoliageHeightMap::get_height_map_normal_form_data(*src, p_size.x, p_size.y, p_src_rect.position.x + x, p_src_rect.position.y + y, p_scale_height, stepX, stepZ);
}
void FoliageNormalMap::init_form_half_data(int p_width, int p_height, const Vector<uint8_t> &p_data, const Rect2i &p_rect, float p_scale_height, float stepX, float stepZ) {
	width = p_width;
	height = p_height;
	int start_x = p_rect.position.x;
	int start_y = p_rect.position.y;
	data.resize(height * (uint64_t)width);
	Vector3 *ptr = data.ptrw();
	if (data.size() > 500) {
		Ref<TaskJobHandle> task = WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageNormalMap::init_form_half_data"),
				callable_mp_static(thread_init_form_half_data).bind((int64_t)ptr, (int64_t)p_data.ptr(), Vector2i(width, height), p_rect, (float)p_scale_height, stepX, stepZ), data.size(), 512, nullptr);
		task->wait_completion();
	} else {
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				ptr[y * width + x] = FoliageHeightMap::get_height_map_normal_form_data(p_data, width, height, start_x + x, start_y + y, p_scale_height, stepX, stepZ);
			}
		}
	}
}
void FoliageNormalMap::set_pixel(int p_x, int p_y, Vector3 p_value) {
	if (p_x < 0 || p_x >= width || p_y < 0 || p_y >= height) {
		return;
	}
	data.write[p_x + p_y * (uint64_t)width] = p_value;
}
Vector3 FoliageNormalMap::get_pixel(int p_x, int p_y) {
	if (p_x < 0 || p_x >= width || p_y < 0 || p_y >= height) {
		return Vector3(0, -1000, 0);
	}
	if (parent.is_valid()) {
		return parent->get_pixel(use_parent_offset.x + p_x, use_parent_offset.y + p_y);
	}
	return data[(uint64_t)p_x + p_y * (uint64_t)width];
}
Vector3 FoliageNormalMap::sample_normal(float p_u, float p_v) {
	if (p_u < 0 || p_u > 1.0f || p_v < 0 || p_v > 1.0) {
		return Vector3(0, -1000, 0);
	}
	if (parent.is_valid()) {
		float star_u = use_parent_offset.x / (float)use_parent_offset.x;
		float star_v = use_parent_offset.y / (float)use_parent_offset.y;
		float end_u = (use_parent_offset.x + width) / (float)use_parent_offset.x;
		float end_v = (use_parent_offset.y + height) / (float)use_parent_offset.y;
		p_u = Math::lerp(star_u, end_u, p_u);
		p_v = Math::lerp(star_v, end_v, p_v);
		return parent->sample_normal(p_u, p_v);
	}

	float x = p_u * (width - 1);
	float y = p_v * (height - 1);

	int x1 = int(x);
	int x2 = x1 + 1;
	int y1 = int(y);
	int y2 = y1 + 1;
	// 计算插值系数
	float dx = x - x1;
	float dy = y - y1;

	Vector3 c00 = get_pixel(x1, y1);
	Vector3 c01 = get_pixel(x1, y2);
	Vector3 c10 = get_pixel(x2, y1);
	Vector3 c11 = get_pixel(x2, y2);

	Vector3 c0 = c00.lerp(c10, dx);
	Vector3 c1 = c01.lerp(c11, dx);

	return c0.lerp(c1, dy).normalized();
}

static void thread_hide_instance_by_slope(int index, const Ref<SceneInstanceBlock> &p_block, const Ref<FoliageNormalMap> &p_normal_map, float p_visble_slope_min, float p_visble_slope_max, const Vector2 &p_instance_start_pos) {
	if (p_block->get_instance_render_level(index) == -1) {
		return;
	}
	Transform3D transform = p_block->get_instance_transform(index);
	float u = (transform.origin.x - p_instance_start_pos.x) / p_normal_map->get_width();
	float v = (transform.origin.z - p_instance_start_pos.y) / p_normal_map->get_height();
	Vector3 slope = p_normal_map->sample_normal(u, v);
	if (slope.y < 100) {
		p_block->set_instance_render_level(index, -1);

	} else if (slope.y < p_visble_slope_min || slope.y > p_visble_slope_max) {
		p_block->set_instance_render_level(index, -1);
	}
}

Ref<TaskJobHandle> FoliageNormalMap::hide_instance_by_slope(const Ref<SceneInstanceBlock> &p_block, float p_visble_slope_min, float p_visble_slope_max, const Vector2 &p_instance_start_pos, const Ref<TaskJobHandle> &depend_task) {
	if (data.size() != p_block->get_instance_count()) {
		return depend_task;
	}
	return WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageNormalMap::hide_instance_by_slope"),
			callable_mp_static(thread_hide_instance_by_slope).bind(p_block, this, p_visble_slope_min, p_visble_slope_max), p_block->get_instance_count(), 512, nullptr);
}
static void thread_get_xz_normal_map_texture(int index, int64_t dest_ptr, int64_t src_ptr) {
	Vector3 *src = (Vector3 *)src_ptr;
	uint8_t *dest = (uint8_t *)dest_ptr;
	float t0 = (src[index].x * 0.5 + 0.5) * 255.0;
	float t1 = (src[index].z * 0.5 + 0.5) * 255.0;
	dest[index * 2] = (uint8_t)t0;
	dest[index * 2 + 1] = (uint8_t)t1;
}
Ref<ImageTexture> FoliageNormalMap::get_xz_normal_map_texture() const {
	// 法线贴图的y轴时钟向上,所以不需要存储y轴
	const Vector3 *normal_ptr = data.ptr();

	Vector<uint8_t> image_data;
	image_data.resize((uint64_t)width * height * 2);
	uint8_t *image_ptr = image_data.ptrw();

	if (data.size() > 500) {
		Ref<TaskJobHandle> task = WorkerTaskPool::get_singleton()->add_group_task(SNAME("FoliageNormalMap::get_xz_normal_map_texture"),
				callable_mp_static(thread_get_xz_normal_map_texture).bind((int64_t)image_ptr, (int64_t)normal_ptr), data.size(), 512, nullptr);
		task->wait_completion();
	} else {
		uint64_t ofs = 0;
		float t0, t1;
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				ofs = (y * (uint64_t)width + x) * 2;
				t0 = (normal_ptr[y * width + x].x * 0.5 + 0.5) * 255.0;
				t1 = (normal_ptr[y * width + x].z * 0.5 + 0.5) * 255.0;
				image_ptr[ofs] = (uint8_t)t0;
				image_ptr[ofs + 1] = (uint8_t)t1;
			}
		}
	}
	Ref<Image> image = Image::create_from_data(width, height, false, Image::FORMAT_RG8, image_data);
	Ref<ImageTexture> normal_map = ImageTexture::create_from_image(image);
	return normal_map;
}

} //namespace Foliage
