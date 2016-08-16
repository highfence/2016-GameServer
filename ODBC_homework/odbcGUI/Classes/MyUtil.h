#pragma once

#include "pch.h"

class MyUtil
{
public:
	static std::wstring utf8toWstring(std::string& src)
	{
		wchar_t strUnicode[256] = { 0, };
		int nLen = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), strlen(src.c_str()), NULL, NULL);
		MultiByteToWideChar(CP_UTF8, 0, src.c_str(), strlen(src.c_str()), strUnicode, nLen);
		auto w = std::wstring(strUnicode);
		return w;
	};
};