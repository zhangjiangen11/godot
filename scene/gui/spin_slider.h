

#include "scene/gui/line_edit.h"
#include "scene/gui/range.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/timer.h"
class SpinSlider : public Range {
	GDCLASS(SpinSlider, Range);

	String label;
	String suffix;
	int updown_offset = -1;
	bool hover_updown = false;

	TextureRect *grabber = nullptr;
	int grabber_range = 1;

	bool mouse_over_spin = false;
	bool mouse_over_grabber = false;
	bool mousewheel_over_grabber = false;

	bool grabbing_grabber = false;
	int grabbing_from = 0;
	float grabbing_ratio = 0.0f;

	bool grabbing_spinner_attempt = false;
	bool grabbing_spinner = false;

	bool read_only = false;
	float grabbing_spinner_dist_cache = 0.0f;
	float grabbing_spinner_speed = 0.0f;
	Vector2 grabbing_spinner_mouse_pos;
	double pre_grab_value = 0.0;

	LineEdit *value_input = nullptr;
	uint64_t value_input_closed_frame = 0;
	bool value_input_dirty = false;
	bool value_input_focus_visible = false;

	float integer_drag_speed = 0.5;
	float float_drag_speed = 0.5;

public:
	enum ControlState {
		CONTROL_STATE_DEFAULT,
		CONTROL_STATE_PREFER_SLIDER,
		CONTROL_STATE_HIDE,
	};

private:
	ControlState control_state = CONTROL_STATE_DEFAULT;
	bool flat = false;
	bool editing_integer = false;

	void _grab_start();
	void _grab_end();

	void _grabber_gui_input(const Ref<InputEvent> &p_event);
	void _value_input_hidden();
	void _value_input_submitted(const String &);
	void _value_focus_exited();
	void _value_input_gui_input(const Ref<InputEvent> &p_event);

	void _evaluate_input_text();

	void _update_value_input_stylebox();
	void _ensure_value_input();
	void _draw_spin_slider();

	struct ThemeCache {
		Ref<StyleBox> label_bg;
		Ref<StyleBox> style_normal;
		Ref<StyleBox> style_focus;
		Ref<StyleBox> style_readonly;

		Ref<Texture2D> updown_icon;
		Ref<Texture2D> updown_disabled_icon;
		Ref<Texture2D> grabber;
		Ref<Texture2D> grabber_highlight;

		Color font_color;
		Color font_uneditable_color;
		Color read_only_label_color;

		Ref<Font> font;
		int font_size = 16;
		int grabber_w = 16;
	} theme_cache;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	static void _bind_methods();
	void _grabber_mouse_entered();
	void _grabber_mouse_exited();
	void _focus_entered(bool p_hide_focus = false);

public:
	String get_tooltip(const Point2 &p_pos) const override;

	virtual Size2 get_minimum_size() const override;

	String get_text_value() const;
	void set_label(const String &p_label);
	String get_label() const;

	void set_suffix(const String &p_suffix);
	String get_suffix() const;

	void set_control_state(ControlState p_type);
	ControlState get_control_state() const;

#ifndef DISABLE_DEPRECATED
	void set_hide_slider(bool p_hide);
	bool is_hiding_slider() const;
#endif

	void set_editing_integer(bool p_editing_integer);
	bool is_editing_integer() const;

	void set_read_only(bool p_enable);
	bool is_read_only() const;

	void set_flat(bool p_enable);
	bool is_flat() const;

	bool is_grabbing() const;

	void setup_and_show() { _focus_entered(); }
	LineEdit *get_line_edit();

	SpinSlider();
};

VARIANT_ENUM_CAST(SpinSlider::ControlState)
