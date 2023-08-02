#pragma once

#include "../Memory/Hook.h"

typedef unsigned short WORD;
typedef unsigned int DWORD_t;

namespace rage
{
	class scrThread;

	struct _scrStack
	{
		rage::scrThread *m_pScrThread;
		DWORD_t m_dwStackSize;
		void *m_pAllocatedMemory;
	};

	class scrThread
	{
	  public:
		static inline scrThread **ms_ppThreads = nullptr;
		static inline WORD *ms_pcwThreads      = nullptr;

		static inline _scrStack *ms_pStacks    = nullptr;
		static inline WORD *ms_pcwStacks       = nullptr;

		DWORD_t m_dwThreadId;
		DWORD_t m_dwProgramId;
		DWORD_t dwSomething2;
		DWORD_t m_dwIP;
		char pad[184];
		char m_szName[32];
		char pad3[100];
		char chSomething3;
		char pad4[3];

		virtual ~scrThread()                                 = 0;

		virtual DWORD *Reset(int a2, const void *a3, int a4) = 0;

		virtual __int64 Run()                                = 0;

		virtual __int64 Update()                             = 0;

		virtual __int64 Kill()                               = 0;

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

static_assert(sizeof(rage::scrThread) == 344);