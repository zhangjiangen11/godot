#pragma once

// #include <godot_cpp/classes/object.hpp>
// #include "core/object/ref_counted.h"
// #include "core/io/image.h"
// #include <godot_cpp/classes/texture.hpp>
// #include "scene/resources/image_texture.h"
// #include <godot_cpp/classes/file_access.hpp>
// #include "scene/resources/image_texture.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "scene/3d/node_3d.h"

#include "mcollision.h"
#include "mconfig.h"

class MTool : public Object {
	GDCLASS(MTool, Object);

private:
	static Node3D *cached_editor_camera;
	static bool editor_plugin_active;

protected:
	static void _bind_methods();

public:
	MTool();
	~MTool();
	static void print_edmsg(const String &msg);
	static Ref<Image> get_r16_image(const String &file_path, const uint64_t width, const uint64_t height, double min_height, double max_height, const bool is_half);
	static void write_r16(const String &file_path, const PackedByteArray &data, double min_height, double max_height);
	static PackedByteArray normalize_rf_data(const PackedByteArray &data, double min_height, double max_height);
	static Node3D *find_editor_camera(bool changed_camera);
	static void enable_editor_plugin();
	static bool is_editor_plugin_active();
	static Ref<MCollision> ray_collision_y_zero_plane(const Vector3 &ray_origin, const Vector3 &ray);

	static PackedInt64Array packed_32_to_64(const PackedInt32Array &p32);
	static PackedInt32Array packed_64_to_32(const PackedInt64Array &p64);

	static AABB get_global_aabb(const AABB &aabb, const Transform3D &global_transform);

	template <typename T>
	static Vector<T> packed_byte_array_to_vector(const PackedByteArray &data) {
		Vector<T> out;
		if (data.size() == 0) {
			return out;
		}
		ERR_FAIL_COND_V(data.size() < 5, out);
		int count = data.decode_u32(0);
		int bcount = sizeof(T) * count;
		ERR_FAIL_COND_V(data.size() != 4 + bcount, out);
		out.resize(count);
		memcpy(out.ptrw(), data.ptr() + 4, bcount);
		return out;
	}

	template <typename T>
	static PackedByteArray vector_to_packed_byte_array(const Vector<T> &data) {
		PackedByteArray out;
		if (data.size() == 0) {
			return out;
		}
		int t_size = sizeof(T) * data.size();
		out.resize(t_size + 4);
		out.encode_u32(0, data.size());
		memcpy(out.ptrw() + 4, data.ptr(), t_size);
		return out;
	}
};
