#pragma once

#include "core/object/ref_counted.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/3d/visual_instance_3d.h"

class DebugShow : public RefCounted
{
    GDCLASS(DebugShow,RefCounted)
    static void _bind_methods() {}
public:
    DebugShow();
    void init(RID p_world_3d_scenario,const Transform3D& p_xform,const Ref<Mesh>& p_mesh, const Color& p_color) ;

    void set_gi_mode(GeometryInstance3D::GIMode p_mode);
    void set_shadow_setting(RenderingServer::ShadowCastingSetting setting);
public:
    void set_color(const Color &p_color) { material->set_albedo(p_color); }
    Color get_color() const { return material->get_albedo(); }
    virtual ~DebugShow() ;
    Ref<StandardMaterial3D> material;
    RID instance;
    Ref<Mesh> mesh;
    Transform3D xform;
};