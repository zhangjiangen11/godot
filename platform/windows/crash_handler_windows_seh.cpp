/**************************************************************************/
/*  crash_handler_windows_seh.cpp                                         */
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

#include "crash_handler_windows.h"

#include "core/config/project_settings.h"
#include "core/object/script_instance.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "core/version.h"
#include "main/main.h"

#ifdef CRASH_HANDLER_EXCEPTION

// Backtrace code based on: https://stackoverflow.com/questions/6205981/windows-c-stack-trace-from-a-running-app

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

#include <psapi.h>

// Some versions of imagehlp.dll lack the proper packing directives themselves
// so we need to do it.
#pragma pack(push, before_imagehlp, 8)
#include <imagehlp.h>
#pragma pack(pop, before_imagehlp)

struct module_data {
	std::string image_name;
	std::string module_name;
	void *base_address = nullptr;
	DWORD load_size;
};

static String stack_frame_to_string(String file, String function, int32_t line) {
	String out;
	if (out.length() > 0) {
		out += "\n";
	}
	out += file + "(" + itos(line) + "):" + function + "";
	return out;
}
void windows_msvc_get_call_stack(String &call_stack) {
	_global_lock();
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
						call_stack += stack_frame_to_string(si[j].file, si[j].func, si[j].line);
					}
				}
			}
		}
		{
			call_stack += stack_frame_to_string(fileName, symbolName, lineNumber);
		}
	}

	SymCleanup(process);
	_global_unlock();
}
class symbol {
	typedef IMAGEHLP_SYMBOL64 sym_type;
	sym_type *sym;
	static const int max_name_len = 1024;

public:
	symbol(HANDLE process, DWORD64 address) :
			sym((sym_type *)::operator new(sizeof(*sym) + max_name_len)) {
		memset(sym, '\0', sizeof(*sym) + max_name_len);
		sym->SizeOfStruct = sizeof(*sym);
		sym->MaxNameLength = max_name_len;
		DWORD64 displacement;

		SymGetSymFromAddr64(process, address, &displacement, sym);
	}

	std::string name() { return std::string(sym->Name); }
	std::string undecorated_name() {
		if (*sym->Name == '\0') {
			return "<couldn't map PC to fn name>";
		}
		std::vector<char> und_name(max_name_len);
		UnDecorateSymbolName(sym->Name, &und_name[0], max_name_len, UNDNAME_COMPLETE);
		return std::string(&und_name[0], strlen(&und_name[0]));
	}
};

class get_mod_info {
	HANDLE process;

public:
	get_mod_info(HANDLE h) :
			process(h) {}

	module_data operator()(HMODULE module) {
		module_data ret;
		char temp[4096];
		MODULEINFO mi;

		GetModuleInformation(process, module, &mi, sizeof(mi));
		ret.base_address = mi.lpBaseOfDll;
		ret.load_size = mi.SizeOfImage;

		GetModuleFileNameEx(process, module, temp, sizeof(temp));
		ret.image_name = temp;
		GetModuleBaseName(process, module, temp, sizeof(temp));
		ret.module_name = temp;
		std::vector<char> img(ret.image_name.begin(), ret.image_name.end());
		std::vector<char> mod(ret.module_name.begin(), ret.module_name.end());
		SymLoadModule64(process, nullptr, &img[0], &mod[0], (DWORD64)ret.base_address, ret.load_size);
		return ret;
	}
};

