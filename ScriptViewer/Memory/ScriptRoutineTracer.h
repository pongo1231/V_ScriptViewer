#pragma once

typedef unsigned long DWORD;

class rage::scrThread;

enum class ERoutineTraceType : int
{
	None,
	Enter,
	Leave
};

class ScriptRoutineTracer
{
public:
	virtual void ScriptRoutineCallback(rage::scrThread* pThread, ERoutineTraceType eTraceType, DWORD dwEnterIP) = 0;
};

namespace Memory
{
	void RegisterTraceCallback(ScriptRoutineTracer* pScriptRoutineTracer);

	void UnregisterTraceCallback(ScriptRoutineTracer* pScriptRoutineTracer);
}