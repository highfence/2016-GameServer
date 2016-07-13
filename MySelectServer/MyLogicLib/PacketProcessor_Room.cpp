
#include "../MySelectServerNetLib/TcpNetwork.h"

#include "UserManager.h"
#include "User.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "PacketProcessor.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace MyLogicLib
{
	// �� ���� ��û
	// ���� �ڱⰡ ���� ���� ��쵵 ���� �� �ִ�.
	ERROR_CODE PacketProcessor::RoomEnter(PacketInfo packetInfo)
	{
		CHECK_START;
		// �� ���� ��û ��Ŷ
		auto reqPacket = (NCommon::PktRoomEnterReq*)packetInfo.dataAddress;
		NCommon::PktRoomEnterRes resPacket;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		// ����
		auto user = std::get<1>(result);

		// �κ� �ִ� ���� �ƴϸ� ����
		if (user->IsLobbyState() == false)
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);

		// �ش� ������ �����ϴ� ���� �κ� ã�´�.
		auto lobbyIndex = user->GetLobbyIndex();
		auto lobby = _lobbyManager->GetLobby(lobbyIndex);
		// �ش� �κ� ������ ����
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);

		auto room = (Room*)nullptr;

		// ���� ����� ����� ���� �����.
		if (reqPacket->IsCreate)
		{
			room = lobby->CreateRoom();
			if (room == nullptr)
				;//�κ��� �ִ� ��� �� ���� �� á��

			auto result = room->CreateRoom(reqPacket->RoomTitle);
			if (result != ERROR_CODE::NONE)
				CHECK_ERROR(result);
		}
		else // �̹� �ִ� �뿡 ������ ���
		{
			room = lobby->GetRoom(reqPacket->RoomIndex);
			if (room == nullptr)
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}
		// �뿡 ������ �ִ´�.
		room->EnterUser(user);

		// �� ���� ���� �Ϸ�. ���� ���� �ȴ�.
		user->EnterRoom(lobbyIndex, room->GetIndex());

		// �κ� ������ �������� �˸���.
		lobby->NotifyLobbyLeaveUserInfo(user);

		// �κ� �� ������ �ٲ� ���� �˸���
		lobby->NotifyChangedRoomInfo(room->GetIndex());

		// �ش� �濡 �� ������ ���Դٰ� �˸���
		room->NotifyEnterUserInfo(user->GetIndex(), user->GetID());

		// ���� �� ������ ��Ŷ�� ��´�.
		resPacket.RoomInfo.RoomIndex = room->GetIndex();
		wcsncpy_s(resPacket.RoomInfo.RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, room->GetTitle().c_str(), NCommon::MAX_ROOM_TITLE_SIZE);
		resPacket.RoomInfo.RoomUserCount = room->GetUserCount();

		// �������� ������.
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPacket), (char*)&resPacket);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPacket), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcessor::RoomUserList(PacketInfo packetInfo)
	{
		//return ERROR_CODE::NONE; // Ŭ�������� ������ ���� ��� ���Ƶд�.
		CHECK_START;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorcode = std::get<0>(result);
		if (errorcode != ERROR_CODE::NONE)
			CHECK_ERROR(errorcode);
		auto user = std::get<1>(result);
		if (user->IsRoomState() == false)
			CHECK_ERROR(ERROR_CODE::ROOM_USER_LIST_INVALID_DOMAIN);

		// �κ� ã�´�
		auto lobby = _lobbyManager->GetLobby(user->GetLobbyIndex());
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_USER_LIST_INVALID_LOBBY_INDEX);
		
		// ���� ã�´�.
		auto room = lobby->GetRoom(user->GetRoomIndex());
		if (room == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_USER_LIST_INVALID_ROOM_INDEX);

		auto reqPacket = (NCommon::PktRoomUserListReq*)packetInfo.dataAddress;

		room->SendUserList(user->GetSessionIndex());

		return ERROR_CODE::NONE;

	CHECK_ERR:

		NCommon::PktLobbyUserListRes resPacket;
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_USER_LIST_REQ, sizeof(NCommon::PktBase), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	// ä�� �޽����� ������ ��
	ERROR_CODE PacketProcessor::RoomChat(PacketInfo packetInfo)
	{
		CHECK_START;
		auto reqPacket = (NCommon::PktRoomChatReq*)packetInfo.dataAddress;
		NCommon::PktRoomChatRes resPacket;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		// ����
		auto user = std::get<1>(result);
		// ������ �濡 �ִ� ���� �ƴϸ� ����
		if (user->IsRoomState() == false)
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);

		// �ش� ������ �����ϰ� �ִ� �κ� ã�´�
		auto lobbyIndex = user->GetLobbyIndex();
		auto lobby = _lobbyManager->GetLobby(lobbyIndex);
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);

		// �ش� ������ ����ִ� ���� ã�´�
		auto room = lobby->GetRoom(user->GetRoomIndex());
		if (room == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_ROOM_INDEX);

		//�ش� ���� �����鿡�� ���� ���� ä�� �޽����� �����Ѵ�
		room->NotifyChat(packetInfo.sessionIndex, user->GetID(), reqPacket->Msg);
		// ��û�ڿ��� ����� ������.
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPacket), (char*)&resPacket);
		//����
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPacket), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcessor::RoomLeave(PacketInfo packetInfo)
	{
		CHECK_START
			NCommon::PktRoomLeaveRes resPkt;

		auto pUserRet = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		// ����
		auto pUser = std::get<1>(pUserRet);
		auto userIndex = pUser->GetIndex();

		// �ش� ������ �濡 ����ִ� ���°� �ƴϸ� ����
		if (pUser->IsRoomState() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
		}

		// �ش� ������ �����ϰ� �ִ� �κ� ã�´�.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = _lobbyManager->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}

		// �ش� ������ ����ִ� ���� ã�´�.
		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		// �ش� ������ ���� �濡�� ��������.
		auto leaveRet = pRoom->LeaveUser(userIndex);
		if (leaveRet != ERROR_CODE::NONE) {
			CHECK_ERROR(leaveRet);
		}

		// ���� ������ �κ�� ����
		pUser->EnterLobby(lobbyIndex);

		// �뿡 ������ �������� �뺸
		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

		// �κ� ���ο� ������ �������� �뺸
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		// �κ� �ٲ� �� ������ �뺸
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}


}