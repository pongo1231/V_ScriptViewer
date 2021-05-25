#pragma once

#include "Component.h"
#include "../Memory/ScriptRoutineTracer.h"
#include "../Util/Misc.h"

#include "../Util/Logging.h"

#include <unordered_map>
#include <vector>
#include <set>
#include <mutex>

typedef unsigned long DWORD;
typedef unsigned long long DWORD64;

class ScriptProfile;
class rage::scrThread;

enum class ERoutineTraceType;

class RecordView : public Component, public ScriptRoutineTracer
{
private:
	enum class ETraceMethod : char
	{
		None,
		IP,
		Routine
	};

	enum class EMessageType : char
	{
		None,
		Pause,
		Resume,
		EnterCalled,
		LeaveCalled
	};

	class DetailedScriptProfile
	{
	private:
		class ScriptTrace
		{
		private:
			DWORD m_dwScriptIP = 0;

			DWORD64 m_qwScriptExecTimeMcS = 0;
			float m_fScriptExecTimeSecs = 0.f;

			bool m_bScriptHasEnded = false;

		public:
			ScriptTrace(DWORD dwIP, DWORD64 qwExecTimeMcS = 0) : m_dwScriptIP(dwIP), m_qwScriptExecTimeMcS(qwExecTimeMcS)
			{

			}

			ScriptTrace(const ScriptTrace& scriptTrace) : ScriptTrace(scriptTrace.m_dwScriptIP, scriptTrace.m_qwScriptExecTimeMcS)
			{
				m_fScriptExecTimeSecs = scriptTrace.m_fScriptExecTimeSecs;
				m_bScriptHasEnded = scriptTrace.m_bScriptHasEnded;
			}

			ScriptTrace& operator=(const ScriptTrace& scriptTrace)
			{
				m_dwScriptIP = scriptTrace.m_dwScriptIP;
				m_qwScriptExecTimeMcS = scriptTrace.m_qwScriptExecTimeMcS;
				m_fScriptExecTimeSecs = scriptTrace.m_fScriptExecTimeSecs;
				m_bScriptHasEnded = scriptTrace.m_bScriptHasEnded;

				return *this;
			}

			ScriptTrace(ScriptTrace&& scriptTrace) noexcept : ScriptTrace(scriptTrace.m_dwScriptIP, scriptTrace.m_qwScriptExecTimeMcS)
			{
				m_fScriptExecTimeSecs = scriptTrace.m_fScriptExecTimeSecs;
				m_bScriptHasEnded = scriptTrace.m_bScriptHasEnded;
			}

			ScriptTrace& operator=(ScriptTrace&& scriptTrace) noexcept
			{
				m_dwScriptIP = scriptTrace.m_dwScriptIP;
				m_qwScriptExecTimeMcS = scriptTrace.m_qwScriptExecTimeMcS;
				m_fScriptExecTimeSecs = scriptTrace.m_fScriptExecTimeSecs;
				m_bScriptHasEnded = scriptTrace.m_bScriptHasEnded;

				return *this;
			}

			bool operator==(DWORD dwIP) const
			{
				return m_dwScriptIP == dwIP;
			}

			bool operator<(const ScriptTrace& scriptTrace) const
			{
				return m_qwScriptExecTimeMcS < scriptTrace.m_qwScriptExecTimeMcS;
			}

			bool operator>(const ScriptTrace& scriptTrace) const
			{
				return m_qwScriptExecTimeMcS > scriptTrace.m_qwScriptExecTimeMcS;
			}

			virtual inline [[nodiscard]] std::unique_ptr<ScriptTrace> Clone() = 0;

			inline void Add(DWORD64 qwExecTimeMcS)
			{
				if (HasEnded())
				{
					return;
				}

				m_qwScriptExecTimeMcS += qwExecTimeMcS;
			}

