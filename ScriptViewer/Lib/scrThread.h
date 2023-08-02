#pragma once

#include "../Memory/Hook.h"

#include "../vendor/scripthookv/main.h"

using WORD  = unsigned short;
using DWORD = unsigned long;

namespace rage
{
	class scrThread;

	struct _scrStack
	{
		rage::scrThread *m_pScrThread;
		DWORD m_dwStackSize;
		void *m_pAllocatedMemory;
	};

	class scrThread
	{
	  public:
		static inline scrThread **ms_ppThreads = nullptr;
		static inline WORD *ms_pcwThreads      = nullptr;

		static inline _scrStack *ms_pStacks    = nullptr;
		static inline WORD *ms_pcwStacks       = nullptr;

		DWORD m_dwThreadId;
		DWORD m_dwProgramId;
		DWORD dwSomething2;
		DWORD m_dwIP;
		char pad[184];
		char pad_2699[4];
		char m_szName[32];
		char pad3[100];
		char chSomething3;
		char pad4[3];

		virtual ~scrThread()                                 = 0;

		virtual DWORD *Reset(int a2, const void *a3, int a4) = 0;

		virtual __int64 Run()                                = 0;

		virtual __int64 Update()                             = 0;

		virtual __int64 Kill()                               = 0;

		const char *GetName() const
		{
			return getGameVersion() < eGameVersion::VER_1_0_2699_0_STEAM ? pad_2699 : m_szName;
		}

		inline rage::_scrStack *GetScriptStack()
		{
			if (!ms_pcwStacks)
			{
				return nullptr;
			}

			for (WORD wStackIdx = 0; wStackIdx < *ms_pcwStacks; wStackIdx++)
			{
				rage::_scrStack &stack = ms_pStacks[wStackIdx];

				if (stack.m_pScrThread == this)
				{
					return &stack;
				}
			}

			return nullptr;
		}
	};
}

static_assert(sizeof(rage::scrThread) == 352);