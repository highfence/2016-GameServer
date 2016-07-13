#pragma once

#include <vector>
#include <unordered_map>

#include "Room.h"

namespace MyLogicLib
{
	struct LobbyUser
	{
		short index = 0;
		User* user = nullptr;
	};

	class Lobby
	{
	public:
		Lobby();
		virtual ~Lobby();

		void							Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountPerLobby, const short maxUserPerRoom);

		void							SetNetwork(TcpNet* network, ILogger* logger);

		short							GetIndex() { return _lobbyIndex; }

		short							GetUserCount();

		ERROR_CODE						EnterUser(User* user);
		ERROR_CODE						LeaveUser(const int userIndex);

		void							NotifyLobbyEnterUserInfo(User* user);

		void							NotifyLobbyLeaveUserInfo(User* user);

		void							NotifyChangedRoomInfo(const short roomIndex);

		void							NotifyChat(const short userIndex, std::string userId, std::wstring message);

		auto							MaxUserCount() { return (short)_maxUserCount; }

		auto							MaxRoomCount() { return (short)_roomList.size(); }

		ERROR_CODE						SendRoomList(const int sessionId, const short startRoomId);

		ERROR_CODE						SendUserList(const int sessionId, const short startUserIndex);

		Room*							CreateRoom();

		Room*							GetRoom(const short roomIndex);

	private:

		User*							FindUser(int userIndex);

		ERROR_CODE						AddUser(User* user);


		void							SendToAllUser(const short packetId, const short dataSize, char* dataPos, const int passUserIndex = -1);
		void							RemoveUser(const int userIndex);
	private:
		TcpNet*							_network;
		ILogger*						_logger;

		short _lobbyIndex = 0;
		short _maxUserCount;

		std::unordered_map<int, User*>	_userIndexDic;
		std::unordered_map<std::string, User*> _userIdDic;
		std::vector<LobbyUser>			_userList;
		std::vector<Room>				_roomList;
	};
}