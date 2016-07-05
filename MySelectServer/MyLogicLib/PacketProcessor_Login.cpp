#include "../Common/Packet.h"
#include "../MySelectServerNetLib/TcpNetwork.h"
#include "../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "PacketProcessor.h"
#include "LobbyManager.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace MyLogicLib
{
	ERROR_CODE PacketProcessor::Login(PacketInfo packetInfo)
	{
		CHECK_START;

		NCommon::PktLogInRes resPacket;
		auto reqPacket = (NCommon::PktLogInReq*)packetInfo.dataAddress;

		// 받은 데이터가 PktLogInReq만큼인지 조사해야한다.
		// 미리 약속한 크기의 데이터가 아니면 해커일 가능성이 높음.

		auto isBodySizeRight = packetInfo.packetBodySize != sizeof(NCommon::PktLogInReq);
		if (packetInfo.packetBodySize != (short)sizeof(NCommon::PktLogInReq))
			CHECK_ERROR(ERROR_CODE::UNASSIGNED_ERROR);

		// 패스워드는 무조건 pass
		// ID 중복이라면 에러 처리한다.(AddUser 안에서)
		auto result = _userManager->AddUser(packetInfo.sessionIndex, reqPacket->szID);
		if (result != ERROR_CODE::NONE)
			CHECK_ERROR(result);

		resPacket.ErrorCode = (short)result;
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPacket);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcessor::LobbyList(PacketInfo packetInfo)
	{
		CHECK_START;
		// 인증 받은 유저인가? - GetUser 안에서 처리해준다.

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);

		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		auto user = std::get<1>(result);

		// 유저가 로그인 상태인지 확인
		// 이미 로비에 들어갔거나, 방에 들어갔거나 하면 에러
		if (user->IsLoginState() == false)
			CHECK_ERROR(ERROR_CODE::LOBBY_LIST_INVALID_DOMAIN);

		// 로비리스트를 보내준다.
		_lobbyManager->SendLobbyListInfo(packetInfo.sessionIndex);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyListRes resPacket;
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(NCommon::PktLogInRes), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}
}