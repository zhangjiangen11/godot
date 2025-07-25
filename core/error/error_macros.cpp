/**************************************************************************/
/*  error_macros.cpp                                                      */
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

#include "error_macros.h"

#include "core/io/logger.h"
#include "core/object/object_id.h"
#include "core/object/script_instance.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/string/ustring.h"

struct StackFrame {
	String file;
	String function;
	int32_t line;
	void to_string(String &out) {
		if (out.length() > 0) {
			out += "\n";
		}
		out += file + "(" + itos(line) + "):" + function + "";
	}
};

static void getStackTrace(LocalVector<StackFrame> &stackTrace);

#if defined(WINDOWS_ENABLED) && defined(_MSC_VER) && defined(DEBUG_ENABLED)

#include <windows.h>
// 调试
#include <dbghelp.h>

static void getStackTrace(LocalVector<StackFrame> &stackTrace) {
	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();

	CONTEXT context = {};
	context.ContextFlags = CONTEXT_FULL;
	RtlCaptureContext(&context);

	SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_EXACT_SYMBOLS);
	SymInitialize(process, nullptr, TRUE);

	DWORD image;
	STACKFRAME64 stackFrame;
	ZeroMemory(&stackFrame, sizeof(STACKFRAME64));

	image = IMAGE_FILE_MACHINE_AMD64;
	stackFrame.AddrPC.Offset = context.Rip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Rsp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Rsp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
#if defined(_M_X64)
	stackFrame.AddrPC.Offset = context.Rip;
	stackFrame.AddrStack.Offset = context.Rsp;
	stackFrame.AddrFrame.Offset = context.Rbp;
#elif defined(_M_ARM64) || defined(_M_ARM64EC)
	stackFrame.AddrPC.Offset = context.Pc;
	stackFrame.AddrStack.Offset = context.Sp;
	stackFrame.AddrFrame.Offset = context->Fp;
#elif defined(_M_ARM)
	stackFrame.AddrPC.Offset = context.Pc;
	stackFrame.AddrStack.Offset = context.Sp;
	stackFrame.AddrFrame.Offset = context->R11;
#else
	stackFrame.AddrPC.Offset = context.Eip;
	stackFrame.AddrStack.Offset = context.Esp;
	stackFrame.AddrFrame.Offset = context.Ebp;

#endif

	typedef SYMBOL_INFO sym_type;
	sym_type *symbol = (sym_type *)alloca(sizeof(sym_type) + 1024);
	int index = 0;
	bool is_in_gdscript = false;
	while (true) {
		if (StackWalk64(
					image, process, thread,
					&stackFrame, &context, nullptr,
					SymFunctionTableAccess64, SymGetModuleBase64, nullptr) == FALSE) {
			break;
		}

		if (stackFrame.AddrReturn.Offset == stackFrame.AddrPC.Offset) {
			break;
		}

		++index;
		if (index < 2) {
			continue;
		}
		memset(symbol, '\0', sizeof(sym_type) + 1024);
		symbol->SizeOfStruct = sizeof(sym_type);
		symbol->MaxNameLen = 1024;

		DWORD64 displacementSymbol = 0;
		const char *symbolName;
		if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacementSymbol, symbol) == TRUE) {
			symbolName = symbol->Name;
		} else {
			symbolName = "??";
		}

		SymSetOptions(SYMOPT_LOAD_LINES);

		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		DWORD displacementLine = 0;

		int32_t lineNumber = -1;
		const char *fileName;
		if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &displacementLine, &line) == TRUE) {
			lineNumber = line.LineNumber;
			fileName = line.FileName;
		} else {
			lineNumber = -1;
			fileName = "??";
		}
		if (is_in_gdscript == false && strcmp(symbolName, "GDScriptFunction::call") == 0) {
			is_in_gdscript = true;
			for (int i = 0; i < ScriptServer::get_language_count(); i++) {
				if (ScriptServer::get_language(i)->get_type() == "GDScript") {
					Vector<ScriptLanguage::StackInfo> si = ScriptServer::get_language(i)->debug_get_current_stack_info();
					for (int j = 0; j < si.size(); j++) {
						stackTrace.push_back(StackFrame{ si[j].file, si[j].func, si[j].line });
					}
				}
			}
		}
		{
			stackTrace.push_back(StackFrame{ fileName, symbolName, lineNumber });
		}
	}

	SymCleanup(process);

	return;
}

#else

static void getStackTrace(LocalVector<StackFrame> &stackFrame) {
}

#endif

// Optional physics interpolation warnings try to include the path to the relevant node.
#if defined(DEBUG_ENABLED) && defined(TOOLS_ENABLED)
#include "core/config/project_settings.h"
#include "scene/main/node.h"
#endif

