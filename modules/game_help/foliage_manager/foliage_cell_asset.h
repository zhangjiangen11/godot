#ifndef FOLIAGE_CELL_ASSET_H
#define FOLIAGE_CELL_ASSET_H
#include "core/io/file_access.h"
#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/math_funcs.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/math/vector3.h"
#include "core/object/worker_thread_pool.h"
#include "core/templates/vector.h"

#include "foliage_resource.h"

#include "memory_pool.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/image_texture.h"

namespace Foliage {
class SceneInstanceBlock;
class FoliageCellMask;

class FoliageGlobals {
	/// <summary>
	/// 植被分块大小(米)，地图中的植被按块进行存储
	/// </summary>
public:
	static const int FOLIAGE_CELL_SIZE = 32;

	// 版本号
	static const int VERSION = 1;

	/// <summary>
	/// 植被分块中一个Tile的大小(米)
	/// </summary>
	static const int FOLIAGE_TILE_SIZE = 32;

	static const int FOLIAGE_PAGE_SIZE = FOLIAGE_TILE_SIZE * 16;

	/// <summary>
	/// 植被分块的Tile分辨率, CELL_SIZE / TILE_SIZE
	/// </summary>
	static const int TILE_RESOLUTION_OF_CELL = FOLIAGE_CELL_SIZE / FOLIAGE_TILE_SIZE;

	/// <summary>
	/// 植被分块的Tile数量，TILE_RESOLUTION_OF_CELL * TILE_RESOLUTION_OF_CELL
	/// </summary>
	static const int NUM_TILES_PER_CELL = TILE_RESOLUTION_OF_CELL * TILE_RESOLUTION_OF_CELL;

	/// <summary>
	/// 每米有几个SpawnSpot, 1 / SPAWN_SPOT_SIZE
	/// </summary>
	static const int SPAWN_SPOT_RESOLUTION_PER_METER = 2;

	/// <summary>
	/// 植被分块的出生区域分辨率, SPAWN_SPOT_RESOLUTION_PER_METER * CELL_SIZE
	/// </summary>
	static const int SPAWN_SPOT_RESOLUTION_OF_CELL = SPAWN_SPOT_RESOLUTION_PER_METER * FOLIAGE_CELL_SIZE;

	/// <summary>
	/// 一个Tile的出生区域分辨率，SPAWN_SPOT_RESOLUTION_PER_METER * TILE_SIZE
	/// </summary>
	static const int SPAWN_SPOT_RESOLUTION_OF_TILE = SPAWN_SPOT_RESOLUTION_PER_METER * FOLIAGE_TILE_SIZE;

	/// <summary>
	/// 一个植被分块有多少个出生区域，SPAWN_SPOT_RESOLUTION_OF_CELL * SPAWN_SPOT_RESOLUTION_OF_CELL
	/// </summary>
	static const int NUM_SPAWN_SPOTS_PER_CELL = SPAWN_SPOT_RESOLUTION_OF_CELL * SPAWN_SPOT_RESOLUTION_OF_CELL;

	/// <summary>
	/// 一个植被Tile有多少个出生区域
	/// </summary>
	static const int NUM_SPAWN_SPOTS_PER_TILE = SPAWN_SPOT_RESOLUTION_OF_TILE * SPAWN_SPOT_RESOLUTION_OF_TILE;

	/// <summary>
	/// 两个颜色相等判断允许的误差
	/// </summary>
	static const int COLOR_TOLERANCE = 8;

	/// <summary>
	/// 调色板支持几种颜色，该值不能大于256
	/// </summary>
	static const int NUM_COLORS_PER_PALETTE = 256;

	/// <summary>
	/// 灌木Mesh每顶点所在面法线放在uv4中，
	/// </summary>
	static const int MESH_FACE_NORMAL_CHANNEL = 4;

	/// <summary>
	/// 植被Mesh每顶点随机方向放在uv5中，该值改动后Shader中STAR_VERTEX_RANDOM_DIRECTION也需要做相应修改
	/// </summary>
	static const int MESH_RANDOM_DIR_UV_CHANNEL = 5;

