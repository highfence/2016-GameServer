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

		bool IsUsed() { return _isUsed; }

		void Clear();

		short GetIndex() { return _index; }

		short GetUserCount() { return (short)_userList.size(); }

		const std::wstring GetTitle() { return _title; }

		void SetTitle(const std::wstring title) { _title = title; }

		ERROR_CODE CreateRoom(const std::wstring roomTitle);

		ERROR_CODE EnterUser(User* pUser);

		ERROR_CODE LeaveUser(const short userIndex);

		void NotifyEnterUserInfo(const short userIndex, const std::string userId);

		void NotifyLeaveUserInfo(const char* pszUserID);

		void NotifyChat(const short userIndex, std::string userId, std::wstring message);
		
		void SendToAllUser(const short packetId, const short dataSize, char* dataPos, const int passUserIndex = -1);
		
		void SetNetwork(TcpNet* network) { _network = network; }

		ERROR_CODE SendUserList(const int sessionId);

	private:
		short _index = -1;
		short _maxUserCount;

		bool _isUsed = false;

		std::vector<User*> _userList;
		std::wstring _title;

		TcpNet*	_network;
	};

}