			inline void End()
			{
				if (m_bScriptHasEnded)
				{
					return;
				}

				m_bScriptHasEnded = true;

				_End();

				m_fScriptExecTimeSecs = !m_qwScriptExecTimeMcS ? m_qwScriptExecTimeMcS : m_qwScriptExecTimeMcS / 1000000.f;
			}

		private:
			virtual inline void _End()
			{

			}

		public:
			inline [[nodiscard]] DWORD GetIP() const
			{
				return m_dwScriptIP;
			}

			inline [[nodiscard]] bool HasEnded() const
			{
				return m_bScriptHasEnded;
			}

			inline [[nodiscard]] float GetSecs() const
			{
				if (HasEnded())
				{
					return m_fScriptExecTimeSecs;
				}

				return !m_qwScriptExecTimeMcS ? m_qwScriptExecTimeMcS : m_qwScriptExecTimeMcS / 1000000.f;
			}

			inline [[nodiscard]] DWORD64 GetMcS() const
			{
				return m_qwScriptExecTimeMcS;
			}
		};

		class ScriptIPTrace : public ScriptTrace
		{
		public:
			ScriptIPTrace(DWORD dwIP, DWORD64 qwExecTimeMcS = 0) : ScriptTrace(dwIP, qwExecTimeMcS)
			{

			}

			virtual inline [[nodiscard]] std::unique_ptr<ScriptTrace> Clone() override
			{
				return std::make_unique<ScriptIPTrace>(*this);
			}
		};

		class ScriptRoutineTrace : public ScriptTrace
		{
		private:
			DWORD64 m_qwTimestamp = 0;

			bool m_bIsPaused = false;

		public:
			ScriptRoutineTrace(DWORD dwIP, DWORD64 qwExecTimeMcS = 0) : ScriptTrace(dwIP, qwExecTimeMcS)
			{
				m_qwTimestamp = Util::GetTimeMcS();
			}

			ScriptRoutineTrace(const ScriptRoutineTrace& scriptTrace) : ScriptTrace(scriptTrace)
			{
				m_qwTimestamp = scriptTrace.m_qwTimestamp;
			}

			ScriptRoutineTrace& operator=(const ScriptRoutineTrace& scriptTrace)
			{
				ScriptTrace::operator=(scriptTrace);
				m_qwTimestamp = scriptTrace.m_qwTimestamp;

				return *this;
			}

			ScriptRoutineTrace(ScriptRoutineTrace&& scriptTrace) noexcept : ScriptTrace(std::move(scriptTrace))
			{
				m_qwTimestamp = scriptTrace.m_qwTimestamp;
			}

			ScriptRoutineTrace& operator=(ScriptRoutineTrace&& scriptTrace) noexcept
			{
				ScriptTrace::operator=(std::move(scriptTrace));
				m_qwTimestamp = scriptTrace.m_qwTimestamp;

				return *this;
			}

			virtual inline [[nodiscard]] std::unique_ptr<ScriptTrace> Clone() override
			{
				return std::make_unique<ScriptRoutineTrace>(*this);
			}

			inline void Pause()
			{
				if (HasEnded() || m_bIsPaused)
				{
					return;
				}

				m_bIsPaused = true;

				Add(Util::GetTimeMcS() - m_qwTimestamp);
			}

			inline void Resume()
			{
				if (HasEnded() || !m_bIsPaused)
				{
					return;
				}

				m_bIsPaused = false;

				m_qwTimestamp = Util::GetTimeMcS();
			}

		private:
			virtual inline void _End() override
			{
				if (!m_bIsPaused)
				{
					Add(Util::GetTimeMcS() - m_qwTimestamp);
				}

				m_qwTimestamp = 0;
			}
		};

		std::unordered_map<DWORD, std::unique_ptr<ScriptTrace>> m_dictScriptTraces;
		std::vector<ScriptRoutineTrace> m_rgScriptRoutineTraces;
		mutable std::set<std::unique_ptr<ScriptTrace>, decltype(
			[] (const std::unique_ptr<ScriptTrace>& pLeftScriptTrace, const std::unique_ptr<ScriptTrace>& pRightScriptTrace)
			{
				return pLeftScriptTrace->GetMcS() > pRightScriptTrace->GetMcS();
			}
		)> m_rgFinalTraceSet;

