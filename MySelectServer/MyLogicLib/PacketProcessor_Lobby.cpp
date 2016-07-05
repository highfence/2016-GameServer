#include "../Common/Packet.h"
#include "../MySelectServerNetLib/TcpNetwork.h"
#include "../Common/ErrorCode.h"

#include "User.h"
#include "UserManager.h"
#include "Lobby.h"
#include "LobbyManager.h"
#include "PacketProcessor.h"

#include <iostream>

using PACKET_ID = NCommon::PACKET_ID;

namespace MyLogicLib
{
	ERROR_CODE PacketProcessor::LobbyEnter(PacketInfo packetInfo)
	{
		CHECK_START;
		// ���� ��ġ�� �α����� �³�?
		// �κ� ����.
		// ���� �κ� �ִ� ������� �� ����� ���Դٰ� �˷��ش�.

		// ���� ��Ŷ �ٵ�(��� �κ�� ���� ������)
		auto reqPacket = (NCommon::PktLobbyEnterReq*)packetInfo.dataAddress;

		// ���� ��Ŷ
		NCommon::PktLobbyEnterRes resPacket;

		// �����ϰ� ������ �޾ƿ�
		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		auto user = std::get<1>(result);

		// ���� �α��� �� ���������� üũ
		if (user->IsLoginState() == false)
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN);

		// ���� �� �Ӥ��� �κ� �޾ƿ´�
		auto lobby = _lobbyManager->GetLobby(reqPacket->LobbyId);
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX);

		// �κ� ������ ����ִ´�.
		auto enterResult = lobby->EnterUser(user);

		// ��� �������� ���� ������ ���Դٰ� �˸���.
		lobby->NotifyLobbyEnterUserInfo(user);

		// ���� ��Ŷ�� ������ ��� �ִ´�.
		resPacket.MaxRoomCount = lobby->MaxRoomCount();
		resPacket.MaxUserCount = lobby->MaxUserCount();
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPacket);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	// �� ����Ʈ�� ��û�޾��� �� ó���ϴ� �Լ�
	ERROR_CODE PacketProcessor::LobbyRoomList(PacketInfo packetInfo)
	{
		CHECK_START;
		// ���� �κ� �ִ��� �����Ѵ�.
		// �� ����Ʈ�� �����ش�.

		auto pUserRet = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsLobbyState() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_DOMAIN);
		}

		auto pLobby = _lobbyManager->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_LOBBY_INDEX);
		}

		auto reqPkt = (NCommon::PktLobbyRoomListReq*)packetInfo.dataAddress;

		pLobby->SendRoomList(pUser->GetSessionIndex(), reqPkt->StartRoomIndex);
		std::cout << std::endl << reqPkt->StartRoomIndex << std::endl;

		return ERROR_CODE::NONE;
	CHECK_ERR:
		NCommon::PktLobbyRoomListRes resPkt;
		resPkt.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

}