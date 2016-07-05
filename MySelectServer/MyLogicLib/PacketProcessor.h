#pragma once

#include "../Common/Packet.h"
#include "../MySelectServerNetLib/Define.h"
#include "../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace MySelectServerNetLib
{
	class TcpNetwork;
	class ILogger;
}

namespace MyLogicLib
{
	class UserManager;
	class LobbyManager;

#define CHECK_START  ERROR_CODE __result=ERROR_CODE::NONE;
#define CHECK_ERROR(f) {__result=f; goto CHECK_ERR;}

	class PacketProcessor
	{
		using PacketInfo = MySelectServerNetLib::RecvPacketInfo;
		typedef ERROR_CODE(PacketProcessor::*PacketFunc)(PacketInfo);
		PacketFunc PacketFuncArray[(int)NCommon::PACKET_ID::MAX];\
		using TcpNet = MySelectServerNetLib::TcpNetwork;
		using ILogger = MySelectServerNetLib::ILogger;

	public:
		PacketProcessor();
		~PacketProcessor();

		void Init(TcpNet* network, UserManager* userManager, LobbyManager* lobbyManager, ILogger* logger);

		void Process(PacketInfo packetInfo);

	private:
		ILogger* _logger;
		TcpNet* _network;
		UserManager* _userManager;
		LobbyManager* _lobbyManager;


	private:

		ERROR_CODE Login(PacketInfo packetInfo);
		ERROR_CODE LobbyList(PacketInfo packetInfo);

		ERROR_CODE LobbyEnter(PacketInfo packetInfo);
		ERROR_CODE LobbyRoomList(PacketInfo packetInfo);
	};
}