	/// <summary>
	/// 植被Mesh分支基点放到uv6中，该值改动后Shader中STAR_VERTEX_INPUT_BASE_POINT也需要做相应修改
	/// </summary>
	static const int MESH_BASE_POINT_UV_CHANNEL = 6;

	/// <summary>
	/// 植被Mesh分支控制点放到uv7中，该值改动后Shader中STAR_VERTEX_INPUT_CONTROL_POINT也需要做相应修改
	/// </summary>
	static const int MESH_CONTROL_POINT_UV_CHANNEL = 7;

	/// <summary>
	/// 植被与地表混和及植被生成目前支持真3D地图，由于性能考虑，对地面的层数做出限制
	/// </summary>
	static const int MAX_GROUND_LAYERS = 4;

	/// <summary>
	/// 无效高度值
	/// </summary>
	static const int INVALID_HEIGHT = 65536 / 2;

	/// <summary>
	/// 拍照每米占用几个像素，值等于1.0 / SNAPSHOT_PRECISION
	/// </summary>
	static const int INV_SNAPSHOT_PRECISION = 2;

	/// <summary>
	/// 一个Cell的高度图分辨率，值等于CELL_SIZE / CELL_HEIGHT_MAP_PRECISION
	/// </summary>
	static const int CELL_HEIGHT_MAP_RESOLUTION = FOLIAGE_CELL_SIZE * 2;

	static const int LOAD_TAG_NONE = 0;
	static const int LOAD_TAG_LOAD = 1;
	static const int LOAD_TAG_PRE_UNLOAD = 2;

	static int EncodeHeightSegment(int _count, bool _validValue) {
		int _validFlag = _validValue ? 1 : 0;
		int _result = (_count & 0xfffff) | (_validFlag << 31);
		return _result;
	}

	static void DecodeHeightSegment(int _segment, int &_count, bool &_validValue) {
		_count = (_segment & 0xfffff);
		_validValue = (_segment & 0x80000000) > 0;
	}
};
struct FoliageCellPos {
public:
	/// <summary>
	/// 假设整个大世界的范围为80公里，这足够了
	/// </summary>
	static const int MAP_SIZE = 8092 * 10;

	/// <summary>
	/// 一张世界地图的格子分辨率
	/// </summary>
	static const int CELL_RESOLUTION = MAP_SIZE / FoliageGlobals::FOLIAGE_CELL_SIZE;

	/// <summary>
	/// 格子坐标起始位置，从-40公里开始
	/// </summary>
	static const int ORIGIN = -MAP_SIZE / 2 / FoliageGlobals::FOLIAGE_CELL_SIZE;

public:
	int x, z;
	FoliageCellPos() :
			x(0), z(0) {}
	FoliageCellPos(int _x, int _z) {
		x = _x;
		z = _z;
	}
	void Offset(Vector2i &_delta, int _cellSize = FoliageGlobals::FOLIAGE_CELL_SIZE) {
		x += Math::fast_ftoi(_delta.x / _cellSize);
		z += Math::fast_ftoi(_delta.y / _cellSize);
	}
	void Offset(Vector3 &_delta, int _cellSize = FoliageGlobals::FOLIAGE_CELL_SIZE) {
		x += Math::fast_ftoi(_delta.x / _cellSize);
		z += Math::fast_ftoi(_delta.z / _cellSize);
	}
	friend bool operator==(const FoliageCellPos &lhs, const FoliageCellPos &rhs) {
		return (lhs.x == rhs.x && lhs.z == rhs.z);
	}
	friend bool operator!=(const FoliageCellPos &lhs, const FoliageCellPos &rhs) {
		return !(lhs == rhs);
	}

	int64_t DecodeLong() const {
		return (int64_t)x << 32 | (uint32_t)z;
	}

	//maxposition  2^16 =  65535  * FoliageGlobals.CELL_SIZE

