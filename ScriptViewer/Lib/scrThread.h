#pragma once

namespace rage
{
	struct scrThread
	{
		void** m_pVft;
		DWORD _m_dwRunningFlags;
		DWORD m_dwProgramId;
		DWORD dwSomething2;
		char pad[188];
		char m_szName[32];
		char pad3[100];
		char chSomething3;
		char pad4[3];
	};
}

static_assert(sizeof(rage::scrThread) == 344);