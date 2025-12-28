#pragma once
#include "core/io/resource.h"
#include "scene/main/timer.h"
#include "scene/gui/scroll_container.h"
class Gradient;
class ScrollDamper : public Resource {
	GDCLASS(ScrollDamper, Resource)

protected:
	float rebound_strength = 7.0f;
	float _attract_factor = 400.0f;

	static void _bind_methods();

	// 抽象方法声明（子类必须实现）
	virtual float _calculate_velocity_by_time(float time) const = 0;
	virtual float _calculate_time_by_velocity(float velocity) const = 0;
	virtual float _calculate_offset_by_time(float time) const = 0;
	virtual float _calculate_time_by_offset(float offset) const = 0;

public:
	ScrollDamper();
	~ScrollDamper() override = default;

	// 导出属性
	void set_rebound_strength(float p_strength);
	float get_rebound_strength() const { return rebound_strength; }

	// 公共方法
	virtual float _calculate_velocity_to_dest(float from, float to) const;
	virtual float _calculate_next_velocity(float present_time, float delta_time) const;
	virtual float _calculate_next_offset(float present_time, float delta_time) const;
	Array slide(float velocity, float delta_time) const;
	float attract(float from, float to, float velocity, float delta_time) const;
};
class SmoothScrollContainer : public ScrollContainer {
	GDCLASS(SmoothScrollContainer, ScrollContainer)
public:
	// 滚动输入类型枚举
	enum ScrollType {
		WHEEL = 1,
		BAR = 2,
		DRAG = 4
	};

private:
	// ------------------------------
	// 导出属性（对应GDScript @export）
	// ------------------------------
	// 鼠标滚轮设置
	float speed = 1000.0f;
	Ref<ScrollDamper> wheel_scroll_damper;
	// 拖动设置
	Ref<ScrollDamper> dragging_scroll_damper;
	bool drag_with_mouse = true;
	bool drag_with_touch = true;
	// 容器设置
	float just_snap_under = 0.4f;
	int follow_focus_margin = 20;
	bool allow_vertical_scroll = true;
	bool allow_horizontal_scroll = true;
	bool auto_allow_scroll = true;
	bool allow_overdragging = true;
	// 滚动条设置
	bool hide_scrollbar_over_time = false;
	float scrollbar_hide_time = 5.0f;
	float scrollbar_fade_in_time = 0.2f;
	float scrollbar_fade_out_time = 0.5f;
	// 输入设置
	bool handle_input = true;
	// 调试设置
	bool debug_mode = false;

	// ------------------------------
	// 内部状态变量
	// ------------------------------
	Vector2 velocity = Vector2(0, 0);
	Control *content_node = nullptr;
	Vector2 pos = Vector2(0, 0);
	Ref<ScrollDamper> scroll_damper;
	bool h_scrollbar_dragging = false;
	bool v_scrollbar_dragging = false;
	bool content_dragging = false;
	bool content_dragging_moved = false;
	Timer *scrollbar_hide_timer = nullptr;
	Ref<Tween> scrollbar_show_tween;
	Ref<Tween> scrollbar_hide_tween;
	Ref<Tween> scroll_x_to_tween;
	Ref<Tween> scroll_y_to_tween;
	// drag_temp_data: [0]x相对移动, [1]y相对移动, [2]初始x, [3]初始y, [4]左距, [5]右距, [6]上距, [7]下距
	double drag_temp_data[8] = {0};
	bool is_in_deadzone = false;
	bool mouse_on_scrollbar = false;
	bool is_scrolling = false;
	ScrollType last_scroll_type = WHEEL;
	Ref<Gradient> debug_gradient;
	// 滚动死区（默认10像素，与GDScript隐含逻辑一致）
	const float scroll_deadzone = 10.0f;

