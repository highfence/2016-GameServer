#pragma once

#include <vector>
#include <unordered_map>

#include "Room.h"

namespace MySelectServerNetLib
{
	class TcpNetwork;
}

namespace MySelectServerNetLib
{
	class ILogger;
}

namespace NCommon
{
	enum class ERROR_CODE :short;
}
using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{
	using TcpNet = MySelectServerNetLib::TcpNetwork;
	using ILogger = MySelectServerNetLib::ILogger;

	class User;
	
	struct LobbyUser
	{
		short Index = 0;
		User* pUser = nullptr;
	};

	class Lobby
	{
	public:
		Lobby();
		virtual ~Lobby();

		void Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount);
		
		void SetNetwork(TcpNet* pNetwork, ILogger* pLogger);

		short GetIndex() { return m_LobbyIndex; }


		ERROR_CODE EnterUser(User* pUser);
		ERROR_CODE LeaveUser(const int userIndex);
		
		short GetUserCount();

		
		void NotifyLobbyEnterUserInfo(User* pUser);
		
		ERROR_CODE SendRoomList(const int sessionId, const short startRoomId);

		ERROR_CODE SendUserList(const int sessionId, const short startUserIndex);

		void NotifyLobbyLeaveUserInfo(User* pUser);

		void NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg);
		
		Room* GetRoom(const short roomIndex);

		void NotifyChangedRoomInfo(const short roomIndex);

		auto MaxUserCount() { return (short)m_MaxUserCount; }

		auto MaxRoomCount() { return (short)m_RoomList.size(); }
		Room* Lobby::CreateRoom()
		{
			for (int i = 0; i < m_RoomList.size(); ++i)
			{
				if (m_RoomList[i].IsUsed() == false) {
					return &m_RoomList[i];
				}
			}
			return nullptr;
		}
	protected:
		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);

		

	protected:
		User* FindUser(const int userIndex);

		ERROR_CODE AddUser(User* pUser);

		void RemoveUser(const int userIndex);


	protected:
		ILogger* m_pRefLogger;
		TcpNet* m_pRefNetwork;


		short m_LobbyIndex = 0;

		short m_MaxUserCount = 0;
		std::vector<LobbyUser> m_UserList;
		std::unordered_map<int, User*> m_UserIndexDic;
		std::unordered_map<const char*, User*> m_UserIDDic;

		std::vector<Room> m_RoomList;
	};
}

