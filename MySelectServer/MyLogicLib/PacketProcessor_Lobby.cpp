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

}