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
using NET_ERROR_CODE = MySelectServerNetLib::NET_ERROR_CODE;

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

	ERROR_CODE PacketProcessor::LobbyUserList(PacketInfo packetInfo)
	{
		CHECK_START;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorcode = std::get<0>(result);
		if (errorcode != ERROR_CODE::NONE)
			CHECK_ERROR(errorcode);
		auto user = std::get<1>(result);
		if (user->IsLobbyState() == false)
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);

		auto lobby = _lobbyManager->GetLobby(user->GetLobbyIndex());
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_LOBBY_INDEX);

		auto reqPacket = (NCommon::PktLobbyUserListReq*)packetInfo.dataAddress;

		lobby->SendUserList(user->GetSessionIndex(), reqPacket->StartUserIndex);

		return ERROR_CODE::NONE;

	CHECK_ERR:

		NCommon::PktLobbyUserListRes resPacket;
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPacket);
		return (ERROR_CODE)__result;

	}

	// ä�� �޽����� ������ ��
	ERROR_CODE PacketProcessor::LobbyChat(PacketInfo packetInfo)
	{
		CHECK_START;
		auto reqPacket = (NCommon::PktLobbyChatReq*)packetInfo.dataAddress;
		NCommon::PktLobbyChatRes resPacket;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		// ����
		auto user = std::get<1>(result);
		// ������ �κ� �ִ� ���� �ƴϸ� ����
		if (user->IsLobbyState() == false)
			CHECK_ERROR(ERROR_CODE::LOBBY_CHAT_INVALID_DOMAIN);

		// �ش� ������ �����ϰ� �ִ� �κ� ã�´�
		auto lobbyIndex = user->GetLobbyIndex();
		auto lobby = _lobbyManager->GetLobby(lobbyIndex);
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::LOBBY_CHAT_INVALID_LOBBY_INDEX);


		//�ش� ���� �����鿡�� ���� ���� ä�� �޽����� �����Ѵ�
		lobby->NotifyChat(packetInfo.sessionIndex, user->GetID(), reqPacket->Msg);
		// ��û�ڿ��� ����� ������.
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_CHAT_RES, sizeof(resPacket), (char*)&resPacket);
		//����
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_CHAT_RES, sizeof(resPacket), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcessor::Whisper(PacketInfo packetInfo)
	{
		CHECK_START;
		auto reqPacket = (NCommon::PktWhisperReq*)packetInfo.dataAddress;
		NCommon::PktWhisperRes resPacket;
		NCommon::PktWhisperNtf whisperPacket;

		// ���� ����
		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);
		auto sendUser = std::get<1>(result);

		// ���� ������ �˾Ƴ���
		result = _userManager->GetUser(reqPacket->UserID);
		errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);
		auto targetUser = std::get<1>(result);

		// �ӼӸ� ��Ŷ
		// ���� ���� ���̵�, �޽��� ����
		strncpy_s(whisperPacket.UserID, _countof(whisperPacket.UserID),sendUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(whisperPacket.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, reqPacket->Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

		// �ӼӸ��� ������.
		_network->SendData(targetUser->GetSessionIndex(), (short)PACKET_ID::WHISPER_NTF, sizeof(whisperPacket), (char*)&whisperPacket);
		// TODO : ���� ó��

		// ���� ��Ŷ
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::WHISPER_RES, sizeof(resPacket), (char*)&resPacket);
		return ERROR_CODE::NONE;


	CHECK_ERR:
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_CHAT_RES, sizeof(resPacket), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcessor::LobbyLeave(PacketInfo packetInfo)
	{
		CHECK_START
			// ���� �κ� �ִ��� �����Ѵ�.
			// �κ񿡼� ������
			// ���� �κ� �ִ� ������� ������ ����� �ִٰ� �˷��ش�.
			NCommon::PktLobbyLeaveRes resPkt;

		auto pUserRet = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsLobbyState() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_DOMAIN);
		}

		auto pLobby = _lobbyManager->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_LOBBY_INDEX);
		}

		auto enterRet = pLobby->LeaveUser(pUser->GetIndex());
		if (enterRet != ERROR_CODE::NONE) {
			CHECK_ERROR(enterRet);
		}

		pLobby->NotifyLobbyLeaveUserInfo(pUser);

		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(NCommon::PktLobbyLeaveRes), (char*)&resPkt);

		return ERROR_CODE::NONE;
	CHECK_ERR:
		resPkt.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(NCommon::PktLobbyLeaveRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}


}