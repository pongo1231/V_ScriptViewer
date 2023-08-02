#pragma once

#include <fstream>
#include <iostream>
#include <sstream>

inline std::ofstream g_log("scriptviewerlog.txt");

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define _LOG(_text, _stream) _stream << _text

#define RAW_LOG(_text)      \
	do                      \
	{                       \
		_LOG(_text, g_log); \
	} while (0)

#define LOG(_text) RAW_LOG("[" << __FILENAME__ << "] " << _text << std::endl)

#ifdef _DEBUG
#define DEBUG_LOG(_text) LOG(_text)
#else
#define DEBUG_LOG(_text)
#endif