	int DecodeInt() const {
		return x << 16 | (uint16_t)z;
	}
	/// <summary>
	/// 把一个long pack 到2个uint里面
	/// </summary>
	/// <param name="uid"></param>
	/// <param name="x 高位"></param>
	/// <param name="y 低位"></param>
	static void EncodeLong(int64_t uid, FoliageCellPos &pos) {
		pos.x = (int)(uid >> 32);
		pos.z = (int)((uid << 32) >> 32);
	}
	static void EncodeInt(int64_t uid, FoliageCellPos &pos) {
		pos.x = (int)(uid >> 16);
		pos.z = (int)((uid << 16) >> 16);
	}
	int GetHashCode() {
		return CELL_RESOLUTION * (z - ORIGIN) + x - ORIGIN;
	}

	Vector3 worldPosition() const {
		return Vector3(FoliageGlobals::FOLIAGE_CELL_SIZE * x, 0.0f, FoliageGlobals::FOLIAGE_CELL_SIZE * z);
	}
	// 頁面的世界位置
	Vector3 pageWorldPosition() const {
		return Vector3(FoliageGlobals::FOLIAGE_PAGE_SIZE * x, 0.0f, FoliageGlobals::FOLIAGE_PAGE_SIZE * z);
	}

	Vector3 centerPosition() {
		return Vector3(FoliageGlobals::FOLIAGE_CELL_SIZE * (x + 0.5f), 0.0f, FoliageGlobals::FOLIAGE_CELL_SIZE * (z + 0.5f));
	}
};
/// <summary>
/// 该类用于存储1..n块的植被数据
/// </summary>
//[PreferBinarySerialization]//不要使用，有概率会导致文件损坏，Unity无法失败
class FoliageCellAsset : public FoliageResource {
	GDCLASS(FoliageCellAsset, FoliageResource)

	static void _bind_methods();
	/// <summary>
	/// 树序列化数据版本号
	/// </summary>
	static const int CURRENT_TREE_ASSET_VERSION = 3;

	/// <summary>
	/// 草序列化数据版本号
	/// </summary>
	static const int CURRENT_GRASS_ASSET_VERSION = 7;

	/// <summary>
	/// 每256*256的范围存储草数据，相当于存储4块草数据
	/// </summary>
	static const int GRASS_STORE_RANGE = 256;

	/// <summary>
	/// 树数据以每个Sector为单位存储
	/// </summary>
	static const int TREE_STORE_RANGE = 512;

public:
	String m_ResourcePath;
	// 版本号
	int m_Version = FoliageGlobals::VERSION;
	/// <summary>
	/// 用short表示相对于格子的x、z坐标，这可提供毫米级的精度(64/32768)
	/// </summary>
	struct CompressedPosition {
		/// <summary>
		/// x、z坐标压缩
		/// </summary>
	public:
		short x = 0;

		short z = 0;

		/// <summary>
		/// y坐标直接存储
		/// </summary>
		float y;
		CompressedPosition() {
		}

		CompressedPosition(const FoliageCellPos &_cellPos, const Vector3 &_uncompressedPos) {
			float _relativeX = _uncompressedPos.x - FoliageGlobals::FOLIAGE_CELL_SIZE * _cellPos.x;
			float _relativeZ = _uncompressedPos.z - FoliageGlobals::FOLIAGE_CELL_SIZE * _cellPos.z;
			x = (int16_t)(_relativeX / FoliageGlobals::FOLIAGE_CELL_SIZE * 32767);
			y = _uncompressedPos.y;
			z = (short)(_relativeZ / FoliageGlobals::FOLIAGE_CELL_SIZE * 32767);
		}

		/// <summary>
		/// 获取解压后的位置
		/// </summary>
		/// <param name="_cellPos"></param>
		/// <returns></returns>
		Vector3 Decompress(const FoliageCellPos &_cellPos) const {
			Vector3 _result;
			_result.x = FoliageGlobals::FOLIAGE_CELL_SIZE * ((float)x / 32767 + _cellPos.x);
			_result.y = y;
			_result.z = FoliageGlobals::FOLIAGE_CELL_SIZE * ((float)z / 32767 + _cellPos.z);
			return _result;
		}
		/// <summary>
		/// 获取解压后的位置
		/// </summary>
		/// <param name="_cellPos"></param>
		/// <returns></returns>
		Vector3 DecompressLocal() const {
			Vector3 _result;
			_result.x = FoliageGlobals::FOLIAGE_CELL_SIZE * ((float)x / 32767);
			_result.y = y;
			_result.z = FoliageGlobals::FOLIAGE_CELL_SIZE * ((float)z / 32767);
			return _result;
		}
	};

