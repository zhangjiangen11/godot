#include "smooth_scroll_container.h"
#include "scene/animation/tween.h"
#include "scene/resources/gradient.h"
#include "scene/main/viewport.h"

ScrollDamper::ScrollDamper() {
	// 初始化时确保吸引因子正确计算
	_attract_factor = rebound_strength * rebound_strength * rebound_strength;
}

void ScrollDamper::_bind_methods() {
	// 绑定属性
	ClassDB::bind_method(D_METHOD("set_rebound_strength", "strength"), &ScrollDamper::set_rebound_strength);
	ClassDB::bind_method(D_METHOD("get_rebound_strength"), &ScrollDamper::get_rebound_strength);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "rebound_strength", PROPERTY_HINT_RANGE, "0.0,1.0,0.001,or_greater,hide_slider"),
			"set_rebound_strength",
			"get_rebound_strength");

	// 绑定方法
	ClassDB::bind_method(D_METHOD("_calculate_velocity_to_dest", "from", "to"), &ScrollDamper::_calculate_velocity_to_dest);
	ClassDB::bind_method(D_METHOD("_calculate_next_velocity", "present_time", "delta_time"), &ScrollDamper::_calculate_next_velocity);
	ClassDB::bind_method(D_METHOD("_calculate_next_offset", "present_time", "delta_time"), &ScrollDamper::_calculate_next_offset);
	ClassDB::bind_method(D_METHOD("slide", "velocity", "delta_time"), &ScrollDamper::slide);
	ClassDB::bind_method(D_METHOD("attract", "from", "to", "velocity", "delta_time"), &ScrollDamper::attract);

	// 标记抽象方法（C++中通过纯虚函数实现）
	ClassDB::bind_method(D_METHOD("_calculate_velocity_by_time", "time"), &ScrollDamper::_calculate_velocity_by_time);
	ClassDB::bind_method(D_METHOD("_calculate_time_by_velocity", "velocity"), &ScrollDamper::_calculate_time_by_velocity);
	ClassDB::bind_method(D_METHOD("_calculate_offset_by_time", "time"), &ScrollDamper::_calculate_offset_by_time);
	ClassDB::bind_method(D_METHOD("_calculate_time_by_offset", "offset"), &ScrollDamper::_calculate_time_by_offset);
}

void ScrollDamper::set_rebound_strength(float p_strength) {
	rebound_strength = MAX(p_strength, 0.0f);
	_attract_factor = rebound_strength * rebound_strength * rebound_strength;
}

float ScrollDamper::_calculate_velocity_to_dest(float from, float to) const {
	float dist = to - from;
	float time = _calculate_time_by_offset(Math::abs(dist));
	float vel = _calculate_velocity_by_time(time) * Math::sign(dist);
	return vel;
}

float ScrollDamper::_calculate_next_velocity(float present_time, float delta_time) const {
	return _calculate_velocity_by_time(present_time - delta_time);
}

float ScrollDamper::_calculate_next_offset(float present_time, float delta_time) const {
	return _calculate_offset_by_time(present_time) - _calculate_offset_by_time(present_time - delta_time);
}

Array ScrollDamper::slide(float velocity, float delta_time) const {
	Array result;
	if (Math::abs(velocity) < 0.001f) {
		result.append(0.0f);
		result.append(0.0f);
		return result;
	}

	float present_time = _calculate_time_by_velocity(velocity);
	float next_vel = _calculate_next_velocity(present_time, delta_time) * Math::sign(velocity);
	float next_offset = _calculate_next_offset(present_time, delta_time) * Math::sign(velocity);

	result.append(next_vel);
	result.append(next_offset);
	return result;
}

