#include "../Common/ErrorCode.h"
#include "../Common/Packet.h"
#include "../MySelectServerNetLib/TcpNetwork.h"
#include "Room.h"

using PACKET_ID = NCommon::PACKET_ID;
namespace MyLogicLib
{
	void Room::Init(const short index, const short maxUserCount)
	{

		_index = index;
		_maxUserCount = maxUserCount;
	}

	void Room::Clear()
	{
		_isUsed = false;
		_title = L"";
		_userList.clear();
	}

	// �� ����
	ERROR_CODE Room::CreateRoom(const std::wstring roomTitle)
	{
		// �̹� �ִ� ���̸� ����
		if (_isUsed)
			return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;

		// ������� ���̶�� üũ
		_isUsed = true;
		_title = roomTitle;

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::EnterUser(User* pUser)
	{
		if (_isUsed == false) {
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		if (_userList.size() == _maxUserCount) {
			return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
		}

		_userList.push_back(pUser);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::LeaveUser(const short userIndex)
	{
		if (_isUsed == false) {
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		// �ش� ���� ã��
		auto iter = std::find_if(std::begin(_userList), std::end(_userList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
		if (iter == std::end(_userList)) {
			return ERROR_CODE::ROOM_LEAVE_NOT_MEMBER;
		}
		// ��������Ʈ���� ����
		_userList.erase(iter);

		// ������ �Ѹ� ���� �ʾ����� �� ����
		if (_userList.empty())
		{
			Clear();
		}

		return ERROR_CODE::NONE;
	}

	void Room::NotifyEnterUserInfo(const short userIndex, const std::string userId)
	{
		NCommon::PktRoomEnterUserInfoNtf packet;
		strncpy_s(packet.UserID, _countof(packet.UserID), userId.c_str(), NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_ENTER_USER_NTF, sizeof(packet), (char*)&packet,userIndex);
	}

	void Room::NotifyLeaveUserInfo(const char* pszUserID)
	{
		if (_isUsed == false) {
			return;
		}

		NCommon::PktRoomLeaveUserInfoNtf pkt;
		// �������� �������� ������ ����.
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt);
	}

	void Room::NotifyChat(const short sessionIndex, std::string userId, std::wstring message)
	{
		NCommon::PktRoomChatNtf packet;
		// id, �޽��� ����
		strncpy_s(packet.UserID, _countof(packet.UserID), userId.c_str(), NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(packet.Msg, NCommon::MAX_ROOM_CHAT_MSG_SIZE + 1, message.c_str(), NCommon::MAX_ROOM_CHAT_MSG_SIZE);
		
		SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(packet), (char*)&packet, sessionIndex);
	}

	void Room::SendToAllUser(const short packetId, const short dataSize, char* dataPos, const int passUserIndex /*= -1*/)
	{
		for (auto user : _userList)
		{
			if(user->GetIndex() == passUserIndex)
				continue;
			_network->SendData(user->GetSessionIndex(), packetId, dataSize, dataPos);
		}
	}

	ERROR_CODE Room::SendUserList(const int sessionId)
	{
		NCommon::PktRoomUserListRes pktRes;

		pktRes.RoomIndex = _index;
		pktRes.UserCount = _userList.size();
		for (int i=0; i<pktRes.UserCount; ++i)
		{
			auto& user = _userList[i];
			strncpy_s(pktRes.UserInfo[i].UserID, NCommon::MAX_USER_ID_SIZE + 1, user->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);
		}

		_network->SendData(sessionId, (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES, sizeof(pktRes), (char*)&pktRes);

		return ERROR_CODE::NONE;
	}

}