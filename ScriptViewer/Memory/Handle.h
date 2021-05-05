#pragma once

typedef unsigned long long DWORD64;
typedef unsigned long DWORD;

class Handle
{
public:
	Handle() : m_dwAddr(0) {}
	Handle(DWORD64 addr) : m_dwAddr(addr) {}

	inline [[nodiscard]] bool IsValid() const
	{
		return m_dwAddr;
	}

	inline [[nodiscard]] Handle At(int iOffset) const
	{
		return IsValid() ? m_dwAddr + iOffset : 0;
	}

	template<typename T>
	inline [[nodiscard]] T* Get() const
	{
		return reinterpret_cast<T*>(m_dwAddr);
	}

	template<typename T>
	inline [[nodiscard]] T Value() const
	{
		return IsValid() ? *Get<T>() : 0;
	}

	inline [[nodiscard]] DWORD64 Addr() const
	{
		return m_dwAddr;
	}

	inline [[nodiscard]] Handle Into() const
	{
		if (IsValid())
		{
			Handle handle = At(1);
			return handle.At(handle.Value<DWORD>()).At(4);
		}

		return 0;
	}

private:
	DWORD64 m_dwAddr;
};