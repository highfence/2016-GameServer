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

			// ������ ����Ǯ
			_availableUserIndexQueue.push(i);

		}
	}

	// ���� ����Ʈ�� ���ο� ������ �߰��Ѵ�.
	// ���̵� �ߺ��̶�� USER_MGR_ID_DUPLICATION�� ��ȯ.
	// ������ �ʰ��̸� USER_MGR_MAX_USER_COUNT ��ȯ.
	ERROR_CODE UserManager::AddUser(const int sessionIndex, const char* pszId)
	{
		if (FindUser(pszId) != nullptr)
			return ERROR_CODE::USER_MGR_ID_DUPLICATION;

		auto user = AllocUserObjFromPool();
		if (user == nullptr)
			return ERROR_CODE::USER_MGR_MAX_USER_COUNT;

		// ���� ���� ����
		user->Set(sessionIndex, pszId);
		// ��ųʸ��� ����ִ´�.
		_userSessionDic.insert({ sessionIndex,user });
		_userIDDic.insert({ pszId,user });

		return ERROR_CODE::NONE;
	}

	// ������ ����
	ERROR_CODE UserManager::RemoveUser(const int sessionIndex)
	{
		auto user = FindUser(sessionIndex);

		// �����޶��... ���� �����ݾ�
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
		// �ε������� ������ ã��
		auto user = FindUser(sessionIndex);

		// �ش� ������ ������ ���µ�?
		if (user == nullptr)
			return{ ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr };

		// ���� ���� Ȯ��
		if (user->IsCertified() == false)
			return{ ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr };

		return{ ERROR_CODE::NONE, user };
	}

	// ���� Ǯ���� ������ ������Ʈ�� ã�� ���ο� �������� �Ҵ��� �ش�.
	// ���� ������Ʈ�� ������ nullptr�� ��ȯ.
	MyLogicLib::User* UserManager::AllocUserObjFromPool()
	{
		// ���� �ε����� ���� == Ǯ�� �� á��.
		if (_availableUserIndexQueue.empty())
			return nullptr;

		// �ϳ� ��� �Ҵ�
		int index = _availableUserIndexQueue.front();
		_availableUserIndexQueue.pop();

		// operator[]�� ���� ������� ���� at�� ����ߴ�.
		return &_userObjectPool.at(index);
	}

	void UserManager::RelaseUserFromPoolByIndex(const int index)
	{
		// ���ڸ��� ���� �ٽ� �ᵵ �ȴ�.
		_availableUserIndexQueue.push(index);
		// �ٽ� �� �� �ְ� �����ϰ� �ʱ�ȭ
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