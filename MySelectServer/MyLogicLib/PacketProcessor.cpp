#include "../MySelectServerNetLib/ILogger.h"
#include "../MySelectServerNetLib/TcpNetwork.h"

#include "UserManager.h"
#include "PacketProcessor.h"

using LOG_TYPE = MySelectServerNetLib::LOG_TYPE;

namespace MyLogicLib
{
	PacketProcessor::PacketProcessor() {}
	PacketProcessor::~PacketProcessor() {}

	void PacketProcessor::Init(TcpNet* network, UserManager* userManager, LobbyManager* lobbyManager, ILogger* logger)
	{
		_network = network;
		_userManager = userManager;
		_lobbyManager = lobbyManager;
		_logger = logger;

		using netLibPacketId = MySelectServerNetLib::PACKET_ID;
		using commonPacketId = NCommon::PACKET_ID;

		for (int i = 0; i < (int)commonPacketId::MAX; ++i)
		{
			PacketFuncArray[i] = nullptr;
		}

		PacketFuncArray[(int)commonPacketId::LOGIN_IN_REQ] = &PacketProcessor::Login;
		PacketFuncArray[(int)commonPacketId::LOBBY_LIST_REQ] = &PacketProcessor::LobbyList;
		PacketFuncArray[(int)commonPacketId::LOBBY_ENTER_REQ] = &PacketProcessor::LobbyEnter;
		PacketFuncArray[(int)commonPacketId::LOBBY_ENTER_ROOM_LIST_REQ] = &PacketProcessor::LobbyRoomList;
	}

	void PacketProcessor::Process(PacketInfo packetInfo)
	{
		auto packetId = packetInfo.packetId;
		auto packetName = (NCommon::PACKET_ID)packetId;

		if (packetName == NCommon::PACKET_ID::LOBBY_ENTER_USER_LIST_REQ)
			return;

		if (PacketFuncArray[packetId] == nullptr)
		{
			// TODO : 로그 남기기
		}
		(this->*PacketFuncArray[packetId])(packetInfo);
	}

}