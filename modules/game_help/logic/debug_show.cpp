#include "debug_show.h"



DebugShow::DebugShow() {
    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);

    const Color collision_color(1.0, 1.0, 1.0, 1.0);

    material->set_albedo(collision_color);
    material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
    material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
    material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 1);
    material->set_cull_mode(StandardMaterial3D::CULL_BACK);
    material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
    material->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    material->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);

    instance = RenderingServer::get_singleton()->instance_create();
    RenderingServer::get_singleton()->instance_attach_object_instance_id(instance, get_instance_id());
    RS::get_singleton()->instance_geometry_set_visibility_range(instance,0,8192,20,100,RS::VISIBILITY_RANGE_FADE_SELF);
    set_gi_mode(GeometryInstance3D::GI_MODE_DISABLED);
    set_shadow_setting(RenderingServer::SHADOW_CASTING_SETTING_OFF);
}
void DebugShow::init(RID p_world_3d_scenario,const Transform3D& p_xform,const Ref<Mesh>& p_mesh, const Color& p_color) {
    set_color(p_color);
    xform = p_xform;
    mesh = p_mesh;
    if(mesh.is_null()){
        return;
    }
    RenderingServer::get_singleton()->instance_set_base(instance, mesh->get_rid());
    RenderingServer::get_singleton()->instance_set_scenario(instance, p_world_3d_scenario);
    RenderingServer::get_singleton()->instance_set_transform(instance, xform);
    RenderingServer::get_singleton()->instance_set_surface_override_material(instance, 0, material->get_rid());
}

void DebugShow::set_gi_mode(GeometryInstance3D::GIMode p_mode){
    switch (p_mode) {
        case GeometryInstance3D::GI_MODE_DISABLED: {
            RenderingServer::get_singleton()->instance_geometry_set_flag(instance, RenderingServer::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
            RenderingServer::get_singleton()->instance_geometry_set_flag(instance, RenderingServer::INSTANCE_FLAG_USE_DYNAMIC_GI, false);
        } break;
        case GeometryInstance3D::GI_MODE_STATIC: {
            RenderingServer::get_singleton()->instance_geometry_set_flag(instance, RenderingServer::INSTANCE_FLAG_USE_BAKED_LIGHT, true);
            RenderingServer::get_singleton()->instance_geometry_set_flag(instance, RenderingServer::INSTANCE_FLAG_USE_DYNAMIC_GI, false);

        } break;
        case GeometryInstance3D::GI_MODE_DYNAMIC: {
            RenderingServer::get_singleton()->instance_geometry_set_flag(instance, RenderingServer::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
            RenderingServer::get_singleton()->instance_geometry_set_flag(instance, RenderingServer::INSTANCE_FLAG_USE_DYNAMIC_GI, true);
        } 
    }
}
void DebugShow::set_shadow_setting(RenderingServer::ShadowCastingSetting setting){
    
    RenderingServer::get_singleton()->instance_geometry_set_cast_shadows_setting(instance,setting);

}
DebugShow:: ~DebugShow() {
    
    RenderingServer::get_singleton()->free(instance);
}