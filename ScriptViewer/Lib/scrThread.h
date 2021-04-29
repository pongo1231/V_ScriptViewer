#pragma once

namespace rage
{
	struct scrThread
	{
		void** m_pVft;
		DWORD m_dwSomething;
		DWORD m_dwProgramId;
		DWORD m_dwSomething2;
		char m_pad2[188];
		char m_szName[32];
		char m_pad3[100];
		char m_chSomething3;
		char m_pad4[3];
	};
}

static_assert(sizeof(rage::scrThread) == 344);