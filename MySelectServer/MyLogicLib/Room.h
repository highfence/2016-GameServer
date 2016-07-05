#pragma once

#include <vector>
#include <string>

#include "User.h"

namespace MySelectServerNetLib
{
	class TcpNetwork;
	class ILogger;
}

namespace NCommon { enum class ERROR_CODE :short; }

using ERROR_CODE = NCommon::ERROR_CODE;

namespace MyLogicLib
{
	using TcpNet = MySelectServerNetLib::TcpNetwork;
	using ILogger = MySelectServerNetLib::ILogger;

	class Room
	{
	public:

		void Init(const short index, const short maxUserCount);

		bool IsUsed() { return true; }

		short GetIndex() { return _index; }

		short GetUserCount() { return (short)_userList.size(); }

		const std::wstring GetTitle() { return _title; }

		void SetTitle(const std::wstring title) { _title = title; }

	private:
		short _index = -1;
		short _maxUserCount;

		std::vector<User*> _userList;
		std::wstring _title;
	};

}