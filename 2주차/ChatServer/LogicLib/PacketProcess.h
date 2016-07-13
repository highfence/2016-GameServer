#pragma once

#include "../../Common/Packet.h"
#include "../MySelectServer/MySelectServerNetLib/Define.h"
//#include "../ServerNetLib/Define.h"
#include "../../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

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
	class UserManager;
	class LobbyManager;

	#define CHECK_START  ERROR_CODE __result=ERROR_CODE::NONE;
	#define CHECK_ERROR(f) __result=f; goto CHECK_ERR;

	class PacketProcess
	{
		using PacketInfo = MySelectServerNetLib::RecvPacketInfo;
		typedef ERROR_CODE(PacketProcess::*PacketFunc)(PacketInfo);
		PacketFunc PacketFuncArray[(int)NCommon::PACKET_ID::MAX];

		using TcpNet = MySelectServerNetLib::TcpNetwork;
		using ILogger = MySelectServerNetLib::ILogger;

	public:
		PacketProcess();
		~PacketProcess();

		void Init(TcpNet* pNetwork, UserManager* pUserMgr, LobbyManager* pLobbyMgr, ILogger* pLogger);

		void Process(PacketInfo packetInfo);
	
	private:
		ILogger* m_pRefLogger;
		TcpNet* m_pRefNetwork;
		UserManager* m_pRefUserMgr;
		LobbyManager* m_pRefLobbyMgr;
		
				
	private:
		ERROR_CODE NtfSysCloseSesson(PacketInfo packetInfo);
		
		ERROR_CODE Login(PacketInfo packetInfo);
		
		ERROR_CODE LobbyList(PacketInfo packetInfo);

		ERROR_CODE LobbyEnter(PacketInfo packetInfo);

		ERROR_CODE LobbyRoomList(PacketInfo packetInfo);

		ERROR_CODE LobbyUserList(PacketInfo packetInfo);

		ERROR_CODE LobbyLeave(PacketInfo packetInfo);

		ERROR_CODE RoomEnter(PacketInfo packetInfo);

		ERROR_CODE RoomLeave(PacketInfo packetInfo);

		ERROR_CODE RoomChat(PacketInfo packetInfo);

		ERROR_CODE LobbyChat(PacketInfo packetInfo);
	};
}