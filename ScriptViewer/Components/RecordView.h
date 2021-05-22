#pragma once

#include "Component.h"

#include <unordered_map>
#include <vector>
#include <set>
#include <mutex>

typedef unsigned long DWORD;
typedef unsigned long long DWORD64;

class ScriptProfile;

class RecordView : public Component
{
private:
	class DetailedScriptProfile
	{
	private:
		class ScriptTrace
		{
		private:
			DWORD m_dwIP;
			DWORD64 m_qwExecTimeNs = 0;
			float m_fExecTimeSecs = 0.f;

			bool m_bHasEnded = false;

		public:
			ScriptTrace(DWORD dwIP) : m_dwIP(dwIP)
			{

			}

			ScriptTrace(const ScriptTrace& scriptTrace) : ScriptTrace(scriptTrace.m_dwIP)
			{
				m_qwExecTimeNs = scriptTrace.m_qwExecTimeNs;
				m_fExecTimeSecs = scriptTrace.m_fExecTimeSecs;
			}

			ScriptTrace& operator=(const ScriptTrace& scriptTrace)
			{
				m_dwIP = scriptTrace.m_dwIP;
				m_qwExecTimeNs = scriptTrace.m_qwExecTimeNs;
				m_fExecTimeSecs = scriptTrace.m_fExecTimeSecs;

				return *this;
			}

			ScriptTrace(ScriptTrace&& scriptTrace) noexcept : ScriptTrace(scriptTrace.m_dwIP)
			{
				m_qwExecTimeNs = scriptTrace.m_qwExecTimeNs;
				m_fExecTimeSecs = scriptTrace.m_fExecTimeSecs;
			}

			ScriptTrace& operator=(ScriptTrace&&) = delete;

			bool operator<(const ScriptTrace& scriptTrace) const
			{
				return m_qwExecTimeNs < scriptTrace.m_qwExecTimeNs;
			}

			bool operator>(const ScriptTrace& scriptTrace) const
			{
				return m_qwExecTimeNs > scriptTrace.m_qwExecTimeNs;
			}

			bool operator==(DWORD dwIP) const
			{
				return m_dwIP;
			}

			inline void Add(DWORD64 qwExecTimeNs)
			{
				if (m_bHasEnded)
				{
					return;
				}

				m_qwExecTimeNs += qwExecTimeNs;
			}

			inline void End()
			{
				m_bHasEnded = true;

				m_fExecTimeSecs = !m_qwExecTimeNs ? m_qwExecTimeNs : m_qwExecTimeNs / 1000000000.f;
			}

			inline [[nodscard]] DWORD GetIP() const
			{
				return m_dwIP;
			}

			inline [[nodiscard]] float GetSecs() const
			{
				if (m_bHasEnded)
				{
					return m_fExecTimeSecs;
				}

				return !m_qwExecTimeNs ? m_qwExecTimeNs : m_qwExecTimeNs / 1000000000.f;
			}
		};

		std::unordered_map<DWORD, ScriptTrace> m_dictScriptTraces;
		mutable std::set<ScriptTrace, std::greater<ScriptTrace>> m_rgFinalTraceSet;

		std::string m_szScriptName;

		DWORD64 m_qwExecTimeNs = 0;
		mutable float m_fExecTimeSecs = 0.f;

		bool m_bHasEnded = false;

	public:
		DetailedScriptProfile(const std::string& szScriptName) : m_szScriptName(szScriptName)
		{

		}

		DetailedScriptProfile(std::string&& szScriptName) : m_szScriptName(std::move(szScriptName))
		{

		}

		DetailedScriptProfile(const DetailedScriptProfile& scriptProfile) : DetailedScriptProfile(scriptProfile.m_szScriptName)
		{
			m_dictScriptTraces = scriptProfile.m_dictScriptTraces;
			m_rgFinalTraceSet = scriptProfile.m_rgFinalTraceSet;
			m_qwExecTimeNs = scriptProfile.m_qwExecTimeNs;
			m_fExecTimeSecs = scriptProfile.m_fExecTimeSecs;
			m_bHasEnded = scriptProfile.m_bHasEnded;
		}

