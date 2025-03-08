#include "foliage_cell_asset.h"
#include "foliage_engine.h"


namespace Foliage
{

    
		  // 移除隐藏的实例
    void SceneInstanceBlock::remove_hiden_instances() {
        int index = 0;
        
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
    void SceneInstanceBlock::hide_instance_by_cell_mask(const Vector3& p_position_move,const Ref<FoliageCellMask>& p_cell_mask,uint8_t p_visble_value_min,uint8_t p_visble_value_max) {
        if (transform.size() != p_cell_mask->get_data_size()) {
            return;
        }

        for(uint32_t i = 0; i < transform.size(); i++) {
            if (render_level[i] >= 0) {
                Vector3 pos = transform[i].origin - p_position_move;
                int x = pos.x / p_cell_mask->get_width();
                int z = pos.z / p_cell_mask->get_height();
                x = CLAMP(x, 0, p_cell_mask->get_width() - 1);
                z = CLAMP(z, 0, p_cell_mask->get_height() - 1);

                uint8_t v = p_cell_mask->get_pixel(x, z);
                if (v < p_visble_value_min || v > p_visble_value_max) {
                    int index = x + z * p_cell_mask->get_width();
                    render_level[index] = -1;
                }
            }
        }
    }
    void SceneInstanceBlock::rendom_instance_rotation(float p_angle_min,float p_angle_max) {

        for(uint32_t i = 0; i < transform.size(); i++) {
            if (render_level[i] >= 0) {
                transform[i].basis = transform[i].basis.rotated(Vector3(0, 1, 0), p_angle_min + Math::randf() * (p_angle_max - p_angle_min)) ;
            }
        }
    }
    
	void SceneInstanceBlock::rendom_instance_scale(float p_scale_min,float p_scale_max) {

        for(uint32_t i = 0; i < transform.size(); i++) {
            if (render_level[i] >= 0) {
                float scale = p_scale_min + Math::randf() * (p_scale_max - p_scale_min);
                transform[i].basis = transform[i].basis.scaled(Vector3(scale, scale, scale));
            }
        }
    }
    void SceneInstanceBlock::rendom_instance_move(float p_move_distance) {

        for(uint32_t i = 0; i < transform.size(); i++) {
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

        ClassDB::bind_method(D_METHOD("init_xz_position","start_position","cell_step_x","cell_step_z"), &SceneInstanceBlock::init_xz_position);
        ClassDB::bind_method(D_METHOD("remove_hiden_instances"), &SceneInstanceBlock::remove_hiden_instances);
        ClassDB::bind_method(D_METHOD("compute_rotation","p_index","p_normal","p_angle"), &SceneInstanceBlock::compute_rotation);
        ClassDB::bind_method(D_METHOD("hide_instance_by_cell_mask","p_cell_mask","p_visble_value_min","p_visble_value_max"), &SceneInstanceBlock::hide_instance_by_cell_mask);

        ADD_PROPERTY(PropertyInfo(Variant::INT, "guid"), "set_guid", "get_guid");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "proto_type_index"), "set_proto_type_index", "get_proto_type_index");


    }



    void FoliageCellAsset::_bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_region_offset", "x", "z"), &FoliageCellAsset::set_region_offset);
        ClassDB::bind_method(D_METHOD("get_region_offset"), &FoliageCellAsset::get_region_offset);

        ClassDB::bind_method(D_METHOD("add_data", "x", "z"), &FoliageCellAsset::add_data);