float ScrollDamper::attract(float from, float to, float velocity, float delta_time) const {
	float dist = to - from;
	if (Math::abs(dist) < 0.001f) {
		return 0.0f;
	}

	float target_vel = _calculate_velocity_to_dest(from, to);
	velocity += _attract_factor * dist * delta_time + _calculate_velocity_by_time(delta_time) * Math::sign(dist);

	// 限制速度不超过目标速度
	if ((dist > 0 && velocity >= target_vel) || (dist < 0 && velocity <= target_vel)) {
		velocity = target_vel;
	}

	return velocity;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ------------------------------
// 构造函数：初始化默认值
// ------------------------------
SmoothScrollContainer::SmoothScrollContainer() {
	// 初始化默认滚动阻尼器（与GDScript的ExpoScrollDamper.new()对应）
	wheel_scroll_damper = memnew(ExpoScrollDamper);
	dragging_scroll_damper = memnew(ExpoScrollDamper);
	scroll_damper = wheel_scroll_damper;

	// 初始化滚动条隐藏计时器
	scrollbar_hide_timer = memnew(Timer);
	scrollbar_hide_timer->set_one_shot(true);
	scrollbar_hide_timer->connect("timeout", callable_mp(this, &SmoothScrollContainer::_scrollbar_hide_timer_timeout));

	// 初始化调试渐变
	debug_gradient = memnew(Gradient);
	debug_gradient->set_color(0.0f, Color(0, 1, 0)); // 绿色
	debug_gradient->set_color(1.0f, Color(1, 0, 0)); // 红色

	set_process(true);
}

// ------------------------------
// 绑定方法与信号（Godot C++核心）
// ------------------------------
void SmoothScrollContainer::_bind_methods() {
	// 1. 绑定信号
	//ADD_SIGNAL(MethodInfo("scroll_started"));
	//ADD_SIGNAL(MethodInfo("scroll_ended"));

	// 2. 绑定属性（编辑器分组与可见性）
	// 鼠标滚轮组
	ClassDB::bind_method(D_METHOD("set_speed", "speed"), &SmoothScrollContainer::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &SmoothScrollContainer::get_speed);
	ClassDB::bind_method(D_METHOD("set_wheel_scroll_damper", "damper"), &SmoothScrollContainer::set_wheel_scroll_damper);
	ClassDB::bind_method(D_METHOD("get_wheel_scroll_damper"), &SmoothScrollContainer::get_wheel_scroll_damper);
	ADD_GROUP("Mouse Wheel", "mouse_wheel_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mouse_wheel_speed", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater,hide_slider"), "set_speed", "get_speed");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mouse_wheel_scroll_damper", PROPERTY_HINT_RESOURCE_TYPE, "ScrollDamper"), "set_wheel_scroll_damper", "get_wheel_scroll_damper");

	// 拖动组
	ClassDB::bind_method(D_METHOD("set_dragging_scroll_damper", "damper"), &SmoothScrollContainer::set_dragging_scroll_damper);
	ClassDB::bind_method(D_METHOD("get_dragging_scroll_damper"), &SmoothScrollContainer::get_dragging_scroll_damper);
	ClassDB::bind_method(D_METHOD("set_drag_with_mouse", "enable"), &SmoothScrollContainer::set_drag_with_mouse);
	ClassDB::bind_method(D_METHOD("get_drag_with_mouse"), &SmoothScrollContainer::get_drag_with_mouse);
	ClassDB::bind_method(D_METHOD("set_drag_with_touch", "enable"), &SmoothScrollContainer::set_drag_with_touch);
	ClassDB::bind_method(D_METHOD("get_drag_with_touch"), &SmoothScrollContainer::get_drag_with_touch);
	ADD_GROUP("Dragging", "dragging_");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "dragging_scroll_damper", PROPERTY_HINT_RESOURCE_TYPE, "ScrollDamper"), "set_dragging_scroll_damper", "get_dragging_scroll_damper");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dragging_drag_with_mouse"), "set_drag_with_mouse", "get_drag_with_mouse");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dragging_drag_with_touch"), "set_drag_with_touch", "get_drag_with_touch");

	// 容器组
	ClassDB::bind_method(D_METHOD("set_just_snap_under", "value"), &SmoothScrollContainer::set_just_snap_under);
	ClassDB::bind_method(D_METHOD("get_just_snap_under"), &SmoothScrollContainer::get_just_snap_under);
	ClassDB::bind_method(D_METHOD("set_follow_focus_margin", "margin"), &SmoothScrollContainer::set_follow_focus_margin);
	ClassDB::bind_method(D_METHOD("get_follow_focus_margin"), &SmoothScrollContainer::get_follow_focus_margin);
	ClassDB::bind_method(D_METHOD("set_allow_vertical_scroll", "enable"), &SmoothScrollContainer::set_allow_vertical_scroll);
	ClassDB::bind_method(D_METHOD("get_allow_vertical_scroll"), &SmoothScrollContainer::get_allow_vertical_scroll);
	ClassDB::bind_method(D_METHOD("set_allow_horizontal_scroll", "enable"), &SmoothScrollContainer::set_allow_horizontal_scroll);
	ClassDB::bind_method(D_METHOD("get_allow_horizontal_scroll"), &SmoothScrollContainer::get_allow_horizontal_scroll);
	ClassDB::bind_method(D_METHOD("set_auto_allow_scroll", "enable"), &SmoothScrollContainer::set_auto_allow_scroll);
	ClassDB::bind_method(D_METHOD("get_auto_allow_scroll"), &SmoothScrollContainer::get_auto_allow_scroll);
	ClassDB::bind_method(D_METHOD("set_allow_overdragging", "enable"), &SmoothScrollContainer::set_allow_overdragging);
	ClassDB::bind_method(D_METHOD("get_allow_overdragging"), &SmoothScrollContainer::get_allow_overdragging);
	ADD_GROUP("Container", "container_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "container_just_snap_under"), "set_just_snap_under", "get_just_snap_under");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "container_follow_focus_margin", PROPERTY_HINT_RANGE, "0,50"), "set_follow_focus_margin", "get_follow_focus_margin");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "container_allow_vertical_scroll"), "set_allow_vertical_scroll", "get_allow_vertical_scroll");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "container_allow_horizontal_scroll"), "set_allow_horizontal_scroll", "get_allow_horizontal_scroll");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "container_auto_allow_scroll"), "set_auto_allow_scroll", "get_auto_allow_scroll");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "container_allow_overdragging"), "set_allow_overdragging", "get_allow_overdragging");

	// 滚动条组
	ClassDB::bind_method(D_METHOD("set_hide_scrollbar_over_time", "enable"), &SmoothScrollContainer::_set_hide_scrollbar_over_time);
	ClassDB::bind_method(D_METHOD("get_hide_scrollbar_over_time"), &SmoothScrollContainer::_get_hide_scrollbar_over_time);
	ClassDB::bind_method(D_METHOD("set_scrollbar_hide_time", "time"), &SmoothScrollContainer::set_scrollbar_hide_time);
	ClassDB::bind_method(D_METHOD("get_scrollbar_hide_time"), &SmoothScrollContainer::get_scrollbar_hide_time);
	ClassDB::bind_method(D_METHOD("set_scrollbar_fade_in_time", "time"), &SmoothScrollContainer::set_scrollbar_fade_in_time);
	ClassDB::bind_method(D_METHOD("get_scrollbar_fade_in_time"), &SmoothScrollContainer::get_scrollbar_fade_in_time);
	ClassDB::bind_method(D_METHOD("set_scrollbar_fade_out_time", "time"), &SmoothScrollContainer::set_scrollbar_fade_out_time);
	ClassDB::bind_method(D_METHOD("get_scrollbar_fade_out_time"), &SmoothScrollContainer::get_scrollbar_fade_out_time);
	ADD_GROUP("Scroll Bar", "scroll_bar_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_bar_hide_scrollbar_over_time"), "set_hide_scrollbar_over_time", "get_hide_scrollbar_over_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scroll_bar_scrollbar_hide_time"), "set_scrollbar_hide_time", "get_scrollbar_hide_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scroll_bar_scrollbar_fade_in_time"), "set_scrollbar_fade_in_time", "get_scrollbar_fade_in_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scroll_bar_scrollbar_fade_out_time"), "set_scrollbar_fade_out_time", "get_scrollbar_fade_out_time");

	// 输入组
	ClassDB::bind_method(D_METHOD("set_handle_input", "enable"), &SmoothScrollContainer::set_handle_input);
	ClassDB::bind_method(D_METHOD("get_handle_input"), &SmoothScrollContainer::get_handle_input);
	ADD_GROUP("Input", "input_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "input_handle_input"), "set_handle_input", "get_handle_input");

	// 调试组
	ClassDB::bind_method(D_METHOD("set_debug_mode", "enable"), &SmoothScrollContainer::set_debug_mode);
	ClassDB::bind_method(D_METHOD("get_debug_mode"), &SmoothScrollContainer::get_debug_mode);
	ADD_GROUP("Debug", "debug_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_debug_mode"), "set_debug_mode", "get_debug_mode");

	// 3. 绑定公共方法（供GDScript调用）
	ClassDB::bind_method(D_METHOD("scroll_x_to", "x_pos", "duration"), &SmoothScrollContainer::scroll_x_to, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_y_to", "y_pos", "duration"), &SmoothScrollContainer::scroll_y_to, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_page_up", "duration"), &SmoothScrollContainer::scroll_page_up, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_page_down", "duration"), &SmoothScrollContainer::scroll_page_down, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_page_left", "duration"), &SmoothScrollContainer::scroll_page_left, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_page_right", "duration"), &SmoothScrollContainer::scroll_page_right, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_vertically", "amount"), &SmoothScrollContainer::scroll_vertically);
	ClassDB::bind_method(D_METHOD("scroll_horizontally", "amount"), &SmoothScrollContainer::scroll_horizontally);
	ClassDB::bind_method(D_METHOD("scroll_to_top", "duration"), &SmoothScrollContainer::scroll_to_top, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_to_bottom", "duration"), &SmoothScrollContainer::scroll_to_bottom, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_to_left", "duration"), &SmoothScrollContainer::scroll_to_left, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("scroll_to_right", "duration"), &SmoothScrollContainer::scroll_to_right, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("ensure_control_visible", "control"), &SmoothScrollContainer::ensure_control_visible);

	BIND_ENUM_CONSTANT(WHEEL);
	BIND_ENUM_CONSTANT(BAR);
	BIND_ENUM_CONSTANT(DRAG);
}

