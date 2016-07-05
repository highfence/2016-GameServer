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

		// ���� �����Ͱ� PktLogInReq��ŭ���� �����ؾ��Ѵ�.
		// �̸� ����� ũ���� �����Ͱ� �ƴϸ� ��Ŀ�� ���ɼ��� ����.

		auto isBodySizeRight = packetInfo.packetBodySize != sizeof(NCommon::PktLogInReq);
		if (packetInfo.packetBodySize != (short)sizeof(NCommon::PktLogInReq))
			CHECK_ERROR(ERROR_CODE::UNASSIGNED_ERROR);

		// �н������ ������ pass
		// ID �ߺ��̶�� ���� ó���Ѵ�.(AddUser �ȿ���)
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
		// ���� ���� �����ΰ�? - GetUser �ȿ��� ó�����ش�.

		auto result = _userManager->GetUser(packetInfo.sessionIndex);
		auto errorCode = std::get<0>(result);

		if (errorCode != ERROR_CODE::NONE)
			CHECK_ERROR(errorCode);

		auto user = std::get<1>(result);

		// ������ �α��� �������� Ȯ��
		// �̹� �κ� ���ų�, �濡 ���ų� �ϸ� ����
		if (user->IsLoginState() == false)
			CHECK_ERROR(ERROR_CODE::LOBBY_LIST_INVALID_DOMAIN);

		// �κ񸮽�Ʈ�� �����ش�.
		_lobbyManager->SendLobbyListInfo(packetInfo.sessionIndex);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyListRes resPacket;
		resPacket.SetError(__result);
		_network->SendData(packetInfo.sessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(NCommon::PktLogInRes), (char*)&resPacket);
		return (ERROR_CODE)__result;
	}
}