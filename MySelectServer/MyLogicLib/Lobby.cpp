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

		// ����Ǯ �ʱ�ȭ
		for (int i = 0; i < maxUserPerRoom; ++i)
		{
			LobbyUser lobbyUser;
			lobbyUser.index = i;
			lobbyUser.user = nullptr;

			_userList.push_back(lobbyUser);
		}

		for (int i = 0; i < maxRoomPerLobby; ++i)
		{
			// TODO : �� Ǯ �ʱ�ȭ
			_roomList.emplace_back(Room());
			_roomList[i].Init((short)i, maxUserPerRoom);
			_roomList[i].SetTitle(L"����" + std::to_wstring(i));
		}
	}

	void Lobby::SetNetwork(TcpNet* network, ILogger* logger)
	{
		_network = network;
		_logger = logger;
		
		// TODO : �� �븶�� ��Ʈ��ũ �������ֱ�
	}

	short Lobby::GetUserCount()
	{
		return static_cast<short>(_userIndexDic.size());
	}

	// �κ� ������ ����ִ´�.
	ERROR_CODE Lobby::EnterUser(User* user)
	{

		// �κ� �� á�� ���
		if (_userIndexDic.size() >= _maxUserCount)
			return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;

		// �̹� ���� �ִ� �������� Ȯ��
		if (FindUser(user->GetIndex()) != nullptr)
			return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;

		// ���� ����ֱ�
		auto result = AddUser(user);
		if (result != ERROR_CODE::NONE)
			return result;

		user->EnterLobby(_lobbyIndex);

		// ��ųʸ� ����
		_userIndexDic.insert({ user->GetIndex(),user });
		_userIdDic.insert({ user->GetID(),user });

		return ERROR_CODE::NONE;

	}


	MyLogicLib::User* Lobby::FindUser(int userIndex)
	{
		auto findIter = _userIndexDic.find(userIndex);

		if (findIter == _userIndexDic.end())
			return nullptr;

		return (User*)findIter->second;
	}

	// �κ��� ��������Ʈ�� ������ �ִ´�
	ERROR_CODE Lobby::AddUser(User* user)
	{
		// �� �ڸ��� ã�´�
		auto findIter = std::find_if(std::begin(_userList), std::end(_userList), [](auto& lobbyUser) { return lobbyUser.user == nullptr; });

		// �� �ڸ��� ����
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

	ERROR_CODE Lobby::SendRoomList(const int sessionId, const short startRoomId)
	{
		// ���� �ε����� �߸����
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

			// ������³�����..
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

}