DWORD CrashHandlerException(EXCEPTION_POINTERS *ep) {
	HANDLE process = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	DWORD offset_from_symbol = 0;
	IMAGEHLP_LINE64 line = {};
	std::vector<module_data> modules;
	DWORD cbNeeded;
	std::vector<HMODULE> module_handles(1);

	if (OS::get_singleton() == nullptr || OS::get_singleton()->is_disable_crash_handler() || IsDebuggerPresent()) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (OS::get_singleton()->is_crash_handler_silent()) {
		std::_Exit(0);
	}

	String msg;
	const ProjectSettings *proj_settings = ProjectSettings::get_singleton();
	if (proj_settings) {
		msg = proj_settings->get("debug/settings/crash_handler/message");
	}

	// Tell MainLoop about the crash. This can be handled by users too in Node.
	if (OS::get_singleton()->get_main_loop()) {
		OS::get_singleton()->get_main_loop()->notification(MainLoop::NOTIFICATION_CRASH);
	}

	print_error("\n================================================================");
	print_error(vformat("%s: Program crashed", __FUNCTION__));

	// Print the engine version just before, so that people are reminded to include the version in backtrace reports.
	if (String(GODOT_VERSION_HASH).is_empty()) {
		print_error(vformat("Engine version: %s", GODOT_VERSION_FULL_NAME));
	} else {
		print_error(vformat("Engine version: %s (%s)", GODOT_VERSION_FULL_NAME, GODOT_VERSION_HASH));
	}
	print_error(vformat("Dumping the backtrace. %s", msg));

	// Load the symbols:
	if (!SymInitialize(process, nullptr, false)) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_EXACT_SYMBOLS);
	EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
	module_handles.resize(cbNeeded / sizeof(HMODULE));
	EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
	std::transform(module_handles.begin(), module_handles.end(), std::back_inserter(modules), get_mod_info(process));
	void *base = modules[0].base_address;

	// Setup stuff:
	CONTEXT *context = ep->ContextRecord;
	STACKFRAME64 frame;
	bool skip_first = false;

	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;

#if defined(_M_X64)
	frame.AddrPC.Offset = context->Rip;
	frame.AddrStack.Offset = context->Rsp;
	frame.AddrFrame.Offset = context->Rbp;
#elif defined(_M_ARM64) || defined(_M_ARM64EC)
	frame.AddrPC.Offset = context->Pc;
	frame.AddrStack.Offset = context->Sp;
	frame.AddrFrame.Offset = context->Fp;
#elif defined(_M_ARM)
	frame.AddrPC.Offset = context->Pc;
	frame.AddrStack.Offset = context->Sp;
	frame.AddrFrame.Offset = context->R11;
#else
	frame.AddrPC.Offset = context->Eip;
	frame.AddrStack.Offset = context->Esp;
	frame.AddrFrame.Offset = context->Ebp;

	// Skip the first one to avoid a duplicate on 32-bit mode
	skip_first = true;
#endif

	line.SizeOfStruct = sizeof(line);
	IMAGE_NT_HEADERS *h = ImageNtHeader(base);
	DWORD image_type = h->FileHeader.Machine;

	int n = 0;
	do {
		if (skip_first) {
			skip_first = false;
		} else {
			if (frame.AddrPC.Offset != 0) {
				std::string fnName = symbol(process, frame.AddrPC.Offset).undecorated_name();

				if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &offset_from_symbol, &line)) {
					print_error(vformat("[%d] %s (%s:%d)", n, fnName.c_str(), (char *)line.FileName, (int)line.LineNumber));
				} else {
					print_error(vformat("[%d] %s", n, fnName.c_str()));
				}
			} else {
				print_error(vformat("[%d] ???", n));
			}

			n++;
		}

		if (!StackWalk64(image_type, process, hThread, &frame, context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
			break;
		}
	} while (frame.AddrReturn.Offset != 0 && n < 256);

	print_error("-- END OF BACKTRACE --");
	print_error("================================================================");

	SymCleanup(process);

	// Pass the exception to the OS
	return EXCEPTION_CONTINUE_SEARCH;
}
static String get_call_stack() {
	String call_stack;
	return call_stack;
}
#endif

CrashHandler::CrashHandler() {
	disabled = false;
}

CrashHandler::~CrashHandler() {
}

void CrashHandler::disable() {
	if (disabled) {
		return;
	}

	disabled = true;
}

void CrashHandler::initialize() {
}