        ClassDB::bind_method(D_METHOD("create_instance_block", "cell_index", "proto_type_index","render_level"), &FoliageCellAsset::create_instance_block);
        ClassDB::bind_method(D_METHOD("add_instance_block", "cell_index", "block"), &FoliageCellAsset::add_instance_block);
    }

    void FoliageCellAsset::load_imp(Ref<FileAccess> & file,uint32_t _version,bool is_big_endian)
    {

        region_offset.x = file->get_32();
        region_offset.y = file->get_32();

        x = file->get_32();
        z = file->get_32();

        int32_t count = file->get_32();
        datas.resize(count);
        for(int i = 0;i < count;i++)
        {
            datas.write[i].load(file,is_big_endian);
        }
    }

    Ref<SceneInstanceBlock> FoliageCellAsset::CellData::create_instance_block(const FoliageCellPos& cell_position, int proto_type_index,int render_level) {
        if(proto_type_index >= prototypes.size())
        {
            return Ref<SceneInstanceBlock>();
        }
        const PrototypeData& prototype = prototypes[proto_type_index];
        Ref<SceneInstanceBlock> block = memnew(SceneInstanceBlock);
        Transform3D transform;
        for(int i = prototype.instanceRange.x; i < prototype.instanceRange.y; i++)
        {
            const InstanceData& instance = instances[i];
            if(instance.renderLodID <= render_level) {
                instance.create_transform(position,transform);
                transform.origin = instance.p.Decompress(cell_position);
                transform.basis = Basis(instance.r.Decompress(), instance.s.Decompress());
                block->add_instance(transform,instance.color,instance.custom_color);
            }
        }
        block->set_guid(prototype.guid);
        block->set_proto_type_index(proto_type_index);
        return block;
    }
    void FoliageCellAsset::CellData::add_instance_block(FoliageCellPos& _cellPos,const Ref<SceneInstanceBlock>& block) {
        PrototypeData prototype;
        prototype.guid = block->get_guid();
        prototype.instanceRange.x = instances.size();
        prototype.instanceRange.y = prototype.instanceRange.x + block->get_instance_count();
        prototypes.push_back(prototype);
        instances.resize(prototype.instanceRange.y);
        
        for(int i = 0; i < block->get_instance_count(); i++) {
            
            InstanceData& instance = instances.write[prototype.instanceRange.x + i];
            const Transform3D& transform = block->get_instance_transform(i);
            instance.p = CompressedPosition(_cellPos,transform.origin);
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
    void FoliageCellAsset::unload_imp()
    {
        datas.clear();
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    void FoliageCellMask::_bind_methods() {
        ClassDB::bind_method(D_METHOD("init", "width", "height","is_bit"), &FoliageCellMask::init);
        ClassDB::bind_method(D_METHOD("set_pixel", "x", "y", "value"), &FoliageCellMask::set_pixel);
        ClassDB::bind_method(D_METHOD("set_rect_pixel", "x", "y", "width", "height", "value"), &FoliageCellMask::set_rect_pixel);
        ClassDB::bind_method(D_METHOD("set_circle_pixel", "x", "y", "radius", "value"), &FoliageCellMask::set_circle_pixel);
        ClassDB::bind_method(D_METHOD("set_form_texture_pixel","texture","dest_rect","source_rect","image_slot"), &FoliageCellMask::set_form_texture_pixel);
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


        ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"), "set_data", "get_data");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "set_width", "get_width");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "set_height", "get_height");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "real_width"), "set_real_width", "get_real_width");
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_bit"), "set_is_bit", "get_is_bit");
    }


    void FoliageCellMask::init(int p_width, int p_height,bool p_is_bit) {
        width = p_width;
        height = p_height;
        is_bit = p_is_bit;
        // 每一个位存储一个状态
        if(is_bit) {
            if(width % 32 == 0) {
                real_width = width / 32;
            }
            else {
                real_width = width / 32 + 1;
            }
            data.resize(real_width * height);
            memset(data.ptrw(), 0, real_width * height);
        }
        else {
            data.resize(width * height);
            memset(data.ptrw(), 0, width * height);
        }
        
    }
    void FoliageCellMask::set_pixel(int p_x, int p_y, uint8_t p_value) {
        if(p_x < 0 || p_x >= width || p_y < 0 || p_y >= height) {
            return;
        }
        if(!is_bit) {
            data.write[p_x + p_y * width] = p_value;
        }
        else {
            int index = p_x / 32;
            int offset = p_x % 32;
            uint8_t v = data[index + p_y * real_width];
            if(p_value == 0) {
                v &= ~(1 << offset);
            }
            else {
                v |= 1 << offset;
            }
            data.write[index + p_y * real_width] = v;
        }
    }

    uint8_t FoliageCellMask::get_pixel(int p_x, int p_y) {
        if(p_x < 0 || p_x >= width || p_y < 0 || p_y >= height) {
            return 0;
        }
        if(!is_bit) {
            return data[p_x + p_y * width];
        }
        else {
            int index = p_x / 32;
            int offset = p_x % 32;
            uint8_t v = data[index + p_y * real_width];
            return (v >> offset) & 1;
        }
    }

    
    void FoliageCellMask::set_rect_pixel(int p_x, int p_y, int p_width, int p_height, uint8_t p_value) {
        for(int w = p_x; w < p_x + p_width; w++) {
            for(int h = p_y; h < p_y + p_height; h++) {
                set_pixel(w, h, p_value);
            }
        }
    }
    void FoliageCellMask::set_circle_pixel(int p_x, int p_y, int p_radius, uint8_t p_value) {
        int square = p_radius * p_radius;
        for(int w = p_x - p_radius; w < p_x + p_radius; w++) {
            for(int h = p_y - p_radius; h < p_y + p_radius; h++) {
                if((w - p_x) * (w - p_x) + (h - p_y) * (h - p_y) <= square) {
                    set_pixel(w, h, p_value);
                }
            }
        }
    }
    void FoliageCellMask::set_form_texture_pixel(const Ref<Image>& p_texture,const Rect2& p_dest_rect,const Rect2& p_src_rect,int image_slot) {

        int dest_start_x = p_dest_rect.position.x * width;
        int dest_start_y = p_dest_rect.position.y * height;
        int dest_width = p_dest_rect.size.x * width;
        int dest_height = p_dest_rect.size.y * height;
        int src_start_x = p_src_rect.position.x * width;
        int src_start_y = p_src_rect.position.y * height;
        int src_width = p_src_rect.size.x * width;
        int src_height = p_src_rect.size.y * height;
        for(int w = 0; w < src_width; w++) {
            for(int h = 0; h < src_height; h++) {
                int dest_x = dest_start_x + w;
                int dest_y = dest_start_y + h;
                int src_x = src_start_x + w;
                int src_y = src_start_y + h;
                Color c = p_texture->get_pixel(src_x, src_y);
                if(is_bit) {
                    if(c[image_slot] > 0) {
                        set_pixel(dest_x, dest_y, 1);
                    }
                    else {
                        set_pixel(dest_x, dest_y, 0);
                    }
                    
                }
                else {
                    set_pixel(dest_x, dest_y, c[image_slot] * 255);
                }
            }
        }  
    }

    void FoliageCellMask::scale_instance(const Ref<SceneInstanceBlock>& p_block, float p_sacle_min,float p_scale_max,bool is_invert) {
        if(is_bit) {
            return;
        }
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                if(p_block->get_instance_render_level(y * width + x) == -1) {
                    continue;
                }
                uint8_t value = data[y * width + x];
                if(is_invert) {
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
        ClassDB::bind_method(D_METHOD("hide_instance_by_height_range", "min_height", "max_height"), &FoliageHeightMap::hide_instance_by_height_range);
        ClassDB::bind_method(D_METHOD("hide_instance_by_flatland","instance_range","flatland_height"), &FoliageHeightMap::hide_instance_by_flatland);
        ClassDB::bind_method(D_METHOD("sample_height", "u", "v"), &FoliageHeightMap::sample_height);
        ClassDB::bind_method(D_METHOD("update_height", "block", "base_height", "height_range","image_rect","instance_start_pos"), &FoliageHeightMap::update_height);

        ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "set_width", "get_width");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "set_height", "get_height");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"), "set_data", "get_data");
    }
    void FoliageHeightMap::init(int p_width, int p_height) {
        width = p_width;
        height = p_height;
        data.resize(height * width);
    }
    void FoliageHeightMap::init_form_image(int p_width, int p_height,const Ref<Image>& p_image,const Rect2i& p_rect) {
        width = p_width;
        height = p_height;
        int start_x = p_rect.position.x;
        int start_y = p_rect.position.y;
        data.resize(height * width);
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                data.write[y * width + x] = p_image->get_pixel(start_x + x, start_y + y).r;
            }
        }
    }
    void FoliageHeightMap::init_form_half_data(int p_width, int p_height,const Vector<uint8_t>& p_data) {
        width = p_width;
        height = p_height;

        data.resize(height * width);
        const uint8_t * ptr = p_data.ptr();
        float* dst_ptr = data.ptrw();
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                int ofs = (y * width + x) * 2;                
                uint16_t r = ((uint16_t *)ptr)[ofs];
                dst_ptr[y * width + x] = Math::half_to_float(r) ;
            }
        }

    }
    // 隐藏不在高度范围内的实例
    void FoliageHeightMap::hide_instance_by_height_range(const Ref<SceneInstanceBlock>& p_block, float p_visble_height_min, float p_visble_height_max) {
        if(data.size() == p_block->get_instance_count()) {
            return;
        }
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                if(p_block->get_instance_render_level(y * width + x) == -1) {
                    continue;
                }
                float _height = data[y * width + x];
                if(_height < p_visble_height_min || _height > p_visble_height_max) {
                    p_block->set_instance_render_level(y * width + x, -1);
                }
            }
        }
    }
    // 隱藏非平地的实例
    void FoliageHeightMap::hide_instance_by_flatland(const Ref<SceneInstanceBlock>& p_block,float p_instance_range, float p_height_difference) {
        if(data.size() == p_block->get_instance_count()) {
            return;
        }
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                if(p_block->get_instance_render_level(y * width + x) == -1) {
                    continue;
                }
                float min_height = data[y * width + x];
                float max_height = min_height;
                bool is_flat = true;
                for(int x2 = x - p_instance_range; x2 <= x + p_instance_range; x2++) {
                    for(int y2 = y - p_instance_range; y2 <= y + p_instance_range; y2++) {
                        if(x2 < 0 || x2 >= width || y2 < 0 || y2 >= height) {
                            continue;
                        }
                        float height = data[y2 * width + x2];
                        min_height = MIN(min_height, height);
                        max_height = MAX(max_height, height);
                    }
                }
                if(abs(max_height - min_height) > p_height_difference) {
                    is_flat = false;
                }
                if(!is_flat) {
                    p_block->set_instance_render_level(y * width + x, -1);
                }
            }
        }
        
    }

    void FoliageHeightMap::update_height(const Ref<SceneInstanceBlock>& p_block,float p_base_height,float p_height_range,const Rect2i& p_image_rect,const Vector2& p_instance_start_pos) {
        float start_u = p_image_rect.position.x / (float)width;
        float start_v = p_image_rect.position.y / (float)height;
        float renge_u =  p_image_rect.size.x / (float)width;
        float renge_v = p_image_rect.size.y / (float)height;
        for(int x = 0; x <  p_image_rect.size.x; x++) {
            for(int y = 0; y < p_image_rect.size.y; y++) {
                if(p_block->get_instance_render_level(y * width + x) == -1) {
                    continue;
                }
                Transform3D transform = p_block->get_instance_transform(y * width + x);
                float x2 = transform.origin.x - p_instance_start_pos.x;
                float y2 = transform.origin.y - p_instance_start_pos.y;

                float u = start_u + x2 * renge_u;
                float v = start_v + y2 * renge_v;
                float height = sample_height(u, v);
                transform.origin.y = p_base_height + height * p_height_range;
                p_block->set_instance_transform(y * width + x, transform);
            }
        }
    }

    Vector3 FoliageHeightMap::get_height_map_normal(int x, int z,float p_scale_height, float stepX, float stepZ) const {

        Vector3 p0(x * stepX, get_pixel(x,z) * p_scale_height, z * stepZ);
        Vector3 sumNormal(0, 0, 0);
        Vector3 normals = Vector3(0,0,0);
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
        }
        else {
            sumNormal = Vector3(0, 1, 0);
        }
        return sumNormal;

    }
    static float get_height_form_data(const uint8_t* p_data,int p_width, int p_height,int x, int z,float p_scale_height, float stepX, float stepZ) {
        uint16_t r = ((uint16_t *)p_data)[(z * p_width + x) * 2];
        return Math::half_to_float(r) * p_scale_height;
    }
    Vector3 FoliageHeightMap::get_height_map_normal_form_data(const Vector<uint8_t>& p_data,int width, int height,int x, int z,float p_scale_height, float stepX, float stepZ) {
        Vector3 p0(x * stepX, get_height_form_data(p_data.ptr(),width,height,x,z,p_scale_height, stepX, stepZ), z * stepZ);
        Vector3 sumNormal(0, 0, 0);
        Vector3 normals = Vector3(0,0,0);
        int validTriangleCount = 0;
    
    
        // 左上三角形
        if (x > 0 && z > 0) {
            Vector3 pLeft((x - 1) * stepX, get_height_form_data(p_data.ptr(),width,height,x - 1,z,p_scale_height, stepX, stepZ), z * stepZ);//get_pixel(x - 1, z) * p_scale_height, z * stepZ);
            Vector3 pTop(x * stepX, get_height_form_data(p_data.ptr(),width,height,x,z - 1,p_scale_height, stepX, stepZ), (z - 1) * stepZ);//get_pixel(x, z - 1) * p_scale_height, (z - 1) * stepZ);
            Vector3 normal = (pLeft - p0).cross(pTop - p0);
            normal.normalize();
            sumNormal = sumNormal + normal;
            validTriangleCount++;
        }
    
        // 右上三角形
        if (x < width - 1 && z > 0) {
            Vector3 pRight((x + 1) * stepX, get_height_form_data(p_data.ptr(),width,height,x + 1,z,p_scale_height, stepX, stepZ), z * stepZ);//get_pixel(x + 1, z) * p_scale_height, z * stepZ);
            Vector3 pTop(x * stepX, get_height_form_data(p_data.ptr(),width,height,x,z - 1,p_scale_height, stepX, stepZ), (z - 1) * stepZ);//get_pixel(x, z - 1) * p_scale_height, (z - 1) * stepZ);
            Vector3 normal = (pTop - p0).cross(pRight - p0);
            normal.normalize();
            sumNormal = sumNormal + normal;
            validTriangleCount++;
        }
    
        // 右下三角形
        if (x < width - 1 && z < height - 1) {
            Vector3 pRight((x + 1) * stepX, get_height_form_data(p_data.ptr(),width,height,x + 1,z,p_scale_height, stepX, stepZ), z * stepZ);//get_pixel(x + 1, z) * p_scale_height, z * stepZ);
            Vector3 pBottom(x * stepX, get_height_form_data(p_data.ptr(),width,height,x,z + 1,p_scale_height, stepX, stepZ), (z + 1) * stepZ);//get_pixel(x, z + 1) * p_scale_height, (z + 1) * stepZ);
            Vector3 normal = (pRight - p0).cross(pBottom - p0);
            normal.normalize();
            sumNormal = sumNormal + normal;
            validTriangleCount++;
        }
    
        // 左下三角形
        if (x > 0 && z < height - 1) {
            Vector3 pLeft((x - 1) * stepX, get_height_form_data(p_data.ptr(),width,height,x - 1,z,p_scale_height, stepX, stepZ), z * stepZ);//get_pixel(x - 1, z) * p_scale_height, z * stepZ);
            Vector3 pBottom(x * stepX, get_height_form_data(p_data.ptr(),width,height,x,z + 1,p_scale_height, stepX, stepZ), (z + 1) * stepZ);//get_pixel(x, z + 1) * p_scale_height, (z + 1) * stepZ);
            Vector3 normal = (pBottom - p0).cross(pLeft - p0);
            normal.normalize();
            sumNormal = sumNormal + normal;
            validTriangleCount++;
        }
    
        if (validTriangleCount > 0) {
            sumNormal.normalize();
        }
        else {
            sumNormal = Vector3(0, 1, 0);
        }
        return sumNormal;
        
    }
    float FoliageHeightMap::sample_height(float p_u,float p_v) {
        
        p_u = CLAMP(p_u, 0.0, 1.0);
        p_v = CLAMP(p_v, 0.0, 1.0);
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
        ClassDB::bind_method(D_METHOD("init_form_height_image", "width", "height", "image", "rect","p_scale_height","stepX","stepZ"), &FoliageNormalMap::init_form_height_image);
        ClassDB::bind_method(D_METHOD("init_form_height_map", "width", "height", "image", "rect","p_scale_height","stepX","stepZ"), &FoliageNormalMap::init_form_height_map);
        ClassDB::bind_method(D_METHOD("init_form_half_data", "width", "height", "data", "rect","p_scale_height","stepX","stepZ"), &FoliageNormalMap::init_form_half_data);
        ClassDB::bind_method(D_METHOD("set_pixel", "x", "y", "value"), &FoliageNormalMap::set_pixel);
        ClassDB::bind_method(D_METHOD("get_pixel", "x", "y"), &FoliageNormalMap::get_pixel);
        ClassDB::bind_method(D_METHOD("hide_instance_by_slope", "instance_range","flatland_height"), &FoliageNormalMap::hide_instance_by_slope);
        ClassDB::bind_method(D_METHOD("get_xz_normal_map_texture"), &FoliageNormalMap::get_xz_normal_map_texture);

        ClassDB::bind_method(D_METHOD("set_width", "width"), &FoliageNormalMap::set_width);
        ClassDB::bind_method(D_METHOD("get_width"), &FoliageNormalMap::get_width);

        ClassDB::bind_method(D_METHOD("get_height"), &FoliageNormalMap::get_height);
        ClassDB::bind_method(D_METHOD("set_height", "height"), &FoliageNormalMap::set_height);

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
    void FoliageNormalMap::init_form_image(int p_width, int p_height,const Ref<Image>& p_image,const Rect2i& p_rect) {
        width = p_width;
        height = p_height;
        int start_x = p_rect.position.x;
        int start_y = p_rect.position.y;
        data.resize(height * (uint64_t)width);
        Vector3 * ptr = data.ptrw();
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                Color c = p_image->get_pixel(start_x + x, start_y + y);
                ptr[y * width + x] = Vector3(c.r * 2.0 - 1.0,c.g * 2.0 - 1.0,c.b * 2.0 - 1.0); 
            }
        }
    }
	void FoliageNormalMap::init_form_height_image(int p_width, int p_height, const Ref<Image>& p_image, const Rect2i& p_rect, float p_scale_height, float stepX, float stepZ) {
		width = p_width;
		height = p_height;
		int start_x = p_rect.position.x;
		int start_y = p_rect.position.y;
		data.resize(height * (uint64_t)width);
		Vector3* ptr = data.ptrw();
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				ptr[y * width + x] = p_image->get_height_map_normal(start_x + x, start_y + y, p_scale_height, stepX, stepZ);
			}
		}
	}
    void FoliageNormalMap::init_form_height_map(int p_width, int p_height,const Ref<FoliageHeightMap>& p_image,const Rect2i& p_rect, float p_scale_height, float stepX, float stepZ) {
        width = p_width;
        height = p_height;
        int start_x = p_rect.position.x;
        int start_y = p_rect.position.y;
		data.resize(height * (uint64_t)width);
        Vector3 * ptr = data.ptrw();
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                ptr[y * width + x] = p_image->get_height_map_normal(start_x + x, start_y + y, p_scale_height, stepX, stepZ);
            }
        }
    }
    void FoliageNormalMap::init_form_half_data(int p_width, int p_height, const Vector<uint8_t>& p_data, const Rect2i& p_rect, float p_scale_height, float stepX, float stepZ) {
        width = p_width;
        height = p_height;
        int start_x = p_rect.position.x;
        int start_y = p_rect.position.y;
		data.resize(height * (uint64_t)width);
        Vector3 * ptr = data.ptrw();
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                ptr[y * width + x] = FoliageHeightMap::get_height_map_normal_form_data(p_data,width, height, start_x + x, start_y + y, p_scale_height, stepX, stepZ);
            }
        }
    }
    void FoliageNormalMap::set_pixel(int p_x, int p_y, Vector3 p_value) {
        if(p_x < 0 || p_x >= width || p_y < 0 || p_y >= height) {
            return;
        }
        data.write[p_x + p_y * (uint64_t)width] = p_value;
    }
    Vector3 FoliageNormalMap::get_pixel(int p_x, int p_y){
        if(p_x < 0 || p_x >= width || p_y < 0 || p_y >= height) {
            return Vector3(0,0,0);
        }
        return data[(uint64_t)p_x + p_y * (uint64_t)width];
    }
    void FoliageNormalMap::hide_instance_by_slope(const Ref<SceneInstanceBlock>& p_block, float p_visble_slope_min, float p_visble_slope_max) {
        if(data.size() != p_block->get_instance_count()) {
            return;
        }
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                if(p_block->get_instance_render_level(y * width + x) == -1) {
                    continue;
                }
                const Vector3& slope = data[y * (uint64_t)width + x];
                if(slope.y < p_visble_slope_min || slope.y > p_visble_slope_max) {
                    p_block->set_instance_render_level(y * width + x, -1);
                }
            }
        }
    }
    Ref<ImageTexture> FoliageNormalMap::get_xz_normal_map_texture() const{
        // 法线贴图的y轴时钟向上,所以不需要存储y轴
        const Vector3 * ptr = data.ptr();
        Color c;
        Vector<uint8_t> image_data;
        image_data.resize((uint64_t)width * height * 2);
        uint8_t * ptr2 = image_data.ptrw();
        uint64_t ofs = 0;
        for(int x = 0; x < width; x++) {
            for(int y = 0; y < height; y++) {
                ofs = (y * (uint64_t)width + x) * 2;
                ptr2[ofs] = ptr[y * width + x].x * 0.5 + 0.5;
                ptr2[ofs + 1] = ptr[y * width + x].z * 0.5 + 0.5;
            }
        }
        Ref<Image> image = Image::create_from_data(width, height, false, Image::FORMAT_RG8, image_data);
        Ref<ImageTexture> normal_map = memnew(ImageTexture);
        normal_map->create_from_image(image);
        return normal_map;
    }

}