	/// <summary>
	/// 用byte表示旋转的值
	/// </summary>
	struct CompressedRotation {
		/// <summary>
		/// 四元数的4个值转为byte存储
		/// </summary>
	public:
		uint8_t x = 127, y = 127, z = 127, w = 255;
		CompressedRotation() {
		}
		CompressedRotation(const Quaternion &_q) {
			x = (uint8_t)((_q.x * 0.5f + 0.5f) * 255.0f);
			y = (uint8_t)((_q.y * 0.5f + 0.5f) * 255.0f);
			z = (uint8_t)((_q.z * 0.5f + 0.5f) * 255.0f);
			w = (uint8_t)((_q.w * 0.5f + 0.5f) * 255.0f);
		}

		/// <summary>
		/// 获取解压后的角度
		/// </summary>
		/// <returns></returns>
		Quaternion Decompress() const {
			return Quaternion(x / 255.0f * 2.0f - 1.0f, y / 255.0f * 2.0f - 1.0f,
					z / 255.0f * 2.0f - 1.0f, w / 255.0f * 2.0f - 1.0f);
		}
	};

	/// <summary>
	/// 用float表示缩放值，不做压缩
	/// </summary>
	struct CompressedScaling {
	public:
		float x = 1.0f, y = 1.0f, z = 1.0f;
		CompressedScaling() {
		}
		CompressedScaling(const Vector3 &_scaling) {
			x = _scaling.x;
			y = _scaling.y;
			z = _scaling.z;
		}

		/// <summary>
		/// 获取解压后的缩放
		/// </summary>
		/// <returns></returns>
		Vector3 Decompress() const {
			return Vector3(x, y, z);
		}
	};

	struct color32 {
		union {
			struct
			{
				uint8_t r;
				uint8_t g;
				uint8_t b;
				uint8_t a;
			};

			uint8_t c[4];

			uint32_t m;
		};
	};
	/// <summary>
	/// 植被实例数据
	/// </summary>
	struct InstanceData {
	public:
		int64_t uid;

		CompressedPosition p;

		/// <summary>
		/// 旋转
		/// </summary>
		CompressedRotation r;

		/// <summary>
		/// 缩放
		/// </summary>
		CompressedScaling s;

		/// <summary>
		/// 实例所在地表颜色,alpha通道保存实例距离地形道路的衰减因子
		/// </summary>
		Color color;
		Color custom_color;

		/// <summary>
		/// 渲染分组ID
		/// </summary>

		int16_t renderGroupID;

		// 渲染的LOD信息
		int16_t renderLodID;

	public:
		InstanceData() :
				uid(0) {
		}
		void load(Ref<FileAccess> &file) {
			uid = file->get_64();

			p.x = file->get_16();
			p.y = file->get_float();
			p.z = file->get_16();

			file->get_buffer((uint8_t *)&r.x, 4);

			s.x = file->get_float();
			s.y = file->get_float();
			s.z = file->get_float();

			color.r = file->get_float();
			color.g = file->get_float();
			color.b = file->get_float();
			color.a = file->get_float();

			custom_color.r = file->get_float();
			custom_color.g = file->get_float();
			custom_color.b = file->get_float();
			custom_color.a = file->get_float();

			renderGroupID = file->get_16();
			renderLodID = file->get_16();
		}

		void create_transform(const FoliageCellPos &p_cell_pos, Transform3D &_transform) const {
			_transform.origin = p.Decompress(p_cell_pos);
			_transform.basis = Basis(r.Decompress(), s.Decompress());
		}

		static int Compare(InstanceData &_left, InstanceData &_right) {
			auto delta_ = _left.p.x - _right.p.x;
			if (delta_ == 0) {
				delta_ = _left.p.z - _right.p.z;
			}
			if (delta_ == 0) {
				delta_ = Math::fast_ftoi(_left.p.y - _right.p.y);
			}
			return delta_;
		}
	};

