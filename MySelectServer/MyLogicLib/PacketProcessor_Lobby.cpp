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
		// 현재 위치는 로그인이 맞나?
		// 로비에 들어간다.
		// 기존 로비에 있는 사람에게 새 사람이 들어왔다고 알려준다.

		// 받은 패킷 바디(몇번 로비로 들어가고 싶은지)
		auto reqPacket = (NCommon::PktLobbyEnterReq*)packetInfo.dataAddress;

		// 응답 패킷
		NCommon::PktLobbyEnterRes resPacket;

		// 에러하고 유저를 받아옴
		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		auto user = std::get<1>(result);

		// 현재 로그인 ㅅ ㅏㅇ태인지 체크
		if (user->IsLoginState() == false)
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN);

		// 가고 ㅅ ㅣㅍ은 로비를 받아온다
		auto lobby = _lobbyManager->GetLobby(reqPacket->LobbyId);
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX);

		// 로비에 유저를 집어넣는다.
		auto enterResult = lobby->EnterUser(user);

		// 모든 유저한테 신참 유저가 들어왔다고 알린다.
		lobby->NotifyLobbyEnterUserInfo(user);

		// 응답 패킷에 정보를 담아 넣는다.
		resPacket.MaxRoomCount = lobby->MaxRoomCount();
		resPacket.MaxUserCount = lobby->MaxUserCount();
		
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPacket);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	// 룸 리스트를 요청받았을 때 처리하는 함수
	ERROR_CODE PacketProcessor::LobbyRoomList(PacketInfo packetInfo)
	{
		CHECK_START;
		// 현재 로비에 있는지 조사한다.
		// 룸 리스트를 보내준다.

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

	// 채팅 메시지가 들어왔을 때
	ERROR_CODE PacketProcessor::LobbyChat(PacketInfo packetInfo)
	{
		CHECK_START;
		auto reqPacket = (NCommon::PktLobbyChatReq*)packetInfo.dataAddress;
		NCommon::PktLobbyChatRes resPacket;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		// 유저
		auto user = std::get<1>(result);
		// 유저가 로비에 있는 놈이 아니면 에러
		if (user->IsLobbyState() == false)
			CHECK_ERROR(ERROR_CODE::LOBBY_CHAT_INVALID_DOMAIN);

		// 해당 유저를 관리하고 있는 로비를 찾는다
		auto lobbyIndex = user->GetLobbyIndex();
		auto lobby = _lobbyManager->GetLobby(lobbyIndex);
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::LOBBY_CHAT_INVALID_LOBBY_INDEX);


		//해당 방의 유저들에게 새로 들어온 채팅 메시지를 전달한다
		lobby->NotifyChat(packetInfo.sessionIndex, user->GetID(), reqPacket->Msg);
		// 요청자에게 결과를 보낸다.
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_CHAT_RES, sizeof(resPacket), (char*)&resPacket);
		//리턴
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

		// 보낸 유저
		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);
		auto sendUser = std::get<1>(result);

		// 받을 유저를 알아낸다
		result = _userManager->GetUser(reqPacket->UserID);
		errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);
		auto targetUser = std::get<1>(result);

		// 귓속말 패킷
		// 보낸 놈의 아이디, 메시지 복사
		strncpy_s(whisperPacket.UserID, _countof(whisperPacket.UserID),sendUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(whisperPacket.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, reqPacket->Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

		// 귓속말을 보낸다.
		_network->SendData(targetUser->GetSessionIndex(), (short)PACKET_ID::WHISPER_NTF, sizeof(whisperPacket), (char*)&whisperPacket);
		// TODO : 에러 처리

		// 응답 패킷
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
			// 현재 로비에 있는지 조사한다.
			// 로비에서 나간다
			// 기존 로비에 있는 사람에게 나가는 사람이 있다고 알려준다.
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