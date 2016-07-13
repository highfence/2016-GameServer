
#include "../MySelectServerNetLib/TcpNetwork.h"

#include "UserManager.h"
#include "User.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "PacketProcessor.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace MyLogicLib
{
	// 방 들어가기 요청
	// 방을 자기가 만들어서 들어가는 경우도 있을 수 있다.
	ERROR_CODE PacketProcessor::RoomEnter(PacketInfo packetInfo)
	{
		CHECK_START;
		// 방 들어가기 요청 패킷
		auto reqPacket = (NCommon::PktRoomEnterReq*)packetInfo.dataAddress;
		NCommon::PktRoomEnterRes resPacket;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		// 유저
		auto user = std::get<1>(result);

		// 로비에 있는 놈이 아니면 에러
		if (user->IsLobbyState() == false)
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);

		// 해당 유저를 관리하는 중인 로비를 찾는다.
		auto lobbyIndex = user->GetLobbyIndex();
		auto lobby = _lobbyManager->GetLobby(lobbyIndex);
		// 해당 로비가 없으면 에러
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);

		auto room = (Room*)nullptr;

		// 룸을 만드는 경우라면 룸을 만든다.
		if (reqPacket->IsCreate)
		{
			room = lobby->CreateRoom();
			if (room == nullptr)
				;//로비의 최대 허용 룸 수가 꽉 찼다

			auto result = room->CreateRoom(reqPacket->RoomTitle);
			if (result != ERROR_CODE::NONE)
				CHECK_ERROR(result);
		}
		else // 이미 있던 룸에 들어가려는 경우
		{
			room = lobby->GetRoom(reqPacket->RoomIndex);
			if (room == nullptr)
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}
		// 룸에 유저를 넣는다.
		room->EnterUser(user);

		// 방 정보 셋팅 완료. 이제 들어가면 된다.
		user->EnterRoom(lobbyIndex, room->GetIndex());

		// 로비에 유저가 나갔음을 알린다.
		lobby->NotifyLobbyLeaveUserInfo(user);

		// 로비에 방 정보가 바뀐 것을 알린다
		lobby->NotifyChangedRoomInfo(room->GetIndex());

		// 해당 방에 새 유저가 들어왔다고 알린다
		room->NotifyEnterUserInfo(user->GetIndex(), user->GetID());

		// 만든 룸 정보를 패킷에 담는다.
		resPacket.RoomInfo.RoomIndex = room->GetIndex();
		wcsncpy_s(resPacket.RoomInfo.RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, room->GetTitle().c_str(), NCommon::MAX_ROOM_TITLE_SIZE);
		resPacket.RoomInfo.RoomUserCount = room->GetUserCount();

		// 리스폰스 보낸다.
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPacket), (char*)&resPacket);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPacket), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcessor::RoomUserList(PacketInfo packetInfo)
	{
		//return ERROR_CODE::NONE; // 클라측에서 에러가 나서 잠시 막아둔다.
		CHECK_START;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorcode = std::get<0>(result);
		if (errorcode != ERROR_CODE::NONE)
			CHECK_ERROR(errorcode);
		auto user = std::get<1>(result);
		if (user->IsRoomState() == false)
			CHECK_ERROR(ERROR_CODE::ROOM_USER_LIST_INVALID_DOMAIN);

		// 로비를 찾는다
		auto lobby = _lobbyManager->GetLobby(user->GetLobbyIndex());
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_USER_LIST_INVALID_LOBBY_INDEX);
		
		// 방을 찾는다.
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

	// 채팅 메시지가 들어왔을 때
	ERROR_CODE PacketProcessor::RoomChat(PacketInfo packetInfo)
	{
		CHECK_START;
		auto reqPacket = (NCommon::PktRoomChatReq*)packetInfo.dataAddress;
		NCommon::PktRoomChatRes resPacket;

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);
		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		// 유저
		auto user = std::get<1>(result);
		// 유저가 방에 있는 놈이 아니면 에러
		if (user->IsRoomState() == false)
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);

		// 해당 유저를 관리하고 있는 로비를 찾는다
		auto lobbyIndex = user->GetLobbyIndex();
		auto lobby = _lobbyManager->GetLobby(lobbyIndex);
		if (lobby == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);

		// 해당 유저가 들어있는 방을 찾는다
		auto room = lobby->GetRoom(user->GetRoomIndex());
		if (room == nullptr)
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_ROOM_INDEX);

		//해당 방의 유저들에게 새로 들어온 채팅 메시지를 전달한다
		room->NotifyChat(packetInfo.sessionIndex, user->GetID(), reqPacket->Msg);
		// 요청자에게 결과를 보낸다.
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPacket), (char*)&resPacket);
		//리턴
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

		// 유저
		auto pUser = std::get<1>(pUserRet);
		auto userIndex = pUser->GetIndex();

		// 해당 유저가 방에 들어있는 상태가 아니면 에러
		if (pUser->IsRoomState() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
		}

		// 해당 유저를 관리하고 있는 로비를 찾는다.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = _lobbyManager->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}

		// 해당 유저가 들어있는 방을 찾는다.
		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		// 해당 유저가 속한 방에서 내보낸다.
		auto leaveRet = pRoom->LeaveUser(userIndex);
		if (leaveRet != ERROR_CODE::NONE) {
			CHECK_ERROR(leaveRet);
		}

		// 유저 정보를 로비로 변경
		pUser->EnterLobby(lobbyIndex);

		// 룸에 유저가 나갔음을 통보
		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

		// 로비에 새로운 유저가 들어왔음을 통보
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		// 로비에 바뀐 방 정보를 통보
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}


}