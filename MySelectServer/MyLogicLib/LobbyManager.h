#pragma once

#include <vector>
#include <unordered_map>

namespace MySelectServerNetLib
{
	class TcpNetwork;
	class ILogger;
}

namespace MyLogicLib
{
	struct LobbyManagerConfig
	{
		int maxLobbyCount;
		int maxUserPerLobby;
		int maxRoomPerLobby;
		int maxUserPerRoom;
	};

	struct LobbySmallInfo
	{
		short num;
		short userCount;
	};

	class Lobby;

	class LobbyManager
	{
		using TcpNet = MySelectServerNetLib::TcpNetwork;
		using ILogger = MySelectServerNetLib::ILogger;

	public:
		LobbyManager();
		virtual ~LobbyManager();

		void	Init(const LobbyManagerConfig config, TcpNet* network, ILogger* logger);

		Lobby*	GetLobby(short lobbyId);
	
	public:
		void	SendLobbyListInfo(const int sessionIndex);

	private:
		ILogger*	_logger;
		TcpNet*		_netWork;

		std::vector<Lobby> _lobbyList;
	};
}