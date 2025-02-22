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
        for (int i = 0; i < transform.size(); i++) {
            if (render_level[i] >= 0) {
                transform.push_back(temp_transform[i]);
                color.push_back(temp_color[i]);
                curstom_color.push_back(temp_curstom_color[i]);
            }
        }
        render_level.resize(transform.size());
    }
    // 隐藏不显示的实例
    void SceneInstanceBlock::hide_instance_by_cell_mask(const Ref<FoliageCellMask>& p_cell_mask,uint8_t p_visble_value_min,uint8_t p_visble_value_max) {
        if (transform.size() != p_cell_mask->get_data().size()) {
            return;
        }

        for(int32_t w = 0; w < p_cell_mask->get_width(); w++) {
            for(int32_t h = 0; h < p_cell_mask->get_height(); h++) {
                uint8_t v = p_cell_mask->get_pixel(w, h);
                if (v < p_visble_value_min || v > p_visble_value_max) {
                    int index = w + h * p_cell_mask->get_width();
                    render_level[index] = -1;
                }
            }
        }
    }

	void SceneInstanceBlock::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_instance_count", "instance_count"), &SceneInstanceBlock::set_instance_count);
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

        ClassDB::bind_method(D_METHOD("remove_hiden_instances"), &SceneInstanceBlock::remove_hiden_instances);
        ClassDB::bind_method(D_METHOD("compute_rotation","p_index","p_normal","p_angle"), &SceneInstanceBlock::compute_rotation);
        ClassDB::bind_method(D_METHOD("hide_instance_by_cell_mask","p_cell_mask","p_visble_value_min","p_visble_value_max"), &SceneInstanceBlock::hide_instance_by_cell_mask);

        ADD_PROPERTY(PropertyInfo(Variant::INT, "instance_count"), "set_instance_count", "get_instance_count");
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





    void FoliageCellMask::_bind_methods() {
        ClassDB::bind_method(D_METHOD("init", "width", "height","is_bit"), &FoliageCellMask::init);
        ClassDB::bind_method(D_METHOD("set_pixel", "x", "y", "value"), &FoliageCellMask::set_pixel);
        ClassDB::bind_method(D_METHOD("set_rect_pixel", "x", "y", "width", "height", "value"), &FoliageCellMask::set_rect_pixel);
        ClassDB::bind_method(D_METHOD("set_circle_pixel", "x", "y", "radius", "value"), &FoliageCellMask::set_circle_pixel);
        ClassDB::bind_method(D_METHOD("set_form_texture_pixel","texture","dest_rect","source_rect","image_slot"), &FoliageCellMask::set_form_texture_pixel);
        ClassDB::bind_method(D_METHOD("get_pixel", "x", "y"), &FoliageCellMask::get_pixel);

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



}