	/// <summary>
	/// 该结构体用于表示格子中存在的植被原型信息
	/// </summary>
	struct PrototypeData {
		/// <summary>
		/// 原型id，存储的是FoliagePrototype.guid
		/// </summary>
	public:
		String guid;

		/// <summary>
		/// 实例(树、大物体、灌木)在数组中的范围，数组指向<seealso cref="CellData.instances"/>
		/// </summary>
		Vector2i instanceRange;
		/// <summary>
		/// lod 索引值信息，x 是lod0 的实例数量，y 是lod1 的实例数量
		/// </summary>
		Vector2i lodRange;

		/// <summary>
		/// 包围盒
		/// </summary>
		AABB box;
		void load(Ref<FileAccess> &file, bool big_endian) {
			guid = file->get_as_utf8_string();
			instanceRange.x = file->get_16();
			instanceRange.y = file->get_16();
			lodRange.x = file->get_16();
			lodRange.y = file->get_16();
			box.position = Vector3(file->get_float(), file->get_float(), file->get_float());
			box.size = Vector3(file->get_float(), file->get_float(), file->get_float()) - box.position;
		}
	};

	/// <summary>
	/// 该结构体表示植被格子数据
	/// </summary>
	struct CellData {
	public:
		/// <summary>
		/// 植被编辑器会给每个格子赋一个索引值，用做随机种子
		/// </summary>
		int index = 0;

		/// <summary>
		/// Cell位置
		/// </summary>
		FoliageCellPos position;

		/// <summary>
		/// 包围盒，该包围盒包上了该格子中所有的植被
		/// </summary>
		AABB box;

		/// <summary>
		/// 植被实例数据，用于树、大物体、灌木
		/// </summary>
		Vector<InstanceData> instances;

		/// <summary>
		/// Cell中使用植被原型数据
		/// </summary>
		Vector<PrototypeData> prototypes;

	public:
		bool is_load = false;
		MemoryPool::Block *data_block = nullptr;

	public:
		void load(Ref<FileAccess> &file, bool big_endian) {
			clear();
			index = file->get_32();
			position.x = file->get_32();
			position.z = file->get_32();
			box.position.x = file->get_float();
			box.position.y = file->get_float();
			box.position.z = file->get_float();
			box.size.x = file->get_float();
			box.size.y = file->get_float();
			box.size.z = file->get_float();
			box.size -= box.position;

			int32_t count = file->get_32();
			instances.resize(count);
			if (big_endian) {
				for (int i = 0; i < count; i++) {
					instances.write[i].load(file);
				}
			} else {
				file->get_buffer((uint8_t *)instances.ptrw(), sizeof(InstanceData) * count);
			}

			int32_t prototype_count = file->get_32();
			prototypes.resize(prototype_count);
			if (big_endian) {
				for (int i = 0; i < prototype_count; i++) {
					prototypes.write[i].load(file, big_endian);
				}
			} else {
				file->get_buffer((uint8_t *)prototypes.ptrw(), sizeof(PrototypeData) * prototype_count);
			}
		}
		Ref<SceneInstanceBlock> create_instance_block(const FoliageCellPos &cell_position, int proto_type_index, int render_level);
		void add_instance_block(FoliageCellPos &_cellPos, const Ref<SceneInstanceBlock> &block);

		void clear() {
			index = 0;
			position = FoliageCellPos();
			box = AABB();
			instances.clear();
			prototypes.clear();
		}
		/// <summary>
		/// 修正包围盒的值，避免浮点数精度导致的结果波动
		/// </summary>
		void OptimizeBox() {
			auto min_ = box.position;
			min_.x = Math::fast_ftoi(min_.x / 0.125f) * 0.125f;
			min_.y = Math::fast_ftoi(min_.y / 0.125f) * 0.125f;
			min_.z = Math::fast_ftoi(min_.z / 0.125f) * 0.125f;
			auto max_ = box.get_end();
			max_.x = Math::ceil(max_.x / 0.125f) * 0.125f;
			max_.y = Math::ceil(max_.y / 0.125f) * 0.125f;
			max_.z = Math::ceil(max_.z / 0.125f) * 0.125f;

			box = AABB(min_, max_ - min_);
		}
	};

private:
	/// <summary>
	/// 坐标
	/// </summary>
	int x, z;
	Vector2i region_offset;
	/// <summary>
	/// 1..n块的植被数据
	/// </summary>
	Vector<CellData> datas;
	bool is_attach_to_manager = false;

public:
	void set_region_offset(int _x, int _z) {
		region_offset = Vector2i(_x, _z);
	}
	Vector2 get_region_offset() { return region_offset; }

