#include <stdafx.h>

#include "Memory.h"

#include <vector>

DWORD64 m_baseAddr;
DWORD64 m_endAddr;

namespace Memory
{
	void Init()
	{
		MODULEINFO moduleInfo;
		GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &moduleInfo, sizeof(moduleInfo));

		m_baseAddr = reinterpret_cast<DWORD64>(moduleInfo.lpBaseOfDll);
		m_endAddr = m_baseAddr + moduleInfo.SizeOfImage;
	}

	void InitHooks()
	{
		MH_Initialize();

		for (void(*pHook)() : Memory::g_rgHooks)
		{
			pHook();
		}
	}

	void FinishHooks()
	{
		MH_EnableHook(MH_ALL_HOOKS);
	}

	void Uninit()
	{

	}

	Handle FindPattern(const std::string& szPattern)
	{
		std::vector<int> rgiBytes;

		std::string szSub = szPattern;
		int offset = 0;
		while ((offset = szSub.find(' ')) != szSub.npos)
		{
			std::string szByteStr = szSub.substr(0, offset);

			if (szByteStr == "?" || szByteStr == "??")
			{
				rgiBytes.push_back(-1);
			}
			else
			{
				rgiBytes.push_back(std::stoi(szByteStr, nullptr, 16));
			}

			szSub = szSub.substr(offset + 1);
		}
		if ((offset = szPattern.rfind(' ')) != szSub.npos)
		{
			std::string szByteStr = szPattern.substr(offset + 1);
			rgiBytes.push_back(std::stoi(szByteStr, nullptr, 16));
		}

		if (rgiBytes.empty())
		{
			return Handle();
		}

		int iCount = 0;
		for (DWORD64 dwAddr = m_baseAddr; dwAddr < m_endAddr; dwAddr++)
		{
			if (rgiBytes[iCount] == -1 || *reinterpret_cast<BYTE*>(dwAddr) == rgiBytes[iCount])
			{
				if (++iCount == rgiBytes.size())
				{
					return Handle(dwAddr - iCount + 1);
				}
			}
			else
			{
				iCount = 0;
			}
		}

		LOG("Couldn't find pattern \"" << szPattern << "\"");

		return Handle();
	}

	MH_STATUS AddHook(void* pTarget, void* pDetour, void* pOrig)
	{
		MH_STATUS result = MH_CreateHook(pTarget, pDetour, reinterpret_cast<void**>(pOrig));

		if (result == MH_OK)
		{
			MH_EnableHook(pTarget);
		}

		return result;
	}

	const char* const GetTypeName(__int64 vptr)
	{
		if (vptr)
		{
			__int64 vftable = *reinterpret_cast<__int64*>(vptr);
			if (vftable)
			{
				__int64 rtti = *reinterpret_cast<__int64*>(vftable - 8);
				if (rtti)
				{
					__int64 rva = *reinterpret_cast<DWORD*>(rtti + 12);
					if (rva)
					{
						__int64 typeDesc = m_baseAddr + rva;
						if (typeDesc)
						{
							return reinterpret_cast<const char*>(typeDesc + 16);
						}
					}
				}
			}
		}

		return "UNK";
	}
}