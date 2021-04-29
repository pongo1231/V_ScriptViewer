#pragma once

#include <atomic>

typedef unsigned long long DWORD64;

struct ScriptProfile
{
	std::atomic<__int64> m_llLongestExecutionTimeNs;
	float m_fCurExecutionTimeMs;
	DWORD64 m_qwLastProfileUpdatedTimestamp;
};