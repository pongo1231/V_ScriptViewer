#pragma once

typedef unsigned int DWORD_t;

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
	virtual void ScriptRoutineCallback(rage::scrThread *pThread, ERoutineTraceType eTraceType, DWORD_t dwEnterIP) = 0;
};

namespace Memory
{
	void RegisterTraceCallback(ScriptRoutineTracer *pScriptRoutineTracer);

	void UnregisterTraceCallback(ScriptRoutineTracer *pScriptRoutineTracer);
}