// ------------------------------
// 生命周期函数
// ------------------------------
void SmoothScrollContainer::_ready() {
	// 初始化调试绘制
	if (debug_mode) {
		setup_debug_drawing();
	}

	// 初始化内容节点（获取非滚动条的子节点）
	for (int i = 0; i < get_child_count(); i++) {
		Node *child = get_child(i);
		if (!Object::cast_to<ScrollBar>(child)) {
			content_node = Object::cast_to<Control>(child);
			break;
		}
	}

	// 添加并初始化计时器
	add_child(scrollbar_hide_timer);
	if (hide_scrollbar_over_time) {
		scrollbar_hide_timer->start(scrollbar_hide_time);
	}

	// 绑定滚动条信号
	get_v_scroll_bar()->connect("gui_input", callable_mp(this, &SmoothScrollContainer::_scrollbar_input).bind(true));
	get_h_scroll_bar()->connect("gui_input", callable_mp(this, &SmoothScrollContainer::_scrollbar_input).bind(false));
	get_v_scroll_bar()->connect("mouse_entered", callable_mp(this, &SmoothScrollContainer::_mouse_on_scroll_bar).bind(true));
	get_v_scroll_bar()->connect("mouse_exited", callable_mp(this, &SmoothScrollContainer::_mouse_on_scroll_bar).bind(false));
	get_h_scroll_bar()->connect("mouse_entered", callable_mp(this, &SmoothScrollContainer::_mouse_on_scroll_bar).bind(true));
	get_h_scroll_bar()->connect("mouse_exited", callable_mp(this, &SmoothScrollContainer::_mouse_on_scroll_bar).bind(false));

	// 绑定焦点变化信号
	get_viewport()->connect("gui_focus_changed", callable_mp(this, &SmoothScrollContainer::_on_focus_changed));

	// 绑定节点添加信号
	get_tree()->connect("node_added", callable_mp(this, &SmoothScrollContainer::_on_node_added));
}

void SmoothScrollContainer::process(double delta) {

	// 处理滚动逻辑
	scroll(true, velocity.y, pos.y, delta);
	scroll(false, velocity.x, pos.x, delta);

	// 更新滚动状态
	update_scrollbars();
	update_is_scrolling();

	// 调试绘制
	if (debug_mode) {
		queue_redraw();
	}
}

// ------------------------------
// 输入处理
// ------------------------------
void SmoothScrollContainer::gui_input(const Ref<InputEvent> &event) {
	// 鼠标移动时显示滚动条
	Ref<InputEventMouseMotion> drag_event = event;
	Ref<InputEventScreenDrag> scene_drag_event = event;
	if (hide_scrollbar_over_time && drag_event.is_valid()) {
		show_scrollbars();
	}

	// 处理鼠标滚轮事件
	Ref<InputEventMouseButton> mouse_button = event;
	if (mouse_button.is_valid()) {
		switch (mouse_button->get_button_index()) {
			case MouseButton::WHEEL_DOWN:
				if (mouse_button->is_pressed()) {
					last_scroll_type = ScrollType::WHEEL;
					if (mouse_button->is_shift_pressed() || !should_scroll_vertical()) {
						if (should_scroll_horizontal()) {
							velocity.x -= speed * mouse_button->get_factor();
						}
					} else {
						if (should_scroll_vertical()) {
							velocity.y -= speed * mouse_button->get_factor();
						}
					}
					scroll_damper = wheel_scroll_damper;
					kill_scroll_to_tweens();
				}
				break;
			case MouseButton::WHEEL_UP:
				if (mouse_button->is_pressed()) {
					last_scroll_type = ScrollType::WHEEL;
					if (mouse_button->is_shift_pressed() || !should_scroll_vertical()) {
						if (should_scroll_horizontal()) {
							velocity.x += speed * mouse_button->get_factor();
						}
					} else {
						if (should_scroll_vertical()) {
							velocity.y += speed * mouse_button->get_factor();
						}
					}
					scroll_damper = wheel_scroll_damper;
					kill_scroll_to_tweens();
				}
				break;
			case MouseButton::WHEEL_LEFT:
				if (mouse_button->is_pressed()) {
					last_scroll_type = ScrollType::WHEEL;
					if (mouse_button->is_shift_pressed()) {
						if (should_scroll_vertical()) {
							velocity.y -= speed * mouse_button->get_factor();
						}
					} else {
						if (should_scroll_horizontal()) {
							velocity.x += speed * mouse_button->get_factor();
						}
					}
					scroll_damper = wheel_scroll_damper;
					kill_scroll_to_tweens();
				}
				break;
			case MouseButton::WHEEL_RIGHT:
				if (mouse_button->is_pressed()) {
					last_scroll_type = ScrollType::WHEEL;
					if (mouse_button->is_shift_pressed()) {
						if (should_scroll_vertical()) {
							velocity.y += speed * mouse_button->get_factor();
						}
					} else {
						if (should_scroll_horizontal()) {
							velocity.x -= speed * mouse_button->get_factor();
						}
					}
					scroll_damper = wheel_scroll_damper;
					kill_scroll_to_tweens();
				}
				break;
			case MouseButton::LEFT:
				if (mouse_button->is_pressed()) {
					if (!drag_with_mouse) {
						return;
					}
					content_dragging = true;
					is_in_deadzone = true;
					scroll_damper = dragging_scroll_damper;
					last_scroll_type = ScrollType::DRAG;
					init_drag_temp_data();
					kill_scroll_to_tweens();
				} else {
					content_dragging = false;
					is_in_deadzone = false;
				}
				break;
		}
	}

	// 处理拖动事件（鼠标或触摸）
	bool is_drag_event = (scene_drag_event.is_valid() && drag_with_touch) ||
			(drag_event.is_valid() && drag_with_mouse);
	if (is_drag_event && content_dragging) {
		if (should_scroll_horizontal()) {
			drag_temp_data[0] += drag_event->get_relative().x;
		}
		if (should_scroll_vertical()) {
			drag_temp_data[1] += drag_event->get_relative().y;
		}
		remove_all_children_focus(this);
		handle_content_dragging();
	}

	// 处理触摸事件
	Ref<InputEventScreenTouch> touch_event = event;
	if (touch_event.is_valid()) {
		if (touch_event->is_pressed()) {
			if (!drag_with_touch) {
				return;
			}
			content_dragging = true;
			is_in_deadzone = true;
			scroll_damper = dragging_scroll_damper;
			last_scroll_type = ScrollType::DRAG;
			init_drag_temp_data();
			kill_scroll_to_tweens();
		} else {
			content_dragging = false;
			is_in_deadzone = false;
		}
	}

	// 处理手势事件
	Ref<InputEventPanGesture> pan_event = event;
	if (pan_event.is_valid()) {
		if (should_scroll_horizontal()) {
			velocity.x = -pan_event->get_delta().x * speed;
			kill_scroll_to_tweens();
		}
		if (should_scroll_vertical()) {
			velocity.y = -pan_event->get_delta().y * speed;
			kill_scroll_to_tweens();
		}
	}

	// 标记输入为已处理
	if (handle_input) {
		get_viewport()->set_input_as_handled();
	}
}

// ------------------------------
// 绘制逻辑
// ------------------------------
void SmoothScrollContainer::_draw() {
	if (debug_mode) {
		draw_debug();
	}
}

