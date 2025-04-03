#include "register_types.hpp"

#include "extrude_profiles/path_extrude_profile_circle.hpp"
#include "extrude_profiles/path_extrude_profile_manual.hpp"
#include "extrude_profiles/path_extrude_profile_rect.hpp"
#include "path_extrude_3d.hpp"
#include "path_extrude_profile_base.hpp"
#include "path_mesh_3d.hpp"
#include "path_multimesh_3d.hpp"
#include "path_scene_3d.hpp"

void initialize_path_mesh_3d(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(PathMesh3D);
		GDREGISTER_CLASS(PathExtrude3D);
		GDREGISTER_CLASS(PathMultiMesh3D);
		GDREGISTER_CLASS(PathScene3D);
		GDREGISTER_CLASS(PathExtrudeProfileBase);
		GDREGISTER_CLASS(PathExtrudeProfileManual);
		GDREGISTER_CLASS(PathExtrudeProfileRect);
		GDREGISTER_CLASS(PathExtrudeProfileCircle);
	}
}

void uninitialize_path_mesh_3d(ModuleInitializationLevel p_level) {
	// void
}
