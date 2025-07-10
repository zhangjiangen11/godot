/**
 * editor.cpp
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */
#include "editor.h"

#ifdef TOOLS_ENABLED

#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/script/script_editor_plugin.h"

void SHOW_BUILTIN_DOC(const String &p_topic) {
	ScriptEditor::get_singleton()->goto_help(p_topic);
	EditorNode::get_singleton()->get_editor_main_screen()->select(EditorMainScreen::EDITOR_SCRIPT);
}

void EDIT_SCRIPT(const String &p_path) {
	Ref<Resource> res = ScriptEditor::get_singleton()->open_file(p_path);
	ERR_FAIL_COND_MSG(res.is_null(), "Failed to load script: " + p_path);
	EditorNode::get_singleton()->edit_resource(res);
}

#endif // ! TOOLS_ENABLED
