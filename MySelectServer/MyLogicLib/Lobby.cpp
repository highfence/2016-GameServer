#include <algorithm>

#include "../MySelectServerNetLib/ILogger.h"
#include "../MySelectServerNetLib/TcpNetwork.h"
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"

#include "Room.h"
#include "Lobby.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace MyLogicLib
{
	Lobby::Lobby() {}
	Lobby::~Lobby() {}

	void Lobby::Init(const short lobbyIndex, const short maxUserPerLobby, const short maxRoomPerLobby, const short maxUserPerRoom)
	{
		_lobbyIndex = lobbyIndex;
		_maxUserCount = maxUserPerLobby;

		// 유저풀 초기화
		for (int i = 0; i < maxUserPerRoom; ++i)
		{
			LobbyUser lobbyUser;
			lobbyUser.index = i;
			lobbyUser.user = nullptr;

			_userList.push_back(lobbyUser);
		}

		for (int i = 0; i < maxRoomPerLobby; ++i)
		{
			_roomList.emplace_back(Room());
			_roomList[i].Init((short)i, maxUserPerRoom);
		}
	}

	void Lobby::SetNetwork(TcpNet* network, ILogger* logger)
	{
		_network = network;
		_logger = logger;
		
		for (auto& room : _roomList)
			room.SetNetwork(network);
	}

	short Lobby::GetUserCount()
	{
		return static_cast<short>(_userIndexDic.size());
	}

	// 로비에 유저를 집어넣는다.
	ERROR_CODE Lobby::EnterUser(User* user)
	{

		// 로비가 다 찼을 경우
		if (_userIndexDic.size() >= _maxUserCount)
			return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;

		// 이미 들어와 있는 유저인지 확인
		if (FindUser(user->GetIndex()) != nullptr)
			return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;

		// 유저 집어넣기
		auto result = AddUser(user);
		if (result != ERROR_CODE::NONE)
			return result;

		user->EnterLobby(_lobbyIndex);

		// 딕셔너리 갱신
		_userIndexDic.insert({ user->GetIndex(),user });
		_userIdDic.insert({ user->GetID(),user });

		return ERROR_CODE::NONE;

	}


	ERROR_CODE Lobby::LeaveUser(const int userIndex)
	{
		RemoveUser(userIndex);

		auto pUser = FindUser(userIndex);

		if (pUser == nullptr) {
			return ERROR_CODE::LOBBY_LEAVE_USER_NVALID_UNIQUEINDEX;
		}

		pUser->LeaveLobby();

		_userIndexDic.erase(pUser->GetIndex());
		_userIdDic.erase(pUser->GetID().c_str());

		return ERROR_CODE::NONE;
	}

	MyLogicLib::User* Lobby::FindUser(int userIndex)
	{
		auto findIter = _userIndexDic.find(userIndex);

		if (findIter == _userIndexDic.end())
			return nullptr;

		return (User*)findIter->second;
	}

	// 로비의 유저리스트에 유저를 넣는다
	ERROR_CODE Lobby::AddUser(User* user)
	{
		// 빈 자리를 찾는다
		auto findIter = std::find_if(std::begin(_userList), std::end(_userList), [](auto& lobbyUser) { return lobbyUser.user == nullptr; });

		// 빈 자리가 없다
		if (findIter == std::end(_userList))
			return ERROR_CODE::LOBBY_ENTER_EMPTY_USER_LIST;

		findIter->user = user;
		return ERROR_CODE::NONE;
	}

	void Lobby::NotifyLobbyEnterUserInfo(User* user)
	{
		NCommon::PktLobbyNewUserInfoNtf packet;
		strncpy_s(packet.UserID, _countof(packet.UserID), user->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::LOBBY_ENTER_USER_NTF, sizeof(packet), (char*)&packet, user->GetIndex());
	}

	void Lobby::NotifyLobbyLeaveUserInfo(User* user)
	{
		NCommon::PktLobbyLeaveUserInfoNtf packet;
		strncpy_s(packet.UserID, _countof(packet.UserID), user->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::LOBBY_LEAVE_USER_NTF, sizeof(packet), (char*)&packet, user->GetIndex());
	}

	// 방 정보를 갱신해서 유저들에게 보낸다.
	void Lobby::NotifyChangedRoomInfo(const short roomIndex)
	{
		NCommon::PktChangedRoomInfoNtf packet;

		auto& room = _roomList[roomIndex];

		packet.RoomInfo.RoomIndex = room.GetIndex();
		packet.RoomInfo.RoomUserCount = room.GetUserCount();

		if (_roomList[roomIndex].IsUsed())
			wcsncpy_s(packet.RoomInfo.RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, room.GetTitle().c_str(), NCommon::MAX_ROOM_TITLE_SIZE);
		else
			packet.RoomInfo.RoomTitle[0] = L'0';

		SendToAllUser((short)PACKET_ID::ROOM_CHANGED_INFO_NTF, sizeof(packet), (char*)&packet);
	}

	void Lobby::NotifyChat(const short sessionIndex, std::string userId, std::wstring message)
	{
		NCommon::PktLobbyChatNtf packet;
		// id, 메시지 복사
		strncpy_s(packet.UserID, _countof(packet.UserID), userId.c_str(), NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(packet.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, message.c_str(), NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

		SendToAllUser((short)PACKET_ID::LOBBY_CHAT_NTF, sizeof(packet), (char*)&packet, sessionIndex);
	}

	ERROR_CODE Lobby::SendRoomList(const int sessionId, const short startRoomId)
	{
		// 시작 인덱스를 잘못줬다
		if (startRoomId < 0 || startRoomId >= (_roomList.size() - 1)) {
			return ERROR_CODE::LOBBY_ROOM_LIST_INVALID_START_ROOM_INDEX;
		}

		NCommon::PktLobbyRoomListRes pktRes;
		short roomCount = 0;
		int lastCheckedIndex = 0;


		for (int i = startRoomId; i < _roomList.size(); ++i)
		{
			auto& room = _roomList[i];
			lastCheckedIndex = i;

			// 어따쓰는놈이지..
			if (room.IsUsed() == false) {
				continue;
			}

			pktRes.RoomInfo[roomCount].RoomIndex = room.GetIndex();
			pktRes.RoomInfo[roomCount].RoomUserCount = room.GetUserCount();
			wcsncpy_s(pktRes.RoomInfo[roomCount].RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, room.GetTitle().c_str(), NCommon::MAX_ROOM_TITLE_SIZE);

			++roomCount;

			if (roomCount >= NCommon::MAX_NTF_LOBBY_ROOM_LIST_COUNT) {
				break;
			}
		}
		pktRes.Count = roomCount;

		if (roomCount <= 0 || (lastCheckedIndex + 1) == _roomList.size()) {
			pktRes.IsEnd = true;
		}

		_network->SendData(sessionId, (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(pktRes), (char*)&pktRes);

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Lobby::SendUserList(const int sessionId, const short startUserIndex)
	{
		if (startUserIndex < 0 || startUserIndex >= (_userList.size() - 1)) {
			return ERROR_CODE::LOBBY_USER_LIST_INVALID_START_USER_INDEX;
		}

		int lastCheckedIndex = 0;
		NCommon::PktLobbyUserListRes pktRes;
		short userCount = 0;

		for (int i = startUserIndex; i < _userList.size(); ++i)
		{
			auto& lobbyUser = _userList[i];
			lastCheckedIndex = i;

			if (lobbyUser.user == nullptr || lobbyUser.user->IsLobbyState() == false) {
				continue;
			}

			pktRes.UserInfo[userCount].LobbyUserIndex = (short)i;
			strncpy_s(pktRes.UserInfo[userCount].UserID, NCommon::MAX_USER_ID_SIZE + 1, lobbyUser.user->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

			++userCount;

			if (userCount >= NCommon::MAX_SEND_LOBBY_USER_LIST_COUNT) {
				break;
			}
		}

		pktRes.Count = userCount;

		if (userCount <= 0 || (lastCheckedIndex + 1) == _userList.size()) {
			pktRes.IsEnd = true;
		}

		_network->SendData(sessionId, (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(pktRes), (char*)&pktRes);

		return ERROR_CODE::NONE;
	}

	// 새로운 방 생성
	Room* Lobby::CreateRoom()
	{
		for (int i = 0; i < _roomList.size(); ++i)
		{
			if (_roomList[i].IsUsed() == false)
				return &_roomList[i];
		}
		return nullptr;
	}

	Room* Lobby::GetRoom(const short roomIndex)
	{
		// 에러
		if (roomIndex < 0 || roomIndex >= _roomList.size())
			return nullptr;
		
		return &_roomList[roomIndex];
	}

	void Lobby::SendToAllUser(const short packetId, const short dataSize, char* dataPos, const int passUserIndex)
	{
		for (auto& user : _userIndexDic)
		{
			if(user.second->GetIndex() == passUserIndex)
				continue;
			if(user.second->IsLobbyState() == false)
				continue;

			_network->SendData(user.second->GetSessionIndex(), packetId, dataSize, dataPos);
		}
	}

	void Lobby::RemoveUser(const int userIndex)
	{
		auto findIter = std::find_if(std::begin(_userList), std::end(_userList), [userIndex](auto& lobbyUser) { return lobbyUser.user != nullptr && lobbyUser.user->GetIndex() == userIndex; });

		if (findIter != std::end(_userList)) {
			return;
		}

		findIter->user = nullptr;
	}

}