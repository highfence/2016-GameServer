#pragma once

#include <vector>
#include <string>

#include "User.h"

namespace MySelectServerNetLib { class TcpNetwork; }
namespace MySelectServerNetLib { class ILogger; }
namespace NCommon { enum class ERROR_CODE :short;}

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{	
	using TcpNet = MySelectServerNetLib::TcpNetwork;
	using ILogger = MySelectServerNetLib::ILogger;

	class Room
	{
	public:
		Room();
		virtual ~Room();

		void Init(const short index, const short maxUserCount);

		void SetNetwork(TcpNet* pNetwork, ILogger* pLogger);

		void Clear();
		
		short GetIndex() { return m_Index;  }

		bool IsUsed() { return m_IsUsed; }

		const wchar_t* GetTitle() { return m_Title.c_str(); }

		short MaxUserCount() { return m_MaxUserCount; }

		short GetUserCount() { return (short)m_UserList.size(); }

		ERROR_CODE CreateRoom(const wchar_t* pRoomTitle);

		ERROR_CODE EnterUser(User* pUser);

		ERROR_CODE LeaveUser(const short userIndex);


		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);

		void NotifyEnterUserInfo(const int userIndex, const char* pszUserID);

		void NotifyLeaveUserInfo(const char* pszUserID);

		void NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg);

	private:
		ILogger* m_pRefLogger;
		TcpNet* m_pRefNetwork;

		short m_Index = -1;
		short m_MaxUserCount;
		
		bool m_IsUsed = false;
		std::wstring m_Title;
		std::vector<User*> m_UserList;
	};
}