	// ------------------------------
	// 私有工具函数
	// ------------------------------
	void _set_hide_scrollbar_over_time(bool p_value);
	bool _get_hide_scrollbar_over_time() const { return hide_scrollbar_over_time; }
	void _scrollbar_hide_timer_timeout();
	void _mouse_on_scroll_bar(bool entered);
	void _scrollbar_input(const Ref<InputEvent> &event, bool vertical);
	void _on_focus_changed(Control *control);
	void _on_node_added(Node *node);
	void init_drag_temp_data();
	void handle_content_dragging();
	void remove_all_children_focus(Node *node);
	void update_is_scrolling();
	void update_scrollbars();
	void kill_scroll_x_to_tween();
	void kill_scroll_y_to_tween();
	void kill_scroll_to_tweens();
	void setup_debug_drawing();
	void draw_debug();
	bool handle_scrollbar_drag();
	float handle_overdrag(bool vertical, float axis_velocity, float axis_pos, float delta);
	Vector2 snap(bool vertical, float axis_velocity, float axis_pos);
	void scroll(bool vertical, float axis_velocity, float axis_pos, float delta);

	// 尺寸计算工具函数
	float get_spare_size_x();
	float get_spare_size_y();
	Vector2 get_spare_size();
	float get_child_size_x_diff(Control *child, bool clamp);
	float get_child_size_y_diff(Control *child, bool clamp);
	Vector2 get_child_size_diff(Control *child, bool clamp_x, bool clamp_y);
	float get_child_left_dist(float child_pos_x, float child_size_diff_x) const;
	float get_child_right_dist(float child_pos_x, float child_size_diff_x) const;
	float get_child_top_dist(float child_pos_y, float child_size_diff_y) const;
	float get_child_bottom_dist(float child_pos_y, float child_size_diff_y) const;
	Vector4 get_child_boundary_dist(const Vector2 &child_pos, const Vector2 &child_size_diff) const;

	// 滚动判断工具函数
	bool should_scroll_vertical();
	bool should_scroll_horizontal();
	bool any_scroll_bar_dragged() const;
	bool is_outside_top_boundary(float y_pos = -1.0f);
	bool is_outside_bottom_boundary(float y_pos = -1.0f);
	bool is_outside_left_boundary(float x_pos = -1.0f);
	bool is_outside_right_boundary(float x_pos = -1.0f);
	bool has_active_scroll_tween();

	// 滚动条动画函数
	void show_scrollbars(float time = -1.0f);
	void hide_scrollbars(float time = -1.0f);

protected:
	static void _bind_methods();
	void _ready() override;
	void process(double delta) override;
	void gui_input(const Ref<InputEvent> &event) override;
	void _draw() override;
	Variant getvar(const Variant &property, bool *r_valid = nullptr) const override;
	void setvar(const Variant &property, const Variant &value, bool *r_valid = nullptr) override;

public:
	// ------------------------------
	// 构造/析构
	// ------------------------------
	SmoothScrollContainer();
	~SmoothScrollContainer() = default;

	// ------------------------------
	// 导出方法（供外部调用）
	// ------------------------------
	void scroll_x_to(float x_pos, float duration = 0.5f);
	void scroll_y_to(float y_pos, float duration = 0.5f);
	void scroll_page_up(float duration = 0.5f);
	void scroll_page_down(float duration = 0.5f);
	void scroll_page_left(float duration = 0.5f);
	void scroll_page_right(float duration = 0.5f);
	void scroll_vertically(float amount);
	void scroll_horizontally(float amount);
	void scroll_to_top(float duration = 0.5f);
	void scroll_to_bottom(float duration = 0.5f);
	void scroll_to_left(float duration = 0.5f);
	void scroll_to_right(float duration = 0.5f);
	void ensure_control_visible(Control *control);