		std::string m_szScriptName;
		ETraceMethod m_eTraceMethod = ETraceMethod::None;
		bool m_bIsCustomScript = false;

		DWORD64 m_qwExecTimeMcS = 0;
		mutable float m_fExecTimeSecs = 0.f;

		bool m_bHasEnded = false;

	public:
		DetailedScriptProfile(const std::string& szScriptName, ETraceMethod eTraceMethod, bool bIsCustomScript = false)
			: m_szScriptName(szScriptName), m_eTraceMethod(eTraceMethod), m_bIsCustomScript(bIsCustomScript)
		{

		}

		DetailedScriptProfile(const DetailedScriptProfile& scriptProfile)
			: DetailedScriptProfile(scriptProfile.m_szScriptName, scriptProfile.m_eTraceMethod, scriptProfile.m_bIsCustomScript)
		{
			for (auto& pair : scriptProfile.m_dictScriptTraces)
			{
				m_dictScriptTraces.emplace(pair.first, pair.second->Clone());
			}
			m_rgScriptRoutineTraces = scriptProfile.m_rgScriptRoutineTraces;
			for (auto& pScriptTrace : scriptProfile.m_rgFinalTraceSet)
			{
				m_rgFinalTraceSet.emplace(pScriptTrace->Clone());
			}
			m_qwExecTimeMcS = scriptProfile.m_qwExecTimeMcS;
			m_fExecTimeSecs = scriptProfile.m_fExecTimeSecs;
			m_bHasEnded = scriptProfile.m_bHasEnded;
		}

		DetailedScriptProfile(DetailedScriptProfile&& scriptProfile) noexcept
			: DetailedScriptProfile(m_szScriptName, scriptProfile.m_eTraceMethod, scriptProfile.m_bIsCustomScript)
		{
			m_szScriptName = std::move(scriptProfile.m_szScriptName);
			m_dictScriptTraces = std::move(scriptProfile.m_dictScriptTraces);
			m_rgScriptRoutineTraces = std::move(scriptProfile.m_rgScriptRoutineTraces);
			m_rgFinalTraceSet = std::move(scriptProfile.m_rgFinalTraceSet);
			m_qwExecTimeMcS = scriptProfile.m_qwExecTimeMcS;
			m_fExecTimeSecs = scriptProfile.m_fExecTimeSecs;
			m_bHasEnded = scriptProfile.m_bHasEnded;
		}

		bool operator<(const DetailedScriptProfile& scriptProfile) const
		{
			return m_qwExecTimeMcS < scriptProfile.m_qwExecTimeMcS;
		}

		bool operator>(const DetailedScriptProfile& scriptProfile) const
		{
			return m_qwExecTimeMcS > scriptProfile.m_qwExecTimeMcS;
		}

		inline [[nodiscard]] const std::string& GetScriptName() const
		{
			return m_szScriptName;
		}

		inline [[nodiscard]] ETraceMethod GetUsedTraceMethod() const
		{
			return m_eTraceMethod;
		}

		inline [[nodiscard]] bool IsCustomScript() const
		{
			return m_bIsCustomScript;
		}

		inline void AddWithTrace(DWORD dwIP, DWORD64 qwExecTimeMcS)
		{
			if (m_bHasEnded)
			{
				return;
			}

			m_qwExecTimeMcS += qwExecTimeMcS;

			if (dwIP >= 0 && m_eTraceMethod == ETraceMethod::IP)
			{
				if (m_dictScriptTraces.find(dwIP) == m_dictScriptTraces.end())
				{
					m_dictScriptTraces.emplace(dwIP, new ScriptIPTrace(dwIP));
				}

				m_dictScriptTraces.at(dwIP)->Add(qwExecTimeMcS);
			}
		}

