#include "mpath.h"

#include "core/io/resource_saver.h"
#include "scene/resources/3d/world_3d.h"
#include "servers/rendering_server.h"

#define RSS RenderingServer::get_singleton()

Vector<MPath *> MPath::all_path_nodes;

void MPath::_bind_methods() {
	ADD_SIGNAL(MethodInfo("curve_changed"));

	ClassDB::bind_method(D_METHOD("set_curve", "input"), &MPath::set_curve);
	ClassDB::bind_method(D_METHOD("get_curve"), &MPath::get_curve);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "curve", PROPERTY_HINT_RESOURCE_TYPE, "MCurve"), "set_curve", "get_curve");

	ClassDB::bind_static_method("MPath", D_METHOD("get_all_path_nodes"), &MPath::get_all_path_nodes);
}

TypedArray<MPath> MPath::get_all_path_nodes() {
	TypedArray<MPath> out;
	for (MPath *p : all_path_nodes) {
		if (p->is_inside_tree()) {
			out.push_back(p);
		}
	}
	return out;
}

MPath::MPath() {
	set_process(true);
	set_notify_transform(true);
	all_path_nodes.push_back(this);
}
MPath::~MPath() {
	for (int i = 0; i < all_path_nodes.size(); i++) {
		if (this == all_path_nodes[i]) {
			all_path_nodes.remove_at(i);
			break;
		}
	}
}

void MPath::set_curve(Ref<MCurve> input) {
	if (!input.is_valid()) {
		if (curve.is_valid()) {
			curve->disconnect("curve_updated", Callable(this, "update_gizmos"));
		}
		curve = input;
		emit_signal("curve_changed");
		return;
	}
	curve = input;
	if (is_inside_tree()) {
		input->init_insert();
	}
	curve->connect("curve_updated", Callable(this, "update_gizmos"));
	emit_signal("curve_changed");
}

Ref<MCurve> MPath::get_curve() {
	return curve;
}

void MPath::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS:
			if (curve.is_valid()) {
				curve->init_insert();
				//set_process(false);
			}
			break;
		case NOTIFICATION_READY:
			update_scenario_space();
			break;
		case NOTIFICATION_TRANSFORM_CHANGED:
			set_global_transform(Transform3D());
			break;
		case NOTIFICATION_EDITOR_PRE_SAVE:
			if (curve.is_valid()) {
				String file_name = curve->get_path().get_file();
				if (file_name.is_valid_filename()) {
					String ext = file_name.get_extension();
					if (ext != "res") {
						WARN_PRINT_ONCE("Please save curve resource in \"" + get_name() + "\" as .res extension");
					}
					// Maybe later put this in a condition
					ResourceSaver::save(curve, curve->get_path());
				} else {
					WARN_PRINT_ONCE("Please save curve resource in \"" + get_name() + "\" as .res extension");
				}
			}
			break;
		case NOTIFICATION_ENTER_WORLD:
			//set_visible(true);
			break;
		case NOTIFICATION_EXIT_WORLD:
			//set_visible(false);
			break;
	}
}

void MPath::update_scenario_space() {
	scenario = get_world_3d()->get_scenario();
	space = get_world_3d()->get_space();
}

RID MPath::get_scenario() {
	if (!scenario.is_valid()) {
		update_scenario_space();
	}
	return scenario;
}

RID MPath::get_space() {
	if (!space.is_valid()) {
		update_scenario_space();
	}
	return space;
}