		DetailedScriptProfile& operator=(const DetailedScriptProfile&) = delete;

		DetailedScriptProfile(DetailedScriptProfile&& scriptProfile) noexcept : DetailedScriptProfile(std::move(scriptProfile.m_szScriptName))
		{
			m_dictScriptTraces = std::move(scriptProfile.m_dictScriptTraces);
			m_rgFinalTraceSet = std::move(scriptProfile.m_rgFinalTraceSet);
			m_qwExecTimeNs = scriptProfile.m_qwExecTimeNs;
			m_fExecTimeSecs = scriptProfile.m_fExecTimeSecs;
			m_bHasEnded = scriptProfile.m_bHasEnded;
		}

		DetailedScriptProfile& operator=(DetailedScriptProfile&&) = delete;

		bool operator<(const DetailedScriptProfile& scriptProfile) const
		{
			return m_qwExecTimeNs < scriptProfile.m_qwExecTimeNs;
		}

		bool operator>(const DetailedScriptProfile& scriptProfile) const
		{
			return m_qwExecTimeNs > scriptProfile.m_qwExecTimeNs;
		}

		inline [[nodiscard]] const std::string& GetScriptName() const
		{
			return m_szScriptName;
		}

		inline void AddWithTrace(DWORD dwIP, DWORD64 qwExecTimeNs)
		{
			if (m_bHasEnded)
			{
				return;
			}

			m_qwExecTimeNs += qwExecTimeNs;

			if (dwIP != -1)
			{
				if (m_dictScriptTraces.find(dwIP) == m_dictScriptTraces.end())
				{
					m_dictScriptTraces.emplace(dwIP, dwIP);
				}

				m_dictScriptTraces.at(dwIP).Add(qwExecTimeNs);
			}
		}

		inline void End()
		{
			if (m_bHasEnded)
			{
				return;
			}

			m_bHasEnded = true;

			m_fExecTimeSecs = !m_qwExecTimeNs ? m_qwExecTimeNs : m_qwExecTimeNs / 1000000000.f;

			for (const auto& pair : m_dictScriptTraces)
			{
				m_rgFinalTraceSet.insert(pair.second);
			}
		}

		inline [[nodsicard]] float GetSecs() const
		{
			if (m_bHasEnded)
			{
				return m_fExecTimeSecs;
			}

			return !m_qwExecTimeNs ? m_qwExecTimeNs : m_qwExecTimeNs / 1000000000.f;
		}

		inline [[nodiscard]] const auto& GetTraces() const
		{
			if (m_bHasEnded)
			{
				return m_rgFinalTraceSet;
			}

			m_rgFinalTraceSet.clear();
			for (const auto& pair : m_dictScriptTraces)
			{
				m_rgFinalTraceSet.insert(pair.second);
			}

			return m_rgFinalTraceSet;
		}
	};

	std::unordered_map<DWORD, DetailedScriptProfile> m_dictScriptProfiles;
	std::mutex m_scriptProfilesMutex;
	std::set<DetailedScriptProfile, std::greater<DetailedScriptProfile>> m_rgFinalScriptProfileSet;

	std::atomic<bool> m_bIsRecording = false;
	std::atomic<float> m_fRecordTimeSecs = 0.f;

public:
	RecordView() : Component()
	{

	}

	RecordView(const RecordView&) = delete;

	RecordView& operator=(const RecordView&) = delete;

	RecordView(RecordView&& recordView) noexcept : Component(std::move(recordView))
	{

	}

	RecordView& operator=(RecordView&&) = delete;

	virtual void RunCallback(rage::scrThread* pScrThread, DWORD64 qwExecutionTime) override;

	virtual void RunImGui() override;

	virtual void RunScript() override;
};