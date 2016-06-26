#pragma once
#include <vector>
#include <unordered_map>

namespace MySelectServerNetLib
{
	class TcpNetwork;
}

namespace MySelectServerNetLib
{
	class ILogger;
}

namespace NLogicLib
{
	struct LobbyManagerConfig
	{
		int maxLobbyCount;
		int maxLobbyUserCount;
		int maxRoomCountByLobby;
		int maxRoomUserCount;
	};

	struct LobbySmallInfo
	{
		short Num;
		short UserCount;
	};
		
	class Lobby;
	
	class LobbyManager
	{
		using TcpNet = MySelectServerNetLib::TcpNetwork;
		using ILogger = MySelectServerNetLib::ILogger;

	public:
		LobbyManager();
		virtual ~LobbyManager();

		void Init(const LobbyManagerConfig config, TcpNet* pNetwork, ILogger* pLogger);

		Lobby* GetLobby(short lobbyId);


	public:
		void SendLobbyListInfo(const int sessionIndex);


	


	private:
		ILogger* m_pRefLogger;
		TcpNet* m_pRefNetwork;

		std::vector<Lobby> m_LobbyList;
		
	};
}