// ------------------------------
// 属性Get/Set（用于脚本访问）
// ------------------------------
Variant SmoothScrollContainer::getvar(const Variant &property, bool *r_valid) const {
	if (property == "scroll_horizontal") {
		return content_node ? -content_node->get_position().x : 0;
	} else if (property == "scroll_vertical") {
		return content_node ? -content_node->get_position().y : 0;
	}
	return Variant();
}

void SmoothScrollContainer::setvar(const Variant &property, const Variant &value, bool *r_valid) {
	if (!content_node) {
		if (r_valid) {
			*r_valid = false;
		}
	}

	if (property == "scroll_horizontal") {
		float val = value;
		kill_scroll_x_to_tween();
		velocity.x = 0.0f;
		float size_x_diff = get_child_size_x_diff(content_node, true);
		pos.x = CLAMP(-val, -size_x_diff, 0.0f);
		if (r_valid) {
			*r_valid = true;
		}
	} else if (property == "scroll_vertical") {
		float val = value;
		kill_scroll_y_to_tween();
		velocity.y = 0.0f;
		float size_y_diff = get_child_size_y_diff(content_node, true);
		pos.y = CLAMP(-val, -size_y_diff, 0.0f);

		if (r_valid) {
			*r_valid = true;
		}
	}

	if (r_valid) {
		*r_valid = false;
	}
}

// ------------------------------
// 私有工具函数实现
// ------------------------------
void SmoothScrollContainer::_set_hide_scrollbar_over_time(bool p_value) {
	if (hide_scrollbar_over_time == p_value) {
		return;
	}
	hide_scrollbar_over_time = p_value;

	if (!p_value) {
		// 禁用时显示滚动条
		if (scrollbar_hide_timer) {
			scrollbar_hide_timer->stop();
		}
		if (scrollbar_show_tween.is_valid()) {
			scrollbar_show_tween->kill();
		}
		if (scrollbar_hide_tween.is_valid()) {
			scrollbar_hide_tween->kill();
		}
		get_h_scroll_bar()->set_modulate(Color(1, 1, 1));
		get_v_scroll_bar()->set_modulate(Color(1, 1, 1));
	} else {
		// 启用时启动计时器
		if (scrollbar_hide_timer && scrollbar_hide_timer->is_inside_tree()) {
			scrollbar_hide_timer->start(scrollbar_hide_time);
		}
	}
}

void SmoothScrollContainer::_scrollbar_hide_timer_timeout() {
	if (!any_scroll_bar_dragged()) {
		hide_scrollbars();
	}
}

void SmoothScrollContainer::_mouse_on_scroll_bar(bool entered) {
	mouse_on_scrollbar = entered;
	if (entered) {
		show_scrollbars();
	} else {
		if (hide_scrollbar_over_time && !is_scrolling) {
			scrollbar_hide_timer->start(scrollbar_hide_time);
		}
	}
}

void SmoothScrollContainer::_scrollbar_input(const Ref<InputEvent> &event, bool vertical) {
	Ref<InputEventMouseButton> mouse_button = event;
	Ref<InputEventScreenTouch> touch_event = event;
	if (mouse_button.is_valid()) {
		// 转发滚轮事件到容器
		if (mouse_button->get_button_index() >= MouseButton::WHEEL_DOWN &&
				mouse_button->get_button_index() <= MouseButton::WHEEL_RIGHT) {
			gui_input(event);
		}
		// 处理滚动条拖动
		if (mouse_button->get_button_index() == MouseButton::LEFT) {
			if (mouse_button->is_pressed()) {
				if (vertical) {
					v_scrollbar_dragging = true;
				} else {
					h_scrollbar_dragging = true;
				}
				last_scroll_type = ScrollType::BAR;
				kill_scroll_to_tweens();
			} else {
				if (vertical) {
					v_scrollbar_dragging = false;
				} else {
					h_scrollbar_dragging = false;
				}
			}
		}
	} else if (touch_event.is_valid()) {
		if (touch_event->is_pressed()) {
			if (vertical) {
				v_scrollbar_dragging = true;
			} else {
				h_scrollbar_dragging = true;
			}
			last_scroll_type = ScrollType::BAR;
			kill_scroll_to_tweens();
		} else {
			if (vertical) {
				v_scrollbar_dragging = false;
			} else {
				h_scrollbar_dragging = false;
			}
		}
	}
}

void SmoothScrollContainer::_on_focus_changed(Control *control) {
	if (control && is_ancestor_of(control)) {
		ensure_control_visible(control);
	}
}

void SmoothScrollContainer::_on_node_added(Node *node) {
	Control* control = Object::cast_to<Control>(node);
	if (Engine::get_singleton()->is_editor_hint() && control) {
		if (is_ancestor_of(control)) {
			control->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		}
	}
}

void SmoothScrollContainer::init_drag_temp_data() {
	if (!content_node) {
		return;
	}
	Vector2 size_diff = get_child_size_diff(content_node, true, true);
	Vector4 boundary_dist = get_child_boundary_dist(content_node->get_position(), size_diff);
	drag_temp_data[0] = 0.0f; // x相对移动
	drag_temp_data[1] = 0.0f; // y相对移动
	drag_temp_data[2] = content_node->get_position().x; // 初始x
	drag_temp_data[3] = content_node->get_position().y; // 初始y
	drag_temp_data[4] = boundary_dist.x; // 左距
	drag_temp_data[5] = boundary_dist.y; // 右距
	drag_temp_data[6] = boundary_dist.z; // 上距
	drag_temp_data[7] = boundary_dist.w; // 下距
}

void SmoothScrollContainer::handle_content_dragging() {
	if (!dragging_scroll_damper.is_valid()) {
		return;
	}

	// 处理死区逻辑
	Vector2 drag_delta(drag_temp_data[0], drag_temp_data[1]);
	if (drag_delta.length() < scroll_deadzone && is_in_deadzone) {
		return;
	} else if (is_in_deadzone) {
		is_in_deadzone = false;
		drag_temp_data[0] = 0.0f;
		drag_temp_data[1] = 0.0f;
		return;
	}

	content_dragging_moved = true;

	// 计算拖动位置（模拟GDScript的匿名函数逻辑）
	auto calculate_dest = [&](float delta) {
		float damping = dragging_scroll_damper->get("attract_factor");
		if (delta >= 0.0f) {
			return delta / (1 + delta * damping * 0.00001f);
		} else {
			return delta / (1 - delta * damping * 0.00001f);
		}
	};

	auto calculate_position = [&](float temp_dist1, float temp_dist2, float temp_relative) {
		if (temp_relative + temp_dist1 > 0.0f) {
			float delta = MIN(temp_relative, temp_relative + temp_dist1);
			float dest = calculate_dest(delta);
			return dest - MIN(0.0f, temp_dist1);
		} else if (temp_relative + temp_dist2 < 0.0f) {
			float delta = MAX(temp_relative, temp_relative + temp_dist2);
			float dest = -calculate_dest(-delta);
			return dest - MAX(0.0f, temp_dist2);
		} else {
			return temp_relative;
		}
	};

	// 更新X位置
	if (should_scroll_horizontal()) {
		double x_pos = calculate_position(
							  drag_temp_data[4], // 左距
							  drag_temp_data[5], // 右距
							  drag_temp_data[0] // x相对移动
							  ) +
				drag_temp_data[2]; // 初始x
		velocity.x = (x_pos - pos.x) / get_process_delta_time();
		pos.x = x_pos;
	}

	// 更新Y位置
	if (should_scroll_vertical()) {
		double y_pos = calculate_position(
							  drag_temp_data[6], // 上距
							  drag_temp_data[7], // 下距
							  drag_temp_data[1] // y相对移动
							  ) +
				drag_temp_data[3]; // 初始y
		velocity.y = (y_pos - pos.y) / get_process_delta_time();
		pos.y = y_pos;
	}
}

