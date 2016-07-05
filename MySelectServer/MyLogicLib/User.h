#pragma once

#include <string>

namespace MyLogicLib
{
	// 유저
	class User
	{
	public:
		enum class USER_STATE
		{
			NONE = 0,
			LOGIN = 1,
			LOBBY = 2,
			ROOM = 3
		};

	public:
		User() {}
		virtual ~User() {}

		void Init(const short index)
		{
			_index = index;
		}

		void Clear()
		{
			_sessionIndex = 0;
			_Id = "";
			_isCertified = false;
			_currentUserState = USER_STATE::NONE;
			_lobbyIndex = -1;
			_RoomIndex = -1;
		}

		void Set(const int sessionIndex, const char* Id)
		{
			// 이게 인증 전부인가?;;
			_isCertified = true;

			_currentUserState = USER_STATE::LOGIN;

			_sessionIndex = sessionIndex;
			_Id = Id;
		}

		short			GetIndex() { return _index; };

		int				GetSessionIndex() { return _sessionIndex; }
		
		std::string&	GetID() { return _Id; }

		// 유저 인증여부
		bool			IsCertified() { return _isCertified; }

		short			GetLobbyIndex() { return _lobbyIndex; }

		void EnterLobby(const short lobbyIndex)
		{
			_lobbyIndex = lobbyIndex;
			_currentUserState = USER_STATE::LOBBY;
		}

		void LeaveLobby()
		{
			_currentUserState = USER_STATE::LOGIN;
		}

		short GetRoomIndex() { return _RoomIndex; }

		void EnterRoom(const short lobbyIndex, const short roomIndex)
		{
			_lobbyIndex = lobbyIndex;
			_RoomIndex = roomIndex;
			_currentUserState = USER_STATE::ROOM;
		}
		bool IsLoginState() {
			return _currentUserState == USER_STATE::LOGIN ? true : false;
		}
		bool IsLobbyState() {
			return _currentUserState == USER_STATE::LOBBY ? true : false;
		}
		bool IsRoomState() {
			return _currentUserState == USER_STATE::ROOM ? true : false;
		}


	private:
		short		_index = -1;
		
		int			_sessionIndex = -1;

		std::string _Id;

		bool		_isCertified = false;

		USER_STATE	_currentUserState = USER_STATE::NONE;

		short		_lobbyIndex = -1;

		short		_RoomIndex = -1;
	};
}