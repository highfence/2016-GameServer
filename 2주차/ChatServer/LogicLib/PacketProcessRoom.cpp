#include "../../Common/Packet.h"
#include "../MySelectServer/MySelectServerNetLib/TcpNetwork.h"
//#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "Room.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{

	// �� ���� ��û
	// ���� �ڱⰡ ���� ���� ��쵵 ���� �� �ִ�.
	ERROR_CODE PacketProcess::RoomEnter(PacketInfo packetInfo)
	{
	CHECK_START
		// �� ���� ��û ��Ŷ.
		// 
		auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.dataAddress;
		NCommon::PktRoomEnterRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		// ����
		auto pUser = std::get<1>(pUserRet);

		// �κ� �ִ� ���� �ƴϸ� ����
		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
		}

		// �ش� ������ �����ϴ� ���� �κ� ã�´�.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}
		
		// ������ ������ �ϴ� ���� Ȯ���Ѵ�.
		auto pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		// ���� ����� ����� ���� �����
		if (reqPkt->IsCreate)
		{
			pRoom = pLobby->CreateRoom();
			if (pRoom == nullptr) {
				//CHECK_ERROR(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
			}

			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
			if (ret != ERROR_CODE::NONE) {
				CHECK_ERROR(ret);
			}
		}
		else
		{
			pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
			if (pRoom == nullptr) {
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
			}
		}
		
		// ���� ������ �뿡 ���Դٰ� �����Ѵ�.
		pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

		// �κ� ������ �������� �˸���
		pLobby->NotifyLobbyLeaveUserInfo(pUser);
		
		// �κ� �� ������ �ٲ� ���� �˸���
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		// �뿡 �� ���� ���Դٰ� �˸���
		pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());
		
		m_pRefNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	// �� ������
	ERROR_CODE PacketProcess::RoomLeave(PacketInfo packetInfo)
	{
	CHECK_START
		NCommon::PktRoomLeaveRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}
		
		// ����
		auto pUser = std::get<1>(pUserRet);
		auto userIndex = pUser->GetIndex();

		// �ش� ������ �濡 ����ִ� ���°� �ƴϸ� ����
		if (pUser->IsCurDomainInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
		}

		// �ش� ������ �����ϰ� �ִ� �κ� ã�´�.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
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
		
		m_pRefNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	// ä�� �޽����� ������ ��
	ERROR_CODE PacketProcess::RoomChat(PacketInfo packetInfo)
	{
	CHECK_START
		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.dataAddress;
		NCommon::PktRoomChatRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		// ����
		auto pUser = std::get<1>(pUserRet);
		
		// ������ �濡 �ִ� ���� �ƴϸ� ����
		if (pUser->IsCurDomainInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
		}

		// �ش� ������ �����ϰ� �ִ� �κ� ã�´�.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
		}

		// �ش� ������ ����ִ� ���� ã�´�.
		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		// �ش� ���� �����鿡�� ���� ���� ä�� �޽����� �����Ѵ�.
		pRoom->NotifyChat(pUser->GetSessioIndex(), pUser->GetID().c_str(), reqPkt->Msg);
				
		m_pRefNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}