void SmoothScrollContainer::remove_all_children_focus(Node *node) {
	Control* control = Object::cast_to<Control>(node);
	if (control) {
		control->release_focus();
	}
	for (int i = 0; i < node->get_child_count(); i++) {
		remove_all_children_focus(node->get_child(i));
	}
}

void SmoothScrollContainer::update_is_scrolling() {
	bool new_scrolling = (content_dragging && !is_in_deadzone) ||
			any_scroll_bar_dragged() ||
			velocity.length_squared() > 0.001f ||
			has_active_scroll_tween();

	if (is_scrolling != new_scrolling) {
		is_scrolling = new_scrolling;
		if (is_scrolling) {
			_on_scroll_started();
		} else {
			_on_scroll_ended();
		}
	}
}

void SmoothScrollContainer::update_scrollbars() {
	// 更新滚动条值
	if (get_v_scroll_bar()->get_value() != -pos.y) {
		get_v_scroll_bar()->set_value_no_signal(-pos.y);
		get_v_scroll_bar()->queue_redraw();
	}
	if (get_h_scroll_bar()->get_value() != -pos.x) {
		get_h_scroll_bar()->set_value_no_signal(-pos.x);
		get_h_scroll_bar()->queue_redraw();
	}

	// 滚动或鼠标在滚动条上时显示滚动条
	if (hide_scrollbar_over_time && (is_scrolling || mouse_on_scrollbar)) {
		show_scrollbars();
	}
}

void SmoothScrollContainer::kill_scroll_x_to_tween() {
	if (scroll_x_to_tween.is_valid() && scroll_x_to_tween->is_running()) {
		scroll_x_to_tween->kill();
	}
}

void SmoothScrollContainer::kill_scroll_y_to_tween() {
	if (scroll_y_to_tween.is_valid() && scroll_y_to_tween->is_running()) {
		scroll_y_to_tween->kill();
	}
}

void SmoothScrollContainer::kill_scroll_to_tweens() {
	kill_scroll_x_to_tween();
	kill_scroll_y_to_tween();
}

void SmoothScrollContainer::setup_debug_drawing() {
	// 调试渐变已在构造函数中初始化
}

void SmoothScrollContainer::draw_debug() {
	if (!content_node) {
		return;
	}

	// 计算边界距离
	Vector2 size_diff = get_child_size_diff(content_node, false, false);
	Vector4 boundary_dist = get_child_boundary_dist(content_node->get_position(), size_diff);
	float bottom_distance = boundary_dist.w;
	float top_distance = boundary_dist.z;
	float right_distance = boundary_dist.y;
	float left_distance = boundary_dist.x;

	// 绘制过拖动边界线
	Color top_color = debug_gradient->get_color_at_offset(CLAMP(top_distance / get_size().y, 0.0f, 1.0f));
	draw_line(Vector2(0, 0), Vector2(0, top_distance), top_color, 5.0f);

	Color bottom_color = debug_gradient->get_color_at_offset(CLAMP(-bottom_distance / get_size().y, 0.0f, 1.0f));
	draw_line(Vector2(0, get_size().y), Vector2(0, get_size().y + bottom_distance), bottom_color, 5.0f);

	Color left_color = debug_gradient->get_color_at_offset(CLAMP(left_distance / get_size().x, 0.0f, 1.0f));
	draw_line(Vector2(0, get_size().y), Vector2(left_distance, get_size().y), left_color, 5.0f);

	Color right_color = debug_gradient->get_color_at_offset(CLAMP(-right_distance / get_size().x, 0.0f, 1.0f));
	draw_line(Vector2(get_size().x, get_size().y), Vector2(get_size().x + right_distance, get_size().y), right_color, 5.0f);

	// 绘制速度向量
	Vector2 origin(5.0f, get_size().y / 2);
	draw_line(origin, origin + Vector2(0, velocity.y * 0.01f), Color(0, 1, 0), 5.0f);
	draw_line(origin, origin + Vector2(velocity.x * 0.01f, 0), Color(1, 0, 0), 5.0f);
}

bool SmoothScrollContainer::handle_scrollbar_drag() {
	if (h_scrollbar_dragging) {
		velocity.x = 0.0f;
		pos.x = -get_h_scroll_bar()->get_value();
		return true;
	}
	if (v_scrollbar_dragging) {
		velocity.y = 0.0f;
		pos.y = -get_v_scroll_bar()->get_value();
		return true;
	}
	return false;
}

float SmoothScrollContainer::handle_overdrag(bool vertical, float axis_velocity, float axis_pos, float delta) {
	if (!scroll_damper.is_valid()) {
		return 0.0f;
	}

	float size_diff = vertical ? get_child_size_y_diff(content_node, true) : get_child_size_x_diff(content_node, true);
	float dist1 = vertical ? get_child_top_dist(axis_pos, size_diff) : get_child_left_dist(axis_pos, size_diff);
	float dist2 = vertical ? get_child_bottom_dist(axis_pos, size_diff) : get_child_right_dist(axis_pos, size_diff);

	// 计算目标速度（模拟ScrollDamper的_calculate_velocity_to_dest）
	auto calculate_velocity = [&](float distance) {
		return scroll_damper->call("_calculate_velocity_to_dest", distance, 0.0f);
	};

	float target_vel1 = calculate_velocity(dist1);
	float target_vel2 = calculate_velocity(dist2);

	// 过拖动时应用反向力
	if (axis_pos > 0.0f && axis_velocity > target_vel1) {
		axis_velocity = scroll_damper->call("attract", dist1, 0.0f, axis_velocity, delta);
	} else if (axis_pos < -size_diff && axis_velocity < target_vel2) {
		axis_velocity = scroll_damper->call("attract", dist2, 0.0f, axis_velocity, delta);
	}

	return axis_velocity;
}

Vector2 SmoothScrollContainer::snap(bool vertical, float axis_velocity, float axis_pos) {
	float size_diff = vertical ? get_child_size_y_diff(content_node, true) : get_child_size_x_diff(content_node, true);
	float dist1 = vertical ? get_child_top_dist(axis_pos, size_diff) : get_child_left_dist(axis_pos, size_diff);
	float dist2 = vertical ? get_child_bottom_dist(axis_pos, size_diff) : get_child_right_dist(axis_pos, size_diff);

	// 接近边界时吸附
	if (dist1 > 0.0f && Math::abs(dist1) < just_snap_under && Math::abs(axis_velocity) < just_snap_under) {
		axis_pos -= dist1;
		axis_velocity = 0.0f;
	} else if (dist2 < 0.0f && Math::abs(dist2) < just_snap_under && Math::abs(axis_velocity) < just_snap_under) {
		axis_pos -= dist2;
		axis_velocity = 0.0f;
	}

	return Vector2(axis_velocity, axis_pos);
}

