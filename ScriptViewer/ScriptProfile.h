#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atomic>

typedef unsigned long long DWORD64;

enum class eScriptProfileType
{
	HIGHEST_TIME,
	AVERAGE_TIME
};

class ScriptProfile
{
private:
	std::atomic<DWORD64> m_qwProfilingTimeNs = 0;
	std::atomic<DWORD64> m_cqwExecutions = 0;
	float m_fCurExecutionTimeMs = 0.f;

public:
	static inline eScriptProfileType ms_eProfileType = eScriptProfileType::HIGHEST_TIME;

	inline void Add(DWORD64 qwTimeNs)
	{
		switch (ms_eProfileType)
		{
		case eScriptProfileType::HIGHEST_TIME:
			if (qwTimeNs > m_qwProfilingTimeNs)
			{
				m_qwProfilingTimeNs = qwTimeNs;
			}

			break;
		case eScriptProfileType::AVERAGE_TIME:
			m_qwProfilingTimeNs += qwTimeNs;

			m_cqwExecutions++;

			break;
		}
	}

	inline void NewRound()
	{
		if (ms_eProfileType == eScriptProfileType::AVERAGE_TIME)
		{
			m_qwProfilingTimeNs = !m_cqwExecutions ? 0 : m_qwProfilingTimeNs / m_cqwExecutions;
		}

		m_fCurExecutionTimeMs = m_qwProfilingTimeNs / 1000000.f;

		m_qwProfilingTimeNs = 0;

		m_cqwExecutions = 0;
	}

	inline float [[nodiscard]] Get() const
	{
		return m_fCurExecutionTimeMs;
	}
};