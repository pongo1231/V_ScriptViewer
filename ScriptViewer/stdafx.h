#pragma once

typedef unsigned int DWORD_t;

#include "Main.h"
#include "nativesNoNamespaces.h"

#include "Components/Component.h"
#include "Components/ComponentView.h"
#include "Components/GlobalView.h"
#include "Components/OptionsView.h"
#include "Components/RecordView.h"
#include "Components/ScriptView.h"

#include "Memory/Handle.h"
#include "Memory/Hook.h"
#include "Memory/Hwnd.h"
#include "Memory/Memory.h"
#include "Memory/ScriptRoutineTracer.h"

#include "Util/Logging.h"
#include "Util/Misc.h"

#include "Lib/fwTimer.h"
#include "Lib/scrThread.h"

#include "../vendor/scripthookv/main.h"
#include "../vendor/scripthookv/natives.h"

#include "../vendor/minhook/include/MinHook.h"

#include "../vendor/imgui/libs/imgui/backends/imgui_impl_dx11.h"
#include "../vendor/imgui/libs/imgui/backends/imgui_impl_win32.h"
#include "../vendor/imgui/libs/imgui/imgui.h"
#include "../vendor/imgui/libs/imgui/imgui_internal.h"
#include <setjmp.h>

#define WIN32_LEAN_AND_MEAN
#include <excpt.h>
#include <psapi.h>
#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

#include <codecvt>
#include <fstream>
#include <locale>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class __SEH_HANDLER;

typedef struct tag__SEH_EXCEPTION_REGISTRATION
{
	tag__SEH_EXCEPTION_REGISTRATION *prev;
	PEXCEPTION_HANDLER handler;
	__SEH_HANDLER *exthandler;
} __SEH_EXCEPTION_REGISTRATION;

class __SEH_HANDLER
{
  public:
	// This is the main exception handling function.  This is called
	// for each exception raised using this method.
	static EXCEPTION_DISPOSITION ExceptionRouter(PEXCEPTION_RECORD pRecord, __SEH_EXCEPTION_REGISTRATION *pReg,
	                                             PCONTEXT pContext, PEXCEPTION_RECORD pRecord2)
	{
		// Retrieve the actual __SEH_HANDLER object from the registration, and call the
		// specific exception handling function.  Everything could have been done from this
		// function alone, but I decided to use an instance method instead.
		return pReg->exthandler->ExceptionHandler(pRecord, pReg, pContext, pRecord2);
	}

	// This is the exception handler for this specific instance.  This is called by the
	// ExceptionRouter class function.
	virtual EXCEPTION_DISPOSITION ExceptionHandler(PEXCEPTION_RECORD pRecord, __SEH_EXCEPTION_REGISTRATION *pReg,
	                                               PCONTEXT pContext, PEXCEPTION_RECORD pRecord2)
	{
		// The objects pointed to by the pointers live on the stack, so a copy of them is required,
		// or they may get overwritten by the time we've hit the real exception handler code
		// back in the offending function.
		CopyMemory(&excContext, pContext, sizeof(_CONTEXT));
		CopyMemory(&excRecord, pRecord, sizeof(_EXCEPTION_RECORD));

		// Jump back to the function where the exception actually occurred.  The 1 is the
		// return code that will be returned by set_jmp.
		longjmp(context, 1);
	}

	// This is the context buffer used by setjmp.  This stores the context at a given point
	// in the program so that it can be resumed.
	jmp_buf context;

	// This is a copy of the EXCEPTION_RECORD structure passed to the exception handler.
	EXCEPTION_RECORD excRecord;
	// This is a copy of the CONTEXT structure passed to the exception handler.
	CONTEXT excContext;
};

#define __try                                                                                       \
	{                                                                                               \
		__SEH_EXCEPTION_REGISTRATION _lseh_er;                                                      \
		__SEH_HANDLER _lseh_handler;                                                                \
                                                                                                    \
		_lseh_er.handler    = reinterpret_cast<PEXCEPTION_HANDLER>(__SEH_HANDLER::ExceptionRouter); \
		_lseh_er.exthandler = &_lseh_handler;                                                       \
		asm volatile("mov %%fs:0, %0" : "=r"(_lseh_er.prev));                                       \
		asm volatile("mov %0, %%fs:0" : : "r"(&_lseh_er));                                          \
		int _lseh_setjmp_res = setjmp(_lseh_handler.context);                                       \
		while (true)                                                                                \
		{                                                                                           \
			if (_lseh_setjmp_res != 0)                                                              \
			{                                                                                       \
				break;                                                                              \
			}

#define __except(rec, ctx)                                 \
	break;                                                 \
	}                                                      \
	PEXCEPTION_RECORD rec = &_lseh_handler.excRecord;      \
	PCONTEXT ctx          = &_lseh_handler.excContext;     \
                                                           \
	asm volatile("mov %0, %%fs:0" : : "r"(_lseh_er.prev)); \
	if (_lseh_setjmp_res != 0)

#define __end }