static ErrorHandlerList *error_handler_list = nullptr;
static thread_local bool is_printing_error = false;

static void _err_print_fallback(const char *p_function, const char *p_file, int p_line, const char *p_error_details, ErrorHandlerType p_type, bool p_reentrance) {
	if (p_reentrance) {
		fprintf(stderr, "While attempting to print an error, another error was printed:\n");
	}

	fprintf(stderr, "%s: %s\n", _error_handler_type_string(p_type), p_error_details);

	if (p_function && p_file) {
				fprintf(stderr, "%s(%i): %s at: [%s]\n", p_file, p_line, p_function, _error_handler_type_string(p_type));
		//fprintf(stderr, "   at: %s (%s:%i)\n", p_function, p_file, p_line);
	}
}

void add_error_handler(ErrorHandlerList *p_handler) {
	// If p_handler is already in error_handler_list
	// we'd better remove it first then we can add it.
	// This prevent cyclic redundancy.
	remove_error_handler(p_handler);

	_global_lock();

	p_handler->next = error_handler_list;
	error_handler_list = p_handler;

	_global_unlock();
}

void remove_error_handler(const ErrorHandlerList *p_handler) {
	_global_lock();

	ErrorHandlerList *prev = nullptr;
	ErrorHandlerList *l = error_handler_list;

	while (l) {
		if (l == p_handler) {
			if (prev) {
				prev->next = l->next;
			} else {
				error_handler_list = l->next;
			}
			break;
		}
		prev = l;
		l = l->next;
	}

	_global_unlock();
}

// Errors without messages.
void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error, "", p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), "", p_editor_notify, p_type);
}
struct LastLogInfo {
	Mutex mutex;
	String function;
	String file;
	int line = 0;
	String error;
	String message;
	ErrorHandlerType type = ERR_HANDLER_ERROR;
	double last_time = 0;

	bool on_log(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
		MutexLock lock(mutex);
		double tm = OS::get_singleton()->get_unix_time();
		if (tm - last_time > 5) {
			cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
			return true;
		}
		if (p_type != type) {
			cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
			return true;
		}
		if (p_function) {
			if (function != p_function) {
				cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
				return true;
			}
		} else if (!function.is_empty()) {
			cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
			return true;
		}

		if (p_file) {
			if (file != p_file) {
				cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
				return true;
			}
		} else if (!file.is_empty()) {
			cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
			return true;
		}

		if (line != p_line) {
			cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
			return true;
		}

		if (p_error) {
			if (error != p_error) {
				cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
				return true;
			}
		} else if (!error.is_empty()) {
			cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
			return true;
		}

		if (p_message) {
			if (message != p_message) {
				cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
				return true;
			}
		} else if (!message.is_empty()) {
			cache(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
			return true;
		}

		return false;
	}
	void cache(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
		last_time = OS::get_singleton()->get_unix_time();
		if (p_function != nullptr) {
			function = p_function;
		} else {
			function = "";
		}

		if (p_file != nullptr) {
			file = p_file;
		} else {
			file = "";
		}

		line = p_line;
		if (p_error != nullptr) {
			error = p_error;
		} else {
			error = "";
		}

		if (p_message != nullptr) {
			message = p_message;
		} else {
			message = "";
		}
		type = p_type;
	}

} last_log_info;
// Main error printing function.
void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	if (OS::get_singleton() == nullptr) {
		return;
	}
	if (!last_log_info.on_log(p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type)) {
		return;
	}
	String temp;
	CharString data;
	if (p_type == ERR_HANDLER_ERROR) {
		if (p_file) {
			temp += p_file;
			temp += "(";
			temp += itos(p_line);
			temp += "): ";
		}
		if (p_message) {
			temp += ": ";
			temp += p_message;
			temp += " ";
		}
		if (p_function != nullptr) {
			temp += "->";
			temp += p_function;
			temp += "()";
		}
		temp += p_error;
		LocalVector<StackFrame> stackFrame;
		_global_lock();
		getStackTrace(stackFrame);
		_global_unlock();
		for (uint32_t i = 0; i < stackFrame.size(); ++i) {
			stackFrame[i].to_string(temp);
		}
		data = temp.utf8();
		p_message = data.get_data();
	}
	if (OS::get_singleton()) {
		OS::get_singleton()->print_error(p_function, p_file, p_line, p_error, p_message, p_editor_notify, (Logger::ErrorType)p_type, ScriptServer::capture_script_backtraces(false));
	} else {
		// Fallback if errors happen before OS init or after it's destroyed.
		const char *err_details = (p_message && *p_message) ? p_message : p_error;
		_err_print_fallback(p_function, p_file, p_line, err_details, p_type, false);
	}