void SmoothScrollContainer::scroll(bool vertical, float axis_velocity, float axis_pos, float delta) {
	// 检查是否允许滚动
	if ((vertical && !should_scroll_vertical()) || (!vertical && !should_scroll_horizontal())) {
		return;
	}
	if (!scroll_damper.is_valid()) {
		return;
	}

	// 处理过拖动
	if (!content_dragging) {
		axis_velocity = handle_overdrag(vertical, axis_velocity, axis_pos, delta);
		// 应用阻尼
		Vector2 slide_result = scroll_damper->call("slide", axis_velocity, delta);
		axis_velocity = slide_result.x;
		axis_pos += slide_result.y;
		// 吸附到边界
		Vector2 snap_result = snap(vertical, axis_velocity, axis_pos);
		axis_velocity = snap_result.x;
		axis_pos = snap_result.y;
	} else {
		// 拖动时重置速度（除非有移动）
		if (content_dragging_moved) {
			content_dragging_moved = false;
		} else {
			axis_velocity = 0.0f;
		}
	}

	// 处理滚动条拖动（优先级最高）
	if (handle_scrollbar_drag()) {
		return;
	}

	// 限制位置（如果禁用过拖动）
	if (!allow_overdragging) {
		if (vertical) {
			if (is_outside_top_boundary(axis_pos)) {
				axis_pos = 0.0f;
				axis_velocity = 0.0f;
			} else if (is_outside_bottom_boundary(axis_pos)) {
				axis_pos = -get_child_size_y_diff(content_node, true);
				axis_velocity = 0.0f;
			}
		} else {
			if (is_outside_left_boundary(axis_pos)) {
				axis_pos = 0.0f;
				axis_velocity = 0.0f;
			} else if (is_outside_right_boundary(axis_pos)) {
				axis_pos = -get_child_size_x_diff(content_node, true);
				axis_velocity = 0.0f;
			}
		}
	}

	// 更新内容位置
	if (!content_node) {
		return;
	}
	if (vertical) {
		content_node->set_position(Vector2(content_node->get_position().x, axis_pos));
		pos.y = axis_pos;
		velocity.y = axis_velocity;
	} else {
		content_node->set_position(Vector2(axis_pos, content_node->get_position().y));
		pos.x = axis_pos;
		velocity.x = axis_velocity;
	}
}

// ------------------------------
// 尺寸计算工具函数实现
// ------------------------------
float SmoothScrollContainer::get_spare_size_x() {
	float size_x = get_size().x;
	if (get_v_scroll_bar()->is_visible()) {
		size_x -= get_v_scroll_bar()->get_size().x;
	}
	return MAX(size_x, 0.0f);
}

float SmoothScrollContainer::get_spare_size_y() {
	float size_y = get_size().y;
	if (get_h_scroll_bar()->is_visible()) {
		size_y -= get_h_scroll_bar()->get_size().y;
	}
	return MAX(size_y, 0.0f);
}

Vector2 SmoothScrollContainer::get_spare_size() {
	return Vector2(get_spare_size_x(), get_spare_size_y());
}

float SmoothScrollContainer::get_child_size_x_diff(Control *child, bool clamp) {
	if (!child) {
		return 0.0f;
	}
	float child_size_x = child->get_size().x * child->get_scale().x;
	if (clamp) {
		child_size_x = MAX(child_size_x, get_spare_size_x());
	}
	return child_size_x - get_spare_size_x();
}

float SmoothScrollContainer::get_child_size_y_diff(Control *child, bool clamp) {
	if (!child) {
		return 0.0f;
	}
	float child_size_y = child->get_size().y * child->get_scale().y;
	if (clamp) {
		child_size_y = MAX(child_size_y, get_spare_size_y());
	}
	return child_size_y - get_spare_size_y();
}

Vector2 SmoothScrollContainer::get_child_size_diff(Control *child, bool clamp_x, bool clamp_y) {
	return Vector2(
			get_child_size_x_diff(child, clamp_x),
			get_child_size_y_diff(child, clamp_y));
}

float SmoothScrollContainer::get_child_left_dist(float child_pos_x, float child_size_diff_x) const {
	return child_pos_x;
}

float SmoothScrollContainer::get_child_right_dist(float child_pos_x, float child_size_diff_x) const {
	return child_pos_x + child_size_diff_x;
}

float SmoothScrollContainer::get_child_top_dist(float child_pos_y, float child_size_diff_y) const {
	return child_pos_y;
}

float SmoothScrollContainer::get_child_bottom_dist(float child_pos_y, float child_size_diff_y) const {
	return child_pos_y + child_size_diff_y;
}

Vector4 SmoothScrollContainer::get_child_boundary_dist(const Vector2 &child_pos, const Vector2 &child_size_diff) const {
	return Vector4(
			get_child_left_dist(child_pos.x, child_size_diff.x),
			get_child_right_dist(child_pos.x, child_size_diff.x),
			get_child_top_dist(child_pos.y, child_size_diff.y),
			get_child_bottom_dist(child_pos.y, child_size_diff.y));
}

// ------------------------------
// 滚动判断工具函数实现
// ------------------------------
bool SmoothScrollContainer::should_scroll_vertical() {
	if (!allow_vertical_scroll || !scroll_damper.is_valid()) {
		return false;
	}
	if (auto_allow_scroll && get_child_size_y_diff(content_node, false) <= 0) {
		return false;
	}
	return true;
}

bool SmoothScrollContainer::should_scroll_horizontal() {
	if (!allow_horizontal_scroll || !scroll_damper.is_valid()) {
		return false;
	}
	if (auto_allow_scroll && get_child_size_x_diff(content_node, false) <= 0) {
		return false;
	}
	return true;
}

bool SmoothScrollContainer::any_scroll_bar_dragged() const {
	return h_scrollbar_dragging || v_scrollbar_dragging;
}

bool SmoothScrollContainer::is_outside_top_boundary(float y_pos) {
	if (y_pos < 0) {
		y_pos = pos.y;
	}
	float size_y_diff = get_child_size_y_diff(content_node, true);
	return get_child_top_dist(y_pos, size_y_diff) > 0.0f;
}

bool SmoothScrollContainer::is_outside_bottom_boundary(float y_pos) {
	if (y_pos < 0) {
		y_pos = pos.y;
	}
	float size_y_diff = get_child_size_y_diff(content_node, true);
	return get_child_bottom_dist(y_pos, size_y_diff) < 0.0f;
}

bool SmoothScrollContainer::is_outside_left_boundary(float x_pos) {
	if (x_pos < 0) {
		x_pos = pos.x;
	}
	float size_x_diff = get_child_size_x_diff(content_node, true);
	return get_child_left_dist(x_pos, size_x_diff) > 0.0f;
}

