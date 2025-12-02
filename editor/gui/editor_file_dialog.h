/**************************************************************************/
/*  editor_file_dialog.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "scene/gui/file_dialog.h"

class DependencyRemoveDialog;

class EditorFileDialog : public FileDialog {
	GDCLASS(EditorFileDialog, FileDialog);

<<<<<<< HEAD
public:
	enum DisplayMode {
		DISPLAY_THUMBNAILS,
		DISPLAY_LIST
	};

	enum Access {
		ACCESS_RESOURCES,
		ACCESS_USERDATA,
		ACCESS_FILESYSTEM
	};

	enum FileMode {
		FILE_MODE_OPEN_FILE,
		FILE_MODE_OPEN_FILES,
		FILE_MODE_OPEN_DIR,
		FILE_MODE_OPEN_ANY,
		FILE_MODE_SAVE_FILE
	};

	typedef Ref<Texture2D> (*GetIconFunc)(const String &);
	typedef void (*RegisterFunc)(EditorFileDialog *);

	static inline GetIconFunc get_icon_func = nullptr;
	static inline GetIconFunc get_thumbnail_func = nullptr;
	static inline RegisterFunc register_func = nullptr;
	static inline RegisterFunc unregister_func = nullptr;

private:
	enum ItemMenu {
		ITEM_MENU_COPY_PATH,
		ITEM_MENU_DELETE,
		ITEM_MENU_REFRESH,
		ITEM_MENU_NEW_FOLDER,
		ITEM_MENU_SHOW_IN_EXPLORER,
		ITEM_MENU_SHOW_BUNDLE_CONTENT,
	};

	ConfirmationDialog *makedialog = nullptr;
	LineEdit *makedirname = nullptr;

	VSeparator *makedir_sep = nullptr;
	Button *makedir = nullptr;
	Access access = ACCESS_RESOURCES;

	HFlowContainer *flow_checkbox_options = nullptr;
	GridContainer *grid_select_options = nullptr;
	VBoxContainer *vbox = nullptr;
	FileMode mode = FILE_MODE_SAVE_FILE;
	bool can_create_dir = false;
	LineEdit *dir = nullptr;

	Button *dir_prev = nullptr;
	Button *dir_next = nullptr;
	Button *dir_up = nullptr;

	HBoxContainer *drives_container = nullptr;
	HBoxContainer *shortcuts_container = nullptr;
	OptionButton *drives = nullptr;
	ItemList *item_list = nullptr;
	PopupMenu *item_menu = nullptr;
	TextureRect *preview = nullptr;
	VBoxContainer *preview_vb = nullptr;
	HSplitContainer *body_hsplit = nullptr;
	HSplitContainer *list_hb = nullptr;
	HBoxContainer *file_box = nullptr;
	LineEdit *file = nullptr;
	OptionButton *filter = nullptr;
	AcceptDialog *error_dialog = nullptr;
	Ref<DirAccess> dir_access;
	ConfirmationDialog *confirm_save = nullptr;
	DependencyRemoveDialog *dep_remove_dialog = nullptr;
	ConfirmationDialog *global_remove_dialog = nullptr;
	VBoxContainer *vbc = nullptr;
	HBoxContainer *pathhb = nullptr;

	Button *mode_thumbnails = nullptr;
	Button *mode_list = nullptr;

	Button *refresh = nullptr;
	Button *favorite = nullptr;
	Button *show_hidden = nullptr;
	Button *show_search_filter_button = nullptr;

	String search_string;
	bool show_search_filter = false;
	HBoxContainer *filter_hb = nullptr;
	LineEdit *filter_box = nullptr;
	FileSortOption file_sort = FileSortOption::FILE_SORT_NAME;
	MenuButton *file_sort_button = nullptr;

	Button *fav_up = nullptr;
	Button *fav_down = nullptr;

	ItemList *favorites = nullptr;
	ItemList *recent = nullptr;

	Vector<String> local_history;
	int local_history_pos = 0;
	void _push_history();

	Vector<String> filters;
	Vector<String> processed_filters;

	bool previews_enabled = true;
	bool preview_waiting = false;
	int preview_wheel_index = 0;
	float preview_wheel_timeout = 0.0f;

	static inline bool default_show_hidden_files = false;
	static inline DisplayMode default_display_mode = DISPLAY_THUMBNAILS;
	bool show_hidden_files;
	DisplayMode display_mode;

	bool disable_overwrite_warning = false;
	bool is_invalidating = false;

	struct ThemeCache {
		Ref<Texture2D> parent_folder;
		Ref<Texture2D> forward_folder;
		Ref<Texture2D> back_folder;
		Ref<Texture2D> open_folder;
		Ref<Texture2D> reload;
		Ref<Texture2D> toggle_hidden;
		Ref<Texture2D> toggle_filename_filter;
		Ref<Texture2D> favorite;
		Ref<Texture2D> mode_thumbnails;
		Ref<Texture2D> mode_list;
		Ref<Texture2D> create_folder;
		Ref<Texture2D> favorites_up;
		Ref<Texture2D> favorites_down;

		Ref<Texture2D> filter_box;
		Ref<Texture2D> file_sort_button;

		Ref<Texture2D> file;
		Ref<Texture2D> folder;
		Color folder_icon_color;

		Ref<Texture2D> action_copy;
		Ref<Texture2D> action_delete;
		Ref<Texture2D> filesystem;

		Ref<Texture2D> folder_medium_thumbnail;
		Ref<Texture2D> file_medium_thumbnail;
		Ref<Texture2D> folder_big_thumbnail;
		Ref<Texture2D> file_big_thumbnail;

		Ref<Texture2D> progress[8]{};
	} theme_cache;

	struct Option {
		String name;
		Vector<String> values;
		int default_idx = 0;
	};

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	Vector<Option> options;
	Dictionary selected_options;
	bool options_dirty = false;
	String full_dir;

	void update_dir();
	void update_file_name();
	void update_file_list();
	void update_search_filter_gui();
	void update_filters();

	void _focus_file_text();

	void _update_favorites();
	void _favorite_pressed();
	void _favorite_selected(int p_idx);
	void _favorite_move_up();
	void _favorite_move_down();

	void _update_recent();
	void _recent_selected(int p_idx);

	void _item_selected(int p_item);
	void _multi_selected(int p_item, bool p_selected);
	void _items_clear_selection(const Vector2 &p_pos, MouseButton p_mouse_button_index);
	void _item_dc_selected(int p_item);

	void _item_list_item_rmb_clicked(int p_item, const Vector2 &p_pos, MouseButton p_mouse_button_index);
	void _item_list_empty_clicked(const Vector2 &p_pos, MouseButton p_mouse_button_index);
	void _item_menu_id_pressed(int p_option);

	void _select_drive(int p_idx);
	void _dir_submitted(const String &p_dir);
	void _action_pressed();
	void _save_confirm_pressed();
	void _cancel_pressed();
	void _filter_selected(int);
	void _make_dir();
	void _make_dir_confirm();

	void _focus_filter_box();
	void _filter_changed(const String &p_text);
	void _search_filter_selected();
	void _file_sort_popup(int p_id);

	void _delete_items();
	void _delete_files_global();

	void _update_drives(bool p_select = true);
	void _update_icons();

	void _go_up();
	void _go_back();
	void _go_forward();

	void _invalidate();

	virtual void _post_popup() override;

	void _save_to_recent();
	// Callback function is callback(String p_path,Ref<Texture2D> preview,Variant udata) preview null if could not load.

	void _thumbnail_result(const String &p_path, const String &p_preview, const String &p_small_preview);
	void _thumbnail_done(const String &p_path, const String &p_preview, const String &p_small_preview);
	void _request_single_thumbnail(const String &p_path);

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

	bool _is_open_should_be_disabled();

	void _native_popup();
	void _native_dialog_cb(bool p_ok, const Vector<String> &p_files, int p_filter, const Dictionary &p_selected_options);

	TypedArray<Dictionary> _get_options() const;
	void _update_option_controls();
	void _option_changed_checkbox_toggled(bool p_pressed, const String &p_name);
	void _option_changed_item_selected(int p_idx, const String &p_name);
=======
	DependencyRemoveDialog *dependency_remove_dialog = nullptr;
>>>>>>> 5f12ada7a4ae9c440e2b22be168c78dba0244075

protected:
	virtual void _item_menu_id_pressed(int p_option) override;

	virtual bool _should_use_native_popup() const override;
	virtual bool _should_hide_file(const String &p_file) const override;
	virtual Color _get_folder_color(const String &p_path) const override;

	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;
	void _notification(int p_what);

public:
#ifndef DISABLE_DEPRECATED
	void add_side_menu(Control *p_menu, const String &p_title = "") { ERR_FAIL_MSG("add_side_menu() is kept for compatibility and does nothing. For similar functionality, you can show another dialog after file dialog."); }
	void set_disable_overwrite_warning(bool p_disable) { set_customization_flag_enabled(CUSTOMIZATION_OVERWRITE_WARNING, !p_disable); }
	bool is_overwrite_warning_disabled() const { return !is_customization_flag_enabled(CUSTOMIZATION_OVERWRITE_WARNING); }
#endif
};