		inline void SendMsg(EMessageType eMessageType, DWORD64 qwData = 0)
		{
			if (m_eTraceMethod != ETraceMethod::Routine)
			{
				return;
			}

			if (eMessageType == EMessageType::EnterCalled)
			{
				m_rgScriptRoutineTraces.emplace_back(qwData /* IP */);
			}
			else if (!m_rgScriptRoutineTraces.empty())
			{
				if (eMessageType == EMessageType::Pause)
				{
					for (ScriptRoutineTrace& scriptTrace : m_rgScriptRoutineTraces)
					{
						scriptTrace.Pause();
					}
				}
				else if (eMessageType == EMessageType::Resume)
				{
					for (ScriptRoutineTrace& scriptTrace : m_rgScriptRoutineTraces)
					{
						scriptTrace.Resume();
					}
				}
				else if (eMessageType == EMessageType::LeaveCalled)
				{
					ScriptRoutineTrace& scriptTrace = m_rgScriptRoutineTraces[m_rgScriptRoutineTraces.size() - 1];

					if (m_dictScriptTraces.find(scriptTrace.GetIP()) == m_dictScriptTraces.end())
					{
						m_dictScriptTraces.insert(std::make_pair(scriptTrace.GetIP(), std::make_unique<ScriptRoutineTrace>(scriptTrace.GetIP(), scriptTrace.GetMcS())));
					}
					else
					{
						m_dictScriptTraces.at(scriptTrace.GetIP())->Add(scriptTrace.GetMcS());
					}

					m_rgScriptRoutineTraces.erase(m_rgScriptRoutineTraces.end() - 1);
				}
			}
		}

		inline void End()
		{
			if (m_bHasEnded)
			{
				return;
			}

			m_bHasEnded = true;

			m_fExecTimeSecs = !m_qwExecTimeMcS ? m_qwExecTimeMcS : m_qwExecTimeMcS / 1000000.f;

			for (const auto& pair : m_dictScriptTraces)
			{
				pair.second->End();

				m_rgFinalTraceSet.insert(pair.second->Clone());
			}
		}

		inline [[nodiscard]] float GetSecs() const
		{
			if (m_bHasEnded)
			{
				return m_fExecTimeSecs;
			}

			return !m_qwExecTimeMcS ? m_qwExecTimeMcS : m_qwExecTimeMcS / 1000000.f;
		}

		inline [[nodiscard]] const auto& GetTraces() const
		{
			if (m_bHasEnded)
			{
				return m_rgFinalTraceSet;
			}

			m_rgFinalTraceSet.clear();
			for (auto& pair : m_dictScriptTraces)
			{
				auto clonedTrace = pair.second->Clone();
				clonedTrace->End();

				m_rgFinalTraceSet.insert(std::move(clonedTrace));
			}

			return m_rgFinalTraceSet;
		}
	};

	ETraceMethod m_eTraceMethod = ETraceMethod::Routine;

	std::unordered_map<DWORD, DetailedScriptProfile> m_dictScriptProfiles;
	std::mutex m_scriptProfilesMutex;
	std::set<DetailedScriptProfile, std::greater<DetailedScriptProfile>> m_rgFinalScriptProfileSet;

	DWORD64 m_qwLastCacheTimestamp = 0;

	std::atomic<bool> m_bIsRecording = false;
	std::atomic<float> m_fRecordTimeSecs = 0.f;
	DWORD64 m_qwRecordBeginTimestamp = 0;
	int m_ciFrames = 0;

public:
	RecordView() : Component()
	{

	}

	virtual bool RunHook(rage::scrThread* pScrThread) override;

	virtual void RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime) override;

	virtual void RunImGui() override;

	virtual void RunScript() override;

	virtual void ScriptRoutineCallback(rage::scrThread* pThread, ERoutineTraceType eTraceType, DWORD dwEnterIP) override;
};