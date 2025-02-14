#ifndef PATH_EXTRUDE_PROFILE_BASE_H
#define PATH_EXTRUDE_PROFILE_BASE_H
#include "core/io/resource.h"
#include "scene/resources/mesh.h"


//namespace godot {

class PathExtrudeProfileBase : public Resource {
    GDCLASS(PathExtrudeProfileBase, Resource)

public:
    _ALWAYS_INLINE_ Array get_mesh_arrays() const { return mesh_array.duplicate(); }
    _ALWAYS_INLINE_ PackedVector2Array get_cross_section() const { return mesh_array[Mesh::ARRAY_VERTEX]; }

    _ALWAYS_INLINE_ void set_flip_normals(const bool p_flip_normals) {
        if (p_flip_normals != flip_normals) {
            flip_normals = p_flip_normals;
            queue_update();
        }
    }
    _ALWAYS_INLINE_ bool get_flip_normals() const { return flip_normals; }

    _ALWAYS_INLINE_ void queue_update() { dirty = true; callable_mp(this, &PathExtrudeProfileBase::_regen).call_deferred(); }

    GDVIRTUAL0R(Array, _generate_cross_section)

protected:
    virtual Array _generate_cross_section();

    void _notification(int p_what);
    static void _bind_methods();

private:
    bool dirty = true;
    bool flip_normals = false;
    Array mesh_array ;

    void _regen();
};

//}

#endif