	int32_t add_data(int _x, int _z) {
		CellData data;
		data.position.x = _x;
		data.position.z = _z;
		datas.push_back(data);
		return datas.size() - 1;
	}
	void add_instance_block(int cell_index, const Ref<SceneInstanceBlock> &block) {
		FoliageCellPos cell_position;
		cell_position.x = datas[cell_index].position.x + region_offset.x;
		cell_position.z = datas[cell_index].position.z + region_offset.y;
		datas.write[cell_index].add_instance_block(cell_position, block);
	}
	Ref<SceneInstanceBlock> create_instance_block(int cell_index, int proto_type_index, int render_level) {
		FoliageCellPos cell_position;
		cell_position.x = datas[cell_index].position.x + region_offset.x;
		cell_position.z = datas[cell_index].position.z + region_offset.y;
		return datas.write[cell_index].create_instance_block(cell_position, proto_type_index, render_level);
	}

protected:
	void load_imp(Ref<FileAccess> &file, uint32_t version, bool is_big_endian) override;
	/// <summary>
	/// 清除数据
	/// </summary>
	void unload_imp() override;

public:
};
class SceneInstanceBlock : public RefCounted {
	GDCLASS(SceneInstanceBlock, RefCounted);
	static void _bind_methods();

public:
	void set_instance_count(int p_cell_width, int p_cell_height) {
		int p_instance_count = p_cell_width * p_cell_height;
		cell_width = p_cell_width;
		cell_height = p_cell_height;
		transform.resize(p_instance_count);
		color.resize(p_instance_count);
		curstom_color.resize(p_instance_count);
		render_level.resize(p_instance_count);
		render_level.fill(-1);

		for (int i = 0; i < p_instance_count; i++) {
			transform[i] = Transform3D();
			color[i] = Color(1, 1, 1, 1);
			curstom_color[i] = Color(1, 1, 1, 1);
		}
	}
	int get_instance_count() const { return transform.size(); }

	const LocalVector<Transform3D> &get_transform() const { return transform; }
	const LocalVector<Color> &get_color() const { return color; }
	const LocalVector<Color> &get_curstom_color() const { return curstom_color; }

	void add_instance(const Transform3D &p_transform, const Color &p_color, const Color &p_curstom_color, int8_t p_render_level = 0) {
		transform.push_back(p_transform);
		color.push_back(p_color);
		curstom_color.push_back(p_curstom_color);
		render_level.push_back(p_render_level);
		last_index = -1;
	}

	void set_instance_render_level(int p_index, int8_t p_visible) { render_level[p_index] = p_visible; }
	int8_t get_instance_render_level(int p_index) const { return render_level[p_index]; }

	void set_instance_color(int p_index, const Color &p_color) {
		color[p_index] = p_color;
		last_index = -1;
	}
	const Color &get_instance_color(int p_index) const { return color[p_index]; }

	void set_instance_curstom_color(int p_index, const Color &p_curstom_color) { curstom_color[p_index] = p_curstom_color; }
	const Color &get_instance_curstom_color(int p_index) const { return curstom_color[p_index]; }

	void set_instance_transform(int p_index, const Transform3D &p_transform) {
		transform[p_index] = p_transform;
		last_index = -1;
	}
	const Transform3D &get_instance_transform(int p_index) const { return transform[p_index]; }

	void set_instance_scale(int p_index, const Vector3 &p_scale) {
		transform[p_index].basis.scale(p_scale);
		last_index = -1;
	}

