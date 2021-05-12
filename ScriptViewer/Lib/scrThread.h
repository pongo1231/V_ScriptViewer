#pragma once

typedef unsigned short WORD;

namespace rage
{
	class scrThread;

	struct _scrStack
	{
		rage::scrThread* m_pScrThread;
		DWORD m_dwStackSize;
		void* m_pAllocatedMemory;
	};

	class scrThread
	{
	public:
		static inline scrThread** sm_Threads = nullptr;
		static inline WORD* _sm_cwThreads = nullptr;

		static inline _scrStack* sm_Stacks = nullptr;
		static inline WORD* _sm_cwStacks = nullptr;

		DWORD m_dwThreadId;
		DWORD m_dwProgramId;
		DWORD dwSomething2;
		char pad[188];
		char m_szName[32];
		char pad3[100];
		char chSomething3;
		char pad4[3];

		virtual ~scrThread() = 0;

		virtual DWORD* Reset(int a2, const void* a3, int a4) = 0;

		virtual __int64 Run() = 0;

		virtual __int64 Update() = 0;

		virtual __int64 Kill() = 0;
	};
}

static_assert(sizeof(rage::scrThread) == 344);