#include "../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"

namespace MyLogicLib
{

	UserManager::UserManager() {}

	UserManager::~UserManager() {}

	void UserManager::Init(const int maxUserCount)
	{
		for (int i = 0; i < maxUserCount; ++i)
		{
			User user;
			user.Init((short)i);

			_userObjectPool.push_back(user);

			// 가용한 유저풀
			_availableUserIndexQueue.push(i);

		}
	}

	// 유저 리스트에 새로운 유저를 추가한다.
	// 아이디 중복이라면 USER_MGR_ID_DUPLICATION을 반환.
	// 유저수 초과이면 USER_MGR_MAX_USER_COUNT 반환.
	ERROR_CODE UserManager::AddUser(const int sessionIndex, const char* pszId)
	{
		if (FindUser(pszId) != nullptr)
			return ERROR_CODE::USER_MGR_ID_DUPLICATION;

		auto user = AllocUserObjFromPool();
		if (user == nullptr)
			return ERROR_CODE::USER_MGR_MAX_USER_COUNT;

		// 유저 정보 셋팅
		user->Set(sessionIndex, pszId);
		// 딕셔너리에 집어넣는다.
		_userSessionDic.insert({ sessionIndex,user });
		_userIDDic.insert({ pszId,user });

		return ERROR_CODE::NONE;
	}

	// 유저를 삭제
	ERROR_CODE UserManager::RemoveUser(const int sessionIndex)
	{
		auto user = FindUser(sessionIndex);

		// 지워달라며... 없는 유저잖아
		if (user == nullptr)
			return ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION;

		auto index = user->GetIndex();
		auto id = user->GetID();

		_userSessionDic.erase(sessionIndex);
		_userIDDic.erase(id.c_str());
		RelaseUserFromPoolByIndex(index);

		return ERROR_CODE::NONE;
	}

	std::tuple<ERROR_CODE, User*> UserManager::GetUser(const int sessionIndex)
	{
		// 인덱스에서 유저를 찾음
		auto user = FindUser(sessionIndex);

		// 해당 세션의 유저는 없는데?
		if (user == nullptr)
			return{ ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr };

		// 인증 여부 확인
		if (user->IsCertified() == false)
			return{ ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr };

		return{ ERROR_CODE::NONE, user };
	}

	// 유저 풀에서 가용한 오브젝트를 찾아 새로운 유저에게 할당해 준다.
	// 가용 오브젝트가 없으면 nullptr을 반환.
	MyLogicLib::User* UserManager::AllocUserObjFromPool()
	{
		// 가용 인덱스가 없다 == 풀이 다 찼다.
		if (_availableUserIndexQueue.empty())
			return nullptr;

		// 하나 떼어서 할당
		int index = _availableUserIndexQueue.front();
		_availableUserIndexQueue.pop();

		// operator[]를 쓰면 디버깅이 힘들어서 at을 사용했다.
		return &_userObjectPool.at(index);
	}

	void UserManager::RelaseUserFromPoolByIndex(const int index)
	{
		// 이자리는 이제 다시 써도 된다.
		_availableUserIndexQueue.push(index);
		// 다시 쓸 수 있게 깨끗하게 초기화
		_userObjectPool.at(index).Clear();
	}

	MyLogicLib::User* UserManager::FindUser(const int sessionIndex)
	{
		auto findIter = _userSessionDic.find(sessionIndex);
		if (findIter == _userSessionDic.end())
			return nullptr;

		return (User*)findIter->second;
	}

	MyLogicLib::User* UserManager::FindUser(const char* pszID)
	{
		auto findIter = _userIDDic.find(pszID);

		if (findIter == _userIDDic.end()) {
			return nullptr;
		}

		return (User*)findIter->second;
	}

}