	void set_guid(const String &p_guid) { guid = p_guid; }
	const String &get_guid() const { return guid; }

	void set_proto_type_index(int p_proto_type_index) { proto_type_index = p_proto_type_index; }
	int get_proto_type_index() const { return proto_type_index; }

	// 移除隐藏的实例
	void remove_hiden_instances();

	void compute_rotation(int p_index, const Vector3 &p_normal, float p_angle) {
		Basis &basis = transform[p_index].basis;
		basis.rotate(Vector3(0, 1, 0), p_angle);
		basis.rotate_to_align(Vector3(0, 1, 0), p_normal);
		last_index = -1;
	}
	void compute_rotation_align_normal(int p_index, const Vector3 &p_normal) {
		Basis &basis = transform[p_index].basis;
		basis.rotate_to_align(Vector3(0, 1, 0), p_normal);
		last_index = -1;
	}
	// 隐藏不显示的实例
	void hide_instance_by_cell_mask(const Vector3 &p_position_move, const Ref<FoliageCellMask> &p_cell_mask, uint8_t p_visble_value_min, uint8_t p_visble_value_max);

	void rendom_instance_rotation(float p_angle_min, float p_angle_max);
	void rendom_instance_scale(float p_scale_min, float p_scale_max);

	void rendom_instance_move(float p_move_distance);

	int get_cell_width() const { return cell_width; }
	int get_cell_height() const { return cell_height; }

	void init_xz_position(const Vector2 &p_start_position, float cell_step_x, float cell_step_z) {
		for (int w = 0; w < cell_width; w++) {
			for (int h = 0; h < cell_height; h++) {
				Transform3D trans;
				trans.origin = Vector3(w * cell_step_x, 0, h * cell_step_z);
				transform[h * cell_width + w] = trans;
			}
		}
	}
	int last_index = -1;
	LocalVector<Transform3D> transform;
	LocalVector<Color> color;
	LocalVector<Color> curstom_color;
	// -1 为隐藏,>=0 为显示,0-2 为显示等级
	LocalVector<int8_t> render_level;

	int cell_width = 0;
	int cell_height = 0;

	String guid;
	int proto_type_index = -1;
};

class FoliageCellMask : public RefCounted {
	GDCLASS(FoliageCellMask, RefCounted);
	static void _bind_methods();

public:
	void init(int p_width, int p_height, bool p_is_bit);
	void set_pixel(int p_x, int p_y, uint8_t p_value);
	// 矩形
	void set_rect_pixel(int p_x, int p_y, int p_width, int p_height, uint8_t p_value);

	// 圆环
	void set_circle_pixel(int p_x, int p_y, int p_radius, uint8_t p_value);

	void set_form_texture_pixel(const Ref<Image> &p_texture, const Rect2 &p_dest_rect, const Rect2 &p_src_rect, int image_slot);

	uint8_t get_pixel(int p_x, int p_y);

	void set_data(const Vector<uint8_t> &p_data) { data = p_data; }
	const Vector<uint8_t> &get_data() { return data; }

	void set_width(int p_width) { width = p_width; }
	void set_height(int p_height) { height = p_height; }

	int get_width() { return width; }
	int get_height() { return height; }

	void set_real_width(int p_real_width) { real_width = p_real_width; }
	int get_real_width() { return real_width; }

	void set_is_bit(bool p_is_bit) { is_bit = p_is_bit; }
	bool get_is_bit() { return is_bit; }

	int get_data_size() { return width * height; }

	void scale_instance(const Ref<SceneInstanceBlock> &p_block, float p_sacle_min, float p_scale_max, bool is_invert = false);

private:
	Vector<uint8_t> data;
	int width = 0;
	int height = 0;

	int real_width = 0;

