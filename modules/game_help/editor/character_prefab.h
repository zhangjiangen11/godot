#include "../logic/character_shape/character_body_prefab.h"
#include "body_main_editor.h"
#include "editor/plugins/mesh_editor_plugin.h"

class CharacterPrefabSection : public LogicSectionBase {
	GDCLASS(CharacterPrefabSection, LogicSectionBase);

public:
	virtual void create_header(HBoxContainer *hb) override {
		set_category_name(L"身体部位列表");
	}
	virtual void create_child_list(VBoxContainer *hb) override {
	}
	virtual void update_state() override {
		prefab = Object::cast_to<CharacterBodyPrefab>(object);
		part = prefab->get_parts();
		for (const KeyValue<Variant, Variant> &kv : part) {
			HBoxContainer *hbox = memnew(HBoxContainer);
			tasks_container->add_child(hbox);

			Label *label = memnew(Label);
			label->set_text(kv.key);
			label->set_h_size_flags(SIZE_EXPAND_FILL);
			hbox->add_child(label);

			CheckButton *button = memnew(CheckButton);
			button->set_pressed(kv.value);
			button->connect("toggled", callable_mp(this, &CharacterPrefabSection::on_part_toggled).bind(kv.key));
			hbox->add_child(button);
		}
	}
	void on_part_toggled(bool p_pressed, StringName index) {
		part[index] = p_pressed;
		prefab->set_parts(part);
	}
	virtual String get_section_unfolded() const override { return "parts Condition Section"; }

	Ref<CharacterBodyPrefab> prefab;
	Dictionary part;
};
