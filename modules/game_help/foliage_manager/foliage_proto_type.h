#ifndef FOLIAGE_PROTO_TYPE_H
#define FOLIAGE_PROTO_TYPE_H

#include "core/io/file_access.h"
#include "core/math/aabb.h"
#include "core/math/math_funcs.h"
#include "core/object/worker_thread_pool.h"

#include "foliage_cell_asset.h"
#include "foliage_resource.h"

namespace Foliage {
class FoliagePrototype : public RefCounted {
	GDCLASS(FoliagePrototype, RefCounted);
	static void _bind_methods();

public:
	enum EHIZCullAccuracy {
		automatic = 0,
		low = 1,
		medium = 2,
		high = 3,
	};

public:
	/// <summary>
	/// prefab代理体的guid
	/// </summary>
	String guid;

	/// <summary>
	/// 名称，方便调试
	/// </summary>
	String name;

	/// <summary>
	/// HIZ剔除精度
	/// </summary>
	EHIZCullAccuracy hizCullAccuracy;

	/// <summary>
	/// 3个LOD等级的Prefab文件名
	/// </summary>
	String lod0PrefabPath, lod1PrefabPath, lod2PrefabPath, lod3PrefabPath;
	String colliderPath;
	bool is_convex_collider;

	/// <summary>
	/// 8个LOD等级最远渲染距离
	/// </summary>
	float lod0EndDistance = 32, lod1EndDistance = 96, lod2EndDistance = 192, lod3EndDistance = 256;
	/// <summary>
	/// 移动端3个LOD等级最远渲染距离
	/// </summary>
	float mobileLod0EndDistance = 32, mobileLod1EndDistance = 96, mobileLod2EndDistance = 192;
	// 4-8 加载距离

	/// <summary>
	/// 3个Lod层级密度显示 3个LOD密度，花类植被lod2密度单独控制
	/// </summary>
	float lod0Density = 1.0f, lod1Density = 1.0f, lod2Density = 1.0f;

	// 移动端阴影渲染最大lod层级
	int mobileShadowRenderLevel = 3;
	// PC端阴影渲染最大lod层级
	int pcShadowRenderLevel = 3;

	// 移动端加载距离缩放
	float mobileLoadScale = 0.75f;

	/// <summary>
	/// 是否启用lod1、lod2
	/// </summary>
	bool lod1Enabled, lod2Enabled, lod3Enabled;

	Vector4 lodEndDistance() {
		return Vector4(lod0EndDistance, lod1EndDistance, lod2EndDistance, lod3EndDistance);
	}

	/// <summary>
	/// 与地表的混和级别
	/// </summary>
	//public int groundBlendLevel = 4;

	/// <summary>
	/// Mesh对象空间包围盒
	/// </summary>
	AABB boxOS;

	// 根部大小
	Vector3 foliage_position_xz_roots_size = Vector3(0, 0, 1);

public:
	int protypeId = 0;
	int refCount = 0;
	// 是否正在被使用
	bool _isUse = false;

public:
	void set_guid(const String &p_guid) {
		guid = p_guid;
	}
	const String &get_guid() const {
		return guid;
	}

	void set_name(const String &p_name) {
		name = p_name;
	}
	const String &get_name() const {
		return name;
	}

	void set_lod0PrefabPath(const String &path) {
		lod0PrefabPath = path;
	}
	const String &get_lod0PrefabPath() const {
		return lod0PrefabPath;
	}

	void set_lod1PrefabPath(const String &path) {
		lod1PrefabPath = path;
	}
	const String &get_lod1PrefabPath() const {
		return lod1PrefabPath;
	}

	void set_lod2PrefabPath(const String &path) {
		lod2PrefabPath = path;
	}
	const String &get_lod2PrefabPath() const {
		return lod2PrefabPath;
	}

	void set_lod3PrefabPath(const String &path) {
		lod3PrefabPath = path;
	}
	const String &get_lod3PrefabPath() const {
		return lod3PrefabPath;
	}

	void set_colliderPath(const String &path) {
		colliderPath = path;
	}
	const String &get_colliderPath() const {
		return colliderPath;
	}

	void set_is_convex_collider(bool p_is_convex_collider) {
		is_convex_collider = p_is_convex_collider;
	}

	bool get_is_convex_collider() const {
		return is_convex_collider;
	}

	void set_lodEndDistance(const Vector4 &distance) {
		lod0EndDistance = distance.x;
		lod1EndDistance = distance.y;
		lod2EndDistance = distance.z;
		lod3EndDistance = distance.w;
	}

	Vector4 get_lodEndDistance() {
		return Vector4(lod0EndDistance, lod1EndDistance, lod2EndDistance, lod3EndDistance);
	}

	void set_mobileLodEndDistance(const Vector3 &distance) {
		mobileLod0EndDistance = distance.x;
		mobileLod1EndDistance = distance.y;
		mobileLod2EndDistance = distance.z;
	}

	Vector3 get_mobileLodEndDistance() {
		return Vector3(mobileLod0EndDistance, mobileLod1EndDistance, mobileLod2EndDistance);
	}

	void set_boxOS(const AABB &p_boxOS) {
		boxOS = p_boxOS;
	}

	const AABB &get_boxOS() const {
		return boxOS;
	}

	void set_foliage_position_xz_roots_size(const Vector3 &p_foliage_position_xz_roots_size) {
		foliage_position_xz_roots_size = p_foliage_position_xz_roots_size;
	}

	const Vector3 &get_foliage_position_xz_roots_size() const {
		return foliage_position_xz_roots_size;
	}

public:
	FoliagePrototype() {
	}
	void reset_use() {
		_isUse = false;
	}
	void set_use() {
		_isUse = true;
	}
	bool is_use() {
		return _isUse;
	}
	void load(Ref<FileAccess> &file, bool big_endian);

	void save(Ref<FileAccess> &file);
};
// 原型资源
class FoliagePrototypeAsset : public FoliageResource {
	GDCLASS(FoliagePrototypeAsset, FoliageResource)

	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_region", "region"), &FoliagePrototypeAsset::set_region);
		ClassDB::bind_method(D_METHOD("get_region"), &FoliagePrototypeAsset::get_region);
		ClassDB::bind_method(D_METHOD("add_prototype", "prototype"), &FoliagePrototypeAsset::add_prototype);
		ClassDB::bind_method(D_METHOD("get_prototype", "index"), &FoliagePrototypeAsset::get_prototype);

		ADD_PROPERTY(PropertyInfo(Variant::RECT2, "region"), "set_region", "get_region");
	}

public:
	void set_region(const Rect2 &p_region) {
		region = p_region;
	}
	const Rect2 &get_region() {
		return region;
	}
	void add_prototype(const Ref<FoliagePrototype> &p_prototype) {
		prototypes.push_back(p_prototype);
	}
	Ref<FoliagePrototype> get_prototype(int index) {
		return prototypes[index];
	}

public:
	FoliagePrototypeAsset() {
	}
	void load_imp(Ref<FileAccess> &file, uint32_t version, bool is_big_endian) override;
	virtual void save_imp(Ref<FileAccess> &file, uint32_t version) override;
	virtual void on_load_finish() override;
	virtual void unload_imp() override;
	virtual void tick_imp() override;

private:
	Rect2 region;
	String file_name = "foliage_prototype";
	Vector<Ref<FoliagePrototype>> prototypes;
	HashMap<int, Ref<FoliageCellAsset>> cellAssetConfig;
};
} //namespace Foliage
#endif