	// ------------------------------
	// 属性Setter/Getter（绑定到编辑器）
	// ------------------------------
	// 鼠标滚轮
	void set_speed(float p_speed) { speed = p_speed; }
	float get_speed() const { return speed; }
	void set_wheel_scroll_damper(const Ref<ScrollDamper> &p_damper) { wheel_scroll_damper = p_damper; }
	Ref<ScrollDamper> get_wheel_scroll_damper() const { return wheel_scroll_damper; }
	// 拖动
	void set_dragging_scroll_damper(const Ref<ScrollDamper> &p_damper) { dragging_scroll_damper = p_damper; }
	Ref<ScrollDamper> get_dragging_scroll_damper() const { return dragging_scroll_damper; }
	void set_drag_with_mouse(bool p_enable) { drag_with_mouse = p_enable; }
	bool get_drag_with_mouse() const { return drag_with_mouse; }
	void set_drag_with_touch(bool p_enable) { drag_with_touch = p_enable; }
	bool get_drag_with_touch() const { return drag_with_touch; }
	// 容器
	void set_just_snap_under(float p_value) { just_snap_under = p_value; }
	float get_just_snap_under() const { return just_snap_under; }
	void set_follow_focus_margin(int p_margin) { follow_focus_margin = p_margin; }
	int get_follow_focus_margin() const { return follow_focus_margin; }
	void set_allow_vertical_scroll(bool p_enable) { allow_vertical_scroll = p_enable; }
	bool get_allow_vertical_scroll() const { return allow_vertical_scroll; }
	void set_allow_horizontal_scroll(bool p_enable) { allow_horizontal_scroll = p_enable; }
	bool get_allow_horizontal_scroll() const { return allow_horizontal_scroll; }
	void set_auto_allow_scroll(bool p_enable) { auto_allow_scroll = p_enable; }
	bool get_auto_allow_scroll() const { return auto_allow_scroll; }
	void set_allow_overdragging(bool p_enable) { allow_overdragging = p_enable; }
	bool get_allow_overdragging() const { return allow_overdragging; }
	// 滚动条
	void set_scrollbar_hide_time(float p_time) { scrollbar_hide_time = p_time; }
	float get_scrollbar_hide_time() const { return scrollbar_hide_time; }
	void set_scrollbar_fade_in_time(float p_time) { scrollbar_fade_in_time = p_time; }
	float get_scrollbar_fade_in_time() const { return scrollbar_fade_in_time; }
	void set_scrollbar_fade_out_time(float p_time) { scrollbar_fade_out_time = p_time; }
	float get_scrollbar_fade_out_time() const { return scrollbar_fade_out_time; }
	// 输入
	void set_handle_input(bool p_enable) { handle_input = p_enable; }
	bool get_handle_input() const { return handle_input; }
	// 调试
	void set_debug_mode(bool p_enable) { debug_mode = p_enable; }
	bool get_debug_mode() const { return debug_mode; }

	// ------------------------------
	// 信号定义
	// ------------------------------
	void _on_scroll_started() { emit_signal("scroll_started"); }
	void _on_scroll_ended() { emit_signal("scroll_ended"); }
};

class LinearScrollDamper : public ScrollDamper {
	GDCLASS(LinearScrollDamper, ScrollDamper)

private:
	float friction = 4.0f;
	float _factor = 10000.0f;

protected:
	static void _bind_methods();

public:
	LinearScrollDamper();
	~LinearScrollDamper() = default;

	// 摩擦系数设置
	void set_friction(float p_friction);
	float get_friction() const;

	// 实现抽象方法
	float _calculate_velocity_by_time(float time) const override;
	float _calculate_time_by_velocity(float velocity) const override;
	float _calculate_offset_by_time(float time) const override;
	float _calculate_time_by_offset(float offset) const override;
};

class ExpoScrollDamper : public ScrollDamper {
	GDCLASS(ExpoScrollDamper, ScrollDamper)

private:
	float friction = 4.0f;
	float _factor = 10000.0f;
	float minimum_velocity = 0.4f;

protected:
	static void _bind_methods();

public:
	ExpoScrollDamper();
	~ExpoScrollDamper() = default;

	// Setters and Getters
	void set_friction(float p_friction);
	float get_friction() const;

	void set_minimum_velocity(float p_min_vel);
	float get_minimum_velocity() const;

	// Overridden abstract methods
	float _calculate_velocity_by_time(float time) const override;
	float _calculate_time_by_velocity(float velocity) const override;
	float _calculate_offset_by_time(float time) const override;
	float _calculate_time_by_offset(float offset) const override;
	float _calculate_velocity_to_dest(float from, float to) const override;
};

class CubicScrollDamper : public ScrollDamper {
	GDCLASS(CubicScrollDamper, ScrollDamper)

protected:
	float friction = 4.0f;
	float _factor = 10000.0f;

	static void _bind_methods();

public:
	CubicScrollDamper();
	~CubicScrollDamper() override = default;

	// 属性访问方法
	void set_friction(float p_friction);
	float get_friction() const { return friction; }

	// 实现抽象方法
	float _calculate_velocity_by_time(float time) const override;
	float _calculate_time_by_velocity(float velocity) const override;
	float _calculate_offset_by_time(float time) const override;
	float _calculate_time_by_offset(float offset) const override;
};
VARIANT_ENUM_CAST(SmoothScrollContainer::ScrollType);
