#include <stdafx.h>
#include <winnt.h>

#include "ScriptRoutineTracer.h"

static std::vector<ScriptRoutineTracer *> ms_rgScriptRoutineTracers;

static char *ms_chEnterTrap = nullptr;
static char *ms_chLeaveTrap = nullptr;

namespace Memory
{
	void RegisterTraceCallback(ScriptRoutineTracer *pScriptRoutineTracer)
	{
		if (!pScriptRoutineTracer
		    || std::find(ms_rgScriptRoutineTracers.begin(), ms_rgScriptRoutineTracers.end(), pScriptRoutineTracer)
		           != ms_rgScriptRoutineTracers.end())
		{
			return;
		}

		ms_rgScriptRoutineTracers.push_back(pScriptRoutineTracer);
	}

	void UnregisterTraceCallback(ScriptRoutineTracer *pScriptRoutineTracer)
	{
		if (!pScriptRoutineTracer)
		{
			return;
		}

		const auto &result =
		    std::find(ms_rgScriptRoutineTracers.begin(), ms_rgScriptRoutineTracers.end(), pScriptRoutineTracer);
		if (result != ms_rgScriptRoutineTracers.end())
		{
			ms_rgScriptRoutineTracers.erase(result);
		}
	}
}

static long HandleCallInstr(EXCEPTION_POINTERS *exceptionInfo)
{
	if (exceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	rage::scrThread *pThread = nullptr;
	for (WORD wScriptIdx = 0; wScriptIdx < *rage::scrThread::ms_pcwThreads; wScriptIdx++)
	{
		if (rage::scrThread::ms_ppThreads[wScriptIdx]->m_dwThreadId
		    == *reinterpret_cast<DWORD *>(exceptionInfo->ContextRecord->R15) /* thread id */)
		{
			pThread = rage::scrThread::ms_ppThreads[wScriptIdx];
		}
	}

	char *pchCurInstrAddr  = reinterpret_cast<char *>(exceptionInfo->ContextRecord->Rip);
	char *pchNextInstrAddr = pchCurInstrAddr + 3;

	if (pchCurInstrAddr == ms_chEnterTrap || pchCurInstrAddr == ms_chLeaveTrap)
	{
		if (pchCurInstrAddr == ms_chEnterTrap && pThread)
		{
			DWORD_t dwIP = exceptionInfo->ContextRecord->Rdi - exceptionInfo->ContextRecord->Rsi - 5;

			for (ScriptRoutineTracer *pScriptRoutineTracer : ms_rgScriptRoutineTracers)
			{
				pScriptRoutineTracer->ScriptRoutineCallback(pThread, ERoutineTraceType::Enter, dwIP);
			}
		}
		else if (pThread)
		{
			for (ScriptRoutineTracer *pScriptRoutineTracer : ms_rgScriptRoutineTracers)
			{
				pScriptRoutineTracer->ScriptRoutineCallback(pThread, ERoutineTraceType::Leave, 0);
			}
		}

		*pchCurInstrAddr = 0x49;

		if (!ms_rgScriptRoutineTracers.empty())
		{
			*pchNextInstrAddr = 0xCC;
		}
	}
	else
	{
		char *pchPrevInstrAddr = reinterpret_cast<char *>(exceptionInfo->ContextRecord->Rip - 3);

		if (pchPrevInstrAddr == ms_chEnterTrap)
		{
			*pchCurInstrAddr  = 0x48;

			*pchPrevInstrAddr = 0xCC;
		}
		else if (pchPrevInstrAddr == ms_chLeaveTrap)
		{
			*pchCurInstrAddr  = 0x45;

			*pchPrevInstrAddr = 0xCC;
		}
		else
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}

	return EXCEPTION_CONTINUE_EXECUTION;
}

__int64 (*OG_rage__scrThread___RunInstr)(__int64, __int64 **, __int64, DWORD *);
__int64 HK_rage__scrThread___RunInstr(__int64 a1, __int64 **a2, __int64 a3, DWORD *pDwThreadId)
{
	if (!ms_rgScriptRoutineTracers.empty() && ms_chEnterTrap && ms_chLeaveTrap)
	{
		*ms_chEnterTrap = 0xCC;
		*ms_chLeaveTrap = 0xCC;
	}

	auto handle = AddVectoredExceptionHandler(1, HandleCallInstr);

	auto result = OG_rage__scrThread___RunInstr(a1, a2, a3, pDwThreadId);

	RemoveVectoredExceptionHandler(handle);

	return result;
}

static void OnHook()
{
	Handle handle;

	handle = Memory::FindPattern("E8 ? ? ? ? 48 85 FF 48 89 1D");
	if (handle.IsValid())
	{
		handle = handle.Into();

		Memory::AddHook(handle.Get<void>(), HK_rage__scrThread___RunInstr, &OG_rage__scrThread___RunInstr);

		LOG("Hooked rage::scrThread::_RunInstr");

		ms_chEnterTrap = handle.At(0x530).Get<char>();

		LOG("Found ENTER instruction byte to trap in rage::scrThread::_RunInstr");

		ms_chLeaveTrap = handle.At(0x5D7).Get<char>();

		LOG("Found LEAVE instruction byte to trap in rage::scrThread::_RunInstr");
	}
}

static RegisterHook hook(OnHook);