bool SmoothScrollContainer::is_outside_right_boundary(float x_pos) {
	if (x_pos < 0) {
		x_pos = pos.x;
	}
	float size_x_diff = get_child_size_x_diff(content_node, true);
	return get_child_right_dist(x_pos, size_x_diff) < 0.0f;
}
bool SmoothScrollContainer::has_active_scroll_tween() {
	return (scroll_x_to_tween.is_valid() && scroll_x_to_tween->is_running())
		|| (scroll_y_to_tween.is_valid() && scroll_y_to_tween->is_running());

}
// ------------------------------
// 滚动条动画函数实现
// ------------------------------
void SmoothScrollContainer::show_scrollbars(float time) {
	if (time < 0) {
		time = scrollbar_fade_in_time;
	}
	scrollbar_hide_timer->start(scrollbar_hide_time);


	// 启动显示动画
	if ((get_v_scroll_bar()->get_modulate() != Color(1, 1, 1) || get_h_scroll_bar()->get_modulate() != Color(1, 1, 1))) {
		// 停止隐藏动画
		if (scrollbar_hide_tween.is_valid() && scrollbar_hide_tween->is_running()) {
			scrollbar_hide_tween->kill();
		}
		scrollbar_show_tween = create_tween();
		scrollbar_show_tween->set_parallel(true);
		scrollbar_show_tween->tween_property(get_v_scroll_bar(), NodePath("modulate"), Color(1, 1, 1), time);
		scrollbar_show_tween->tween_property(get_h_scroll_bar(), NodePath("modulate"), Color(1, 1, 1), time);
	}
}

void SmoothScrollContainer::hide_scrollbars(float time) {
	if (time < 0) {
		time = scrollbar_fade_out_time;
	}

	// 启动隐藏动画
	if ((get_v_scroll_bar()->get_modulate() != Color(0, 0, 0, 0) || get_h_scroll_bar()->get_modulate() != Color(0, 0, 0, 0))) {

	// 停止显示动画
	if (scrollbar_show_tween.is_valid() && scrollbar_show_tween->is_running()) {
		scrollbar_show_tween->kill();
	}
		scrollbar_hide_tween = create_tween();
		scrollbar_hide_tween->set_parallel(true);
		scrollbar_hide_tween->tween_property(get_v_scroll_bar(), NodePath("modulate"), Color(0, 0, 0, 0), time);
		scrollbar_hide_tween->tween_property(get_h_scroll_bar(), NodePath("modulate"), Color(0, 0, 0, 0), time);
	}
}

// ------------------------------
// 公共API实现
// ------------------------------
void SmoothScrollContainer::scroll_x_to(float x_pos, float duration) {
	if (!should_scroll_horizontal() || content_dragging) {
		return;
	}
	velocity.x = 0.0f;
	float size_x_diff = get_child_size_x_diff(content_node, true);
	x_pos = CLAMP(x_pos, -size_x_diff, 0.0f);
	kill_scroll_x_to_tween();
	scroll_x_to_tween = create_tween();
	scroll_x_to_tween->tween_property(this, NodePath("pos:x"), x_pos, duration)
			->set_ease(Tween::EASE_OUT)
			->set_trans(Tween::TRANS_QUINT);
}

void SmoothScrollContainer::scroll_y_to(float y_pos, float duration) {
	if (!should_scroll_vertical() || content_dragging) {
		return;
	}
	velocity.y = 0.0f;
	float size_y_diff = get_child_size_y_diff(content_node, true);
	y_pos = CLAMP(y_pos, -size_y_diff, 0.0f);
	kill_scroll_y_to_tween();
	scroll_y_to_tween = create_tween();
	scroll_y_to_tween->tween_property(this, NodePath("pos:y"), y_pos, duration)
			->set_ease(Tween::EASE_OUT)
			->set_trans(Tween::TRANS_QUINT);
}

void SmoothScrollContainer::scroll_page_up(float duration) {
	if (!content_node) {
		return;
	}
	float destination = content_node->get_position().y + get_spare_size_y();
	scroll_y_to(destination, duration);
}

void SmoothScrollContainer::scroll_page_down(float duration) {
	if (!content_node) {
		return;
	}
	float destination = content_node->get_position().y - get_spare_size_y();
	scroll_y_to(destination, duration);
}

void SmoothScrollContainer::scroll_page_left(float duration) {
	if (!content_node) {
		return;
	}
	float destination = content_node->get_position().x + get_spare_size_x();
	scroll_x_to(destination, duration);
}

void SmoothScrollContainer::scroll_page_right(float duration) {
	if (!content_node) {
		return;
	}
	float destination = content_node->get_position().x - get_spare_size_x();
	scroll_x_to(destination, duration);
}

void SmoothScrollContainer::scroll_vertically(float amount) {
	velocity.y -= amount;
}

void SmoothScrollContainer::scroll_horizontally(float amount) {
	velocity.x -= amount;
}

void SmoothScrollContainer::scroll_to_top(float duration) {
	scroll_y_to(0.0f, duration);
}

void SmoothScrollContainer::scroll_to_bottom(float duration) {
	if (!content_node) {
		return;
	}
	scroll_y_to(get_spare_size_y() - content_node->get_size().y, duration);
}

void SmoothScrollContainer::scroll_to_left(float duration) {
	scroll_x_to(0.0f, duration);
}

void SmoothScrollContainer::scroll_to_right(float duration) {
	if (!content_node) {
		return;
	}
	scroll_x_to(get_spare_size_x() - content_node->get_size().x, duration);
}

void SmoothScrollContainer::ensure_control_visible(Control *control) {
	if (!content_node || !control || !content_node->is_ancestor_of(control)) {
		return;
	}
	if (!scroll_damper.is_valid()) {
		return;
	}

	// 计算控制节点在容器中的位置
	Vector2 control_global_pos = control->get_global_position();
	Vector2 container_global_pos = get_global_position();
	Vector2 control_pos_in_container = (control_global_pos - container_global_pos) / (get_global_rect().size / get_size());

	Vector2 control_size = control->get_global_rect().size;
	Vector2 container_size = get_global_rect().size;
	Vector2 size_diff = (control_size - container_size) / (container_size / get_size());

	Vector4 boundary_dist = get_child_boundary_dist(control_pos_in_container, size_diff);
	Vector2 content_pos = content_node->get_position();

	// 水平滚动调整
	if (boundary_dist.x < 0 + follow_focus_margin) {
		scroll_x_to(content_pos.x - boundary_dist.x + follow_focus_margin);
	} else if (boundary_dist.y > 0 - follow_focus_margin) {
		scroll_x_to(content_pos.x - boundary_dist.y - follow_focus_margin);
	}

	// 垂直滚动调整
	if (boundary_dist.z < 0 + follow_focus_margin) {
		scroll_y_to(content_pos.y - boundary_dist.z + follow_focus_margin);
	} else if (boundary_dist.w > 0 - follow_focus_margin) {
		scroll_y_to(content_pos.y - boundary_dist.w - follow_focus_margin);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LinearScrollDamper::_bind_methods() {
	// 绑定摩擦系数属性
	ClassDB::bind_method(D_METHOD("set_friction", "friction"), &LinearScrollDamper::set_friction);
	ClassDB::bind_method(D_METHOD("get_friction"), &LinearScrollDamper::get_friction);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "friction", PROPERTY_HINT_RANGE, "0.001,10000.0,0.001,or_greater,hide_slider"),
			"set_friction",
			"get_friction");
}

LinearScrollDamper::LinearScrollDamper() {
	// 初始化因子
	_factor = pow(10.0, friction) - 1.0;
}

