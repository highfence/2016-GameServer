
#include "../MySelectServer/MySelectServerNetLib/ILogger.h"
//#include "../ServerNetLib/ILog.h"
#include "../MySelectServer/MySelectServerNetLib/TcpNetwork.h"
//#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/Packet.h"
#include "../../Common/ErrorCode.h"

#include "Lobby.h"
#include "LobbyManager.h"

using ERROR_CODE = NCommon::ERROR_CODE;
using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	LobbyManager::LobbyManager() {}

	LobbyManager::~LobbyManager() {}


	void LobbyManager::Init(const LobbyManagerConfig config, TcpNet* pNetwork, ILogger* pLogger)
	{
		m_pRefLogger = pLogger;
		m_pRefNetwork = pNetwork;

		for (int i = 0; i < config.maxLobbyCount; ++i)
		{
			Lobby lobby;
			lobby.Init((short)i, (short)config.maxLobbyUserCount, (short)config.maxRoomCountByLobby, (short)config.maxRoomUserCount);
			lobby.SetNetwork(m_pRefNetwork, m_pRefLogger);

			m_LobbyList.push_back(lobby);
		}
	}

	// 로비 id를 넣으면 로비를 꺼내준다.
	Lobby* LobbyManager::GetLobby(short lobbyId)
	{
		if (lobbyId < 0 || lobbyId >= (short)m_LobbyList.size()) {
			return nullptr;
		}

		return &m_LobbyList[lobbyId];
	}
		
	void LobbyManager::SendLobbyListInfo(const int sessionIndex)
	{
		NCommon::PktLobbyListRes resPkt;
		resPkt.ErrorCode = (short)ERROR_CODE::NONE;
		resPkt.LobbyCount = static_cast<short>(m_LobbyList.size());

		int index = 0;
		for (auto& lobby : m_LobbyList)
		{
			resPkt.LobbyList[index].LobbyId = lobby.GetIndex();
			resPkt.LobbyList[index].LobbyUserCount = lobby.GetUserCount();

			++index;
		}

		// 보낼 데이터를 줄이기 위해 사용하지 않은 LobbyListInfo 크기는 빼고 보내도 된다.
		m_pRefNetwork->SendData(sessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(resPkt), (char*)&resPkt);
	}

}