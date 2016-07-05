#include "../MySelectServerNetLib/ILogger.h"
#include "../MySelectServerNetLib/TcpNetwork.h"
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"

#include "Lobby.h"
#include "LobbyManager.h"

using ERROR_CODE = NCommon::ERROR_CODE;
using PACKET_ID = NCommon::PACKET_ID;

namespace MyLogicLib
{
	LobbyManager::LobbyManager() {}
	LobbyManager::~LobbyManager() {}

	void LobbyManager::Init(const LobbyManagerConfig config, TcpNet* network, ILogger* logger)
	{
		_logger = logger;
		_netWork = network;

		for(int i = 0; i < config.maxLobbyCount; ++i)
		{
			Lobby newLobby;
			newLobby.Init((short)i, (short)config.maxUserPerLobby, (short)config.maxRoomPerLobby, (short)config.maxUserPerRoom);
			newLobby.SetNetwork(_netWork, _logger);

			_lobbyList.push_back(newLobby);

		}
	}

	// 로비 id를 넣으면 로비를 꺼내준다.
	MyLogicLib::Lobby* LobbyManager::GetLobby(short lobbyId)
	{
		if (lobbyId < 0 || lobbyId >= (short)_lobbyList.size())
			return nullptr;

		return &_lobbyList[lobbyId];
	}

	void LobbyManager::SendLobbyListInfo(const int sessionIndex)
	{
		// 응답용 패킷
		NCommon::PktLobbyListRes resPacket;
		resPacket.ErrorCode = (short)ERROR_CODE::NONE;
		resPacket.LobbyCount = static_cast<short>(_lobbyList.size());

		// 모든 로비에 대한 정보를 담는다
		int index = 0;
		for (auto& lobby : _lobbyList)
		{
			resPacket.LobbyList[index].LobbyId = lobby.GetIndex();
			resPacket.LobbyList[index].LobbyUserCount = lobby.GetUserCount();

			++index;
		}

		// 클라한테 전송
		_netWork->SendData(sessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(resPacket), (char*)&resPacket);
	}

}