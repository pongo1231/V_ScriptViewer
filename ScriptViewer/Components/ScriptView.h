#pragma once

#include "Component.h"

#include <array>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

typedef unsigned short WORD;
typedef unsigned int DWORD_t;
typedef unsigned long long DWORD64;

class ScriptProfile;

class ScriptView : public Component
{
  private:
	enum class eScriptProfileType
	{
		HIGHEST_TIME,
		AVERAGE_TIME
	};

	class ScriptProfile
	{
	  private:
		std::atomic<DWORD64> m_qwProfilingTimeMcS = 0;
		std::atomic<DWORD64> m_cqwExecutions      = 0;
		float m_fCurExecutionTimeMs               = 0.f;

	  public:
		static inline eScriptProfileType ms_eProfileType = eScriptProfileType::HIGHEST_TIME;

		ScriptProfile()                                  = default;

		ScriptProfile(const ScriptProfile &)             = delete;

		ScriptProfile &operator=(const ScriptProfile &)  = delete;

		ScriptProfile(ScriptProfile &&)                  = delete;

		ScriptProfile &operator=(ScriptProfile &&)       = delete;

		inline void Add(DWORD64 qwTimeMcS)
		{
			switch (ms_eProfileType)
			{
			case eScriptProfileType::HIGHEST_TIME:
				if (qwTimeMcS > m_qwProfilingTimeMcS)
				{
					m_qwProfilingTimeMcS = qwTimeMcS;
				}

				break;
			case eScriptProfileType::AVERAGE_TIME:
				m_qwProfilingTimeMcS += qwTimeMcS;

				m_cqwExecutions++;

				break;
			}
		}

		inline void NewRound()
		{
			if (ms_eProfileType == eScriptProfileType::AVERAGE_TIME)
			{
				m_qwProfilingTimeMcS = !m_cqwExecutions ? 0 : m_qwProfilingTimeMcS / m_cqwExecutions;
			}

			m_fCurExecutionTimeMs = m_qwProfilingTimeMcS / 1000.f;

			m_qwProfilingTimeMcS  = 0;

			m_cqwExecutions       = 0;
		}

		inline float GetMs() const
		{
			return m_fCurExecutionTimeMs;
		}
	};

	std::unordered_map<DWORD, ScriptProfile> m_dcScriptProfiles;

	std::vector<DWORD> m_rgdwBlacklistedScriptThreadIds;
	std::mutex m_blacklistedScriptThreadIdsMutex;

	std::atomic<DWORD> m_dwKillScriptThreadId = 0;

	bool m_bShowExecutionTimes                = false;
	bool m_bShowStackSizes                    = false;

	bool m_bIsNewScriptWindowOpen             = false;
	std::array<char, 64> m_rgchNewScriptNameBuffer {};
	int m_iNewScriptStackSize               = 0;
	bool m_bDoDispatchNewScript             = false;

	WORD m_wSelectedItemIdx                 = 0;

	DWORD64 m_qwLastProfileUpdatedTimestamp = 0;

  public:
	ScriptView() : Component()
	{
		m_bIsOpen = true;
	}

	virtual bool RunHook(rage::scrThread *pScrThread) override;

	virtual void RunCallback(rage::scrThread *pScrThread, DWORD64 qwExecutionTime) override;

	virtual void RunImGui() override;

	virtual void RunScript() override;

  private:
	bool IsScriptThreadIdPaused(DWORD_t dwThreadId);

	void PauseScriptThreadId(DWORD_t dwThreadId);

	void UnpauseScriptThreadId(DWORD_t dwThreadId);

	void ClearNewScriptWindowState();
};