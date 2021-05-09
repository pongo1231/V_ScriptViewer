#pragma once

typedef unsigned long DWORD;

namespace rage
{
	class scrThread;

	struct _ScriptStack
	{
		rage::scrThread* m_pScrThread;
		DWORD m_dwStackSize;
		void* m_pAllocatedMemory;
	};
}