#include "../MySelectServerNetLib/ILogger.h"
#include "../MySelectServerNetLib/TcpNetwork.h"

#include "User.h"
#include "UserManager.h"
#include "PacketProcessor.h"
#include "Lobby.h"
#include "LobbyManager.h"

#include <iostream>

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

		PacketFuncArray[(int)netLibPacketId::NTF_SYS_CLOSE_SESSION] = &PacketProcessor::NtfSysCloseSesson;
		PacketFuncArray[(int)commonPacketId::LOGIN_IN_REQ] = &PacketProcessor::Login;
		PacketFuncArray[(int)commonPacketId::LOBBY_LIST_REQ] = &PacketProcessor::LobbyList;
		PacketFuncArray[(int)commonPacketId::LOBBY_ENTER_REQ] = &PacketProcessor::LobbyEnter;
		PacketFuncArray[(int)commonPacketId::LOBBY_ENTER_ROOM_LIST_REQ] = &PacketProcessor::LobbyRoomList;
		PacketFuncArray[(int)commonPacketId::ROOM_ENTER_REQ] = &PacketProcessor::RoomEnter;
		PacketFuncArray[(int)commonPacketId::ROOM_CHAT_REQ] = &PacketProcessor::RoomChat;
		PacketFuncArray[(int)commonPacketId::LOBBY_CHAT_REQ] = &PacketProcessor::LobbyChat;
		PacketFuncArray[(int)commonPacketId::WHISPER_REQ] = &PacketProcessor::Whisper;
		PacketFuncArray[(int)commonPacketId::LOBBY_ENTER_USER_LIST_REQ] = &PacketProcessor::LobbyUserList;
		PacketFuncArray[(int)commonPacketId::ROOM_ENTER_USER_LIST_REQ] = &PacketProcessor::RoomUserList;
		PacketFuncArray[(int)commonPacketId::LOBBY_LEAVE_REQ] = &PacketProcessor::LobbyLeave;
		PacketFuncArray[(int)commonPacketId::ROOM_LEAVE_REQ] = &PacketProcessor::RoomLeave;
	}

	void PacketProcessor::Process(PacketInfo packetInfo)
	{
		auto packetId = packetInfo.packetId;
		auto packetName = (NCommon::PACKET_ID)packetId;

		if (PacketFuncArray[packetId] == nullptr)
		{
			std::cout << "아니 이런 세상에.. 모르는 패킷id가 날아왔어요. 패킷 id : " << packetId << ", 바디싸이즈 : " << packetInfo.packetBodySize << std::endl;
		}
		else
			(this->*PacketFuncArray[packetId])(packetInfo);
	}
	ERROR_CODE PacketProcessor::NtfSysCloseSesson(PacketInfo packetInfo)
	{
		auto pUser = std::get<1>(_userManager->GetUser(packetInfo.sessionIndex));

		if (pUser)
		{
			auto pLobby = _lobbyManager->GetLobby(pUser->GetLobbyIndex());
			if (pLobby)
			{
				auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());

				if (pRoom)
				{
					pRoom->LeaveUser(pUser->GetIndex());
					pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());
					pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

					_logger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Room Out", __FUNCTION__, packetInfo.sessionIndex);
				}

				pLobby->LeaveUser(pUser->GetIndex());

				if (pRoom == nullptr) {
					pLobby->NotifyLobbyLeaveUserInfo(pUser);
				}

				_logger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Lobby Out", __FUNCTION__, packetInfo.sessionIndex);
			}

			_userManager->RemoveUser(packetInfo.sessionIndex);
		}


		_logger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d)", __FUNCTION__, packetInfo.sessionIndex);
		return ERROR_CODE::NONE;
	}
}