	bool is_bit = false;
	Ref<FoliageCellMask> parent;
	Vector2i use_parent_offset;
};

class FoliageHeightMap : public RefCounted {
	GDCLASS(FoliageHeightMap, RefCounted);
	static void _bind_methods();

public:
	void init(int p_width, int p_height);
	void init_form_image(int p_width, int p_height, const Ref<Image> &p_image, const Rect2i &p_rect);
	void init_form_half_data(int p_width, int p_height, const Vector<uint8_t> &p_data);
	_FORCE_INLINE_ void set_pixel(int p_x, int p_y, float p_value) {
		if (p_x < 0 || p_x >= width || p_y < 0 || p_y >= height) {
			return;
		}
		uint64_t index = p_x + p_y * width;
		data.write[index] = p_value;
	}
	_FORCE_INLINE_ float get_pixel(int p_x, int p_y) const {
		if (p_x < 0 || p_x >= width || p_y < 0 || p_y >= height) {
			return 0;
		}
		uint64_t index = p_x + p_y * width;
		return data[index];
	}
	int get_width() { return width; }
	int get_height() { return height; }
	void set_width(int p_width) { width = p_width; }
	void set_height(int p_height) { height = p_height; }
	void set_data(const Vector<float> &p_data) { data = p_data; }
	const Vector<float> &get_data() { return data; }
	// 隐藏不在高度范围内的实例
	Ref<TaskJobHandle> hide_instance_by_height_range(const Ref<SceneInstanceBlock> &p_block, float p_visble_height_min, float p_visble_height_max, const Vector2 &p_instance_start_pos, const Ref<TaskJobHandle> &depend_task);
	// 隱藏非平地的实例
	Ref<TaskJobHandle> hide_instance_by_flatland(const Ref<SceneInstanceBlock> &p_block, float p_instance_range, float p_height_difference, const Vector2 &p_instance_start_pos, const Ref<TaskJobHandle> &depend_task);

	Ref<TaskJobHandle> update_height(const Ref<SceneInstanceBlock> &p_block, float p_base_height, float p_height_range, const Vector2 &p_instance_start_pos, const Ref<TaskJobHandle> &depend_task);

	Vector3 get_height_map_normal(int p_x, int p_y, float p_scale_height, float stepX, float stepZ) const;
	static Vector3 get_height_map_normal_form_data(const Vector<uint8_t> &p_data, int p_width, int p_height, int p_x, int p_y, float p_scale_height, float stepX, float stepZ);
	float sample_height(float u, float v);

private:
	int width = 0;
	int height = 0;
	Vector<float> data;
	Ref<FoliageHeightMap> parent;
	Vector2i use_parent_offset;
};
class FoliageNormalMap : public RefCounted {
	GDCLASS(FoliageNormalMap, RefCounted);
	static void _bind_methods();

public:
	int get_width() { return width; }
	int get_height() { return height; }
	void set_width(int p_width) { width = p_width; }
	void set_height(int p_height) { height = p_height; }

	void set_data(const Vector<Vector3> &p_data) { data = p_data; }
	const Vector<Vector3> &get_data() { return data; }

	void init(int p_width, int p_height);
	void init_form_image(int p_width, int p_height, const Ref<Image> &p_image, const Rect2i &p_rect);
	void init_form_height_image(int p_width, int p_height, const Ref<Image> &p_image, const Rect2i &p_rect, float p_scale_height, float stepX, float stepZ);
	void init_form_height_map(int p_width, int p_height, const Ref<FoliageHeightMap> &p_image, const Rect2i &p_rect, float p_scale_height, float stepX, float stepZ);
	void init_form_half_data(int p_width, int p_height, const Vector<uint8_t> &p_data, const Rect2i &p_rect, float p_scale_height, float stepX, float stepZ);
	void set_pixel(int p_x, int p_y, Vector3 p_value);
	Vector3 get_pixel(int p_x, int p_y);
	Vector3 sample_normal(float u, float v);
	// 通过坡度隐藏实例
	Ref<TaskJobHandle> hide_instance_by_slope(const Ref<SceneInstanceBlock> &p_block, float p_visble_slope_min, float p_visble_slope_max, const Vector2 &p_instance_start_pos, const Ref<TaskJobHandle> &depend_task);
	Ref<ImageTexture> get_xz_normal_map_texture() const;

public:
	int width = 0;
	int height = 0;
	Vector<Vector3> data;
	Ref<FoliageNormalMap> parent;
	Vector2i use_parent_offset;
};

} //namespace Foliage
#endif