void LinearScrollDamper::set_friction(float p_friction) {
	friction = MAX(p_friction, 0.001f);
	_factor = pow(10.0, friction) - 1.0;
	_factor = MAX(_factor, 0.000000000001f);
}

float LinearScrollDamper::get_friction() const {
	return friction;
}

float LinearScrollDamper::_calculate_velocity_by_time(float time) const {
	if (time <= 0.0f) {
		return 0.0f;
	}
	return time * _factor;
}

float LinearScrollDamper::_calculate_time_by_velocity(float velocity) const {
	return Math::abs(velocity) / _factor;
}

float LinearScrollDamper::_calculate_offset_by_time(float time) const {
	time = MAX(time, 0.0f);
	return 0.5f * _factor * time * time;
}

float LinearScrollDamper::_calculate_time_by_offset(float offset) const {
	return sqrt(Math::abs(offset) * 2.0f / _factor);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExpoScrollDamper::_bind_methods() {
	// Bind friction property
	ClassDB::bind_method(D_METHOD("set_friction", "friction"), &ExpoScrollDamper::set_friction);
	ClassDB::bind_method(D_METHOD("get_friction"), &ExpoScrollDamper::get_friction);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "friction", PROPERTY_HINT_RANGE, "0.001,10000.0,0.001,or_greater,hide_slider"),
			"set_friction",
			"get_friction");

	// Bind minimum velocity property
	ClassDB::bind_method(D_METHOD("set_minimum_velocity", "min_vel"), &ExpoScrollDamper::set_minimum_velocity);
	ClassDB::bind_method(D_METHOD("get_minimum_velocity"), &ExpoScrollDamper::get_minimum_velocity);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "minimum_velocity", PROPERTY_HINT_RANGE, "0.001,100000.0,0.001,or_greater,hide_slider"),
			"set_minimum_velocity",
			"get_minimum_velocity");
}

ExpoScrollDamper::ExpoScrollDamper() {
	// Initialize factor based on default friction
	_factor = pow(10.0, friction);
	_factor = MAX(_factor, 1.000000000001f);
}

void ExpoScrollDamper::set_friction(float p_friction) {
	friction = MAX(p_friction, 0.001f);
	_factor = pow(10.0, friction);
	_factor = MAX(_factor, 1.000000000001f);
}

float ExpoScrollDamper::get_friction() const {
	return friction;
}

void ExpoScrollDamper::set_minimum_velocity(float p_min_vel) {
	minimum_velocity = MAX(p_min_vel, 0.001f);
}

float ExpoScrollDamper::get_minimum_velocity() const {
	return minimum_velocity;
}

float ExpoScrollDamper::_calculate_velocity_by_time(float time) const {
	float minimum_time = _calculate_time_by_velocity(minimum_velocity);
	if (time <= minimum_time) {
		return 0.0f;
	}
	return pow(_factor, time);
}

float ExpoScrollDamper::_calculate_time_by_velocity(float velocity) const {
	float abs_vel = Math::abs(velocity);
	if (abs_vel < 0.0001f) {
		return 0.0f;
	}
	return log(abs_vel) / log(_factor);
}

float ExpoScrollDamper::_calculate_offset_by_time(float time) const {
	return pow(_factor, time) / log(_factor);
}

float ExpoScrollDamper::_calculate_time_by_offset(float offset) const {
	float abs_offset = Math::abs(offset);
	if (abs_offset < 0.0001f) {
		return 0.0f;
	}
	return log(abs_offset * log(_factor)) / log(_factor);
}

float ExpoScrollDamper::_calculate_velocity_to_dest(float from, float to) const {
	float dist = to - from;
	float min_time = _calculate_time_by_velocity(minimum_velocity);
	float min_offset = _calculate_offset_by_time(min_time);
	float time = _calculate_time_by_offset(Math::abs(dist) + min_offset);
	float vel = _calculate_velocity_by_time(time) * Math::sign(dist);
	return vel;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 绑定方法与属性（Godot 反射系统核心）
void CubicScrollDamper::_bind_methods() {
	// 绑定 friction 属性的 Setter/Getter
	ClassDB::bind_method(D_METHOD("set_friction", "friction"), &CubicScrollDamper::set_friction);
	ClassDB::bind_method(D_METHOD("get_friction"), &CubicScrollDamper::get_friction);
	// 注册属性到编辑器，保持与 GDScript 一致的范围限制
	ADD_PROPERTY(
			PropertyInfo(
					Variant::FLOAT,
					"friction",
					PROPERTY_HINT_RANGE,
					"0.001,10000.0,0.001,or_greater,hide_slider"),
			"set_friction",
			"get_friction");
}

// 构造函数：初始化默认值
CubicScrollDamper::CubicScrollDamper() {
	// 按照 GDScript 逻辑计算初始 _factor
	_factor = pow(10.0, friction) - 1.0;
	// 确保 _factor 不小于极小值，避免后续计算出错
	_factor = MAX(_factor, 0.000000000001f);
}

// friction 属性 Setter：更新值时同步更新 _factor
void CubicScrollDamper::set_friction(float p_friction) {
	// 限制摩擦系数最小值为 0.001（与 GDScript 一致）
	friction = MAX(p_friction, 0.001f);
	// 重新计算因子
	_factor = pow(10.0, friction) - 1.0;
	// 确保因子下限
	_factor = MAX(_factor, 0.000000000001f);
}

// 实现抽象方法：根据时间计算速度（三次方逻辑）
float CubicScrollDamper::_calculate_velocity_by_time(float time) const {
	if (time <= 0.0f) {
		return 0.0f;
	}
	// 公式：速度 = 时间³ × 因子（对应 GDScript 的 time*time*time * _factor）
	return time * time * time * _factor;
}

// 实现抽象方法：根据速度计算时间（三次方根）
float CubicScrollDamper::_calculate_time_by_velocity(float velocity) const {
	float abs_vel = Math::abs(velocity);
	// 避免极小值导致计算异常
	if (abs_vel < 0.0001f) {
		return 0.0f;
	}
	// 公式：时间 = (速度绝对值 / 因子)^(1/3)（对应 GDScript 的 pow(abs(velocity)/_factor, 1/3)）
	return pow(abs_vel / _factor, 1.0f / 3.0f);
}

// 实现抽象方法：根据时间计算位移（四次方逻辑）
float CubicScrollDamper::_calculate_offset_by_time(float time) const {
	// 确保时间不为负
	time = MAX(time, 0.0f);
	// 公式：位移 = 1/4 × 因子 × 时间⁴（对应 GDScript 的 1.0/4.0 * _factor * time^4）
	return 0.25f * _factor * time * time * time * time;
}

// 实现抽象方法：根据位移计算时间（四次方根）
float CubicScrollDamper::_calculate_time_by_offset(float offset) const {
	float abs_offset = Math::abs(offset);
	// 避免极小值导致计算异常
	if (abs_offset < 0.0001f) {
		return 0.0f;
	}
	// 公式：时间 = (位移绝对值 × 4 / 因子)^(1/4)（对应 GDScript 的 pow(abs(offset)*4/_factor, 1/4)）
	return pow(abs_offset * 4.0f / _factor, 1.0f / 4.0f);
}
