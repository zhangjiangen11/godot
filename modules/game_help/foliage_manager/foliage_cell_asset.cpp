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










}
