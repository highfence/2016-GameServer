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

		void							NotifyLobbyEnterUserInfo(User* user);

		auto							MaxUserCount() { return (short)_maxUserCount; }

		auto							MaxRoomCount() { return (short)_roomList.size(); }

		ERROR_CODE						SendRoomList(const int sessionId, const short startRoomId);

	private:

		User*							FindUser(int userIndex);

		ERROR_CODE						AddUser(User* user);


		void							SendToAllUser(const short packetId, const short dataSize, char* dataPos, const int passUserIndex);

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