	_global_lock();

	ErrorHandlerList *l = error_handler_list;
	while (l) {
		l->errfunc(l->userdata, p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
		l = l->next;
	}

	_global_unlock();

	is_printing_error = false;
}

// For printing errors when we may crash at any point, so we must flush ASAP a lot of lines
// but we don't want to make it noisy by printing lots of file & line info (because it's already
// been printing by a preceding _err_print_error).
void _err_print_error_asap(const String &p_error, ErrorHandlerType p_type) {
	const char *err_details = p_error.utf8().get_data();

	if (is_printing_error) {
		// Fallback if we're already printing an error, to prevent infinite recursion.
		_err_print_fallback(nullptr, nullptr, 0, err_details, p_type, true);
		return;
	}

	is_printing_error = true;

	if (OS::get_singleton()) {
		OS::get_singleton()->printerr("%s: %s\n", _error_handler_type_string(p_type), err_details);
	} else {
		// Fallback if errors happen before OS init or after it's destroyed.
		_err_print_fallback(nullptr, nullptr, 0, err_details, p_type, false);
	}

	_global_lock();

	ErrorHandlerList *l = error_handler_list;
	while (l) {
		l->errfunc(l->userdata, "", "", 0, err_details, "", false, p_type);
		l = l->next;
	}

	_global_unlock();

	is_printing_error = false;
}

// Errors with message. (All combinations of p_error and p_message as String or char*.)
void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), p_message, p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const String &p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error, p_message.utf8().get_data(), p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const String &p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), p_message.utf8().get_data(), p_editor_notify, p_type);
}

// Index errors. (All combinations of p_message as String or char*.)
void _err_print_index_error(const char *p_function, const char *p_file, int p_line, int64_t p_index, int64_t p_size, const char *p_index_str, const char *p_size_str, const char *p_message, bool p_editor_notify, bool p_fatal) {
	String fstr(p_fatal ? "FATAL: " : "");
	String err(fstr + "Index " + p_index_str + " = " + itos(p_index) + " is out of bounds (" + p_size_str + " = " + itos(p_size) + ").");
	_err_print_error(p_function, p_file, p_line, err.utf8().get_data(), p_message, p_editor_notify, ERR_HANDLER_ERROR);
}

void _err_print_index_error(const char *p_function, const char *p_file, int p_line, int64_t p_index, int64_t p_size, const char *p_index_str, const char *p_size_str, const String &p_message, bool p_editor_notify, bool p_fatal) {
	_err_print_index_error(p_function, p_file, p_line, p_index, p_size, p_index_str, p_size_str, p_message.utf8().get_data(), p_editor_notify, p_fatal);
}

void _err_flush_stdout() {
	fflush(stdout);
}

// Prevent error spam by limiting the warnings to a certain frequency.
void _physics_interpolation_warning(const char *p_function, const char *p_file, int p_line, ObjectID p_id, const char *p_warn_string) {
#if defined(DEBUG_ENABLED) && defined(TOOLS_ENABLED)
	const uint32_t warn_max = 2048;
	const uint32_t warn_timeout_seconds = 15;

	static uint32_t warn_count = warn_max;
	static uint32_t warn_timeout = warn_timeout_seconds;

	uint32_t time_now = UINT32_MAX;

	if (warn_count) {
		warn_count--;
	}

	if (!warn_count) {
		time_now = OS::get_singleton()->get_ticks_msec() / 1000;
	}

	if ((warn_count == 0) && (time_now >= warn_timeout)) {
		warn_count = warn_max;
		warn_timeout = time_now + warn_timeout_seconds;

		if (GLOBAL_GET("debug/settings/physics_interpolation/enable_warnings")) {
			// UINT64_MAX means unused.
			if (p_id.operator uint64_t() == UINT64_MAX) {
				_err_print_error(p_function, p_file, p_line, "[Physics interpolation] " + String(p_warn_string) + " (possibly benign).", false, ERR_HANDLER_WARNING);
			} else {
				String node_name;
				if (p_id.is_valid()) {
					Node *node = ObjectDB::get_instance<Node>(p_id);
					if (node && node->is_inside_tree()) {
						node_name = "\"" + String(node->get_path()) + "\"";
					} else {
						node_name = "\"unknown\"";
					}
				}

				_err_print_error(p_function, p_file, p_line, "[Physics interpolation] " + String(p_warn_string) + ": " + node_name + " (possibly benign).", false, ERR_HANDLER_WARNING);
			}
		}
	}
#endif
}
