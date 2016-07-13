#pragma once

#include <tuple>
#include <unordered_map>
#include <queue>

namespace NCommon
{
	enum class ERROR_CODE : short;
}
using ERROR_CODE = NCommon::ERROR_CODE;
namespace MyLogicLib
{

	class User;

	class UserManager
	{
	public:
		UserManager();
		virtual ~UserManager();

		void									Init(const int maxUserCount);

		ERROR_CODE								AddUser(const int sessionIndex, const char* pszId);
		ERROR_CODE								RemoveUser(const int sessionIndex);

		std::tuple<ERROR_CODE, User*>			GetUser(const int sessionIndex);
		std::tuple<ERROR_CODE, User*>			GetUser(std::string id);

	private:
		User*									AllocUserObjFromPool();
		void									RelaseUserFromPoolByIndex(const int index);

		User*									FindUser(const int sessionIndex);
		User*									FindUser(const char* pszID);

	private:
		std::vector<User>						_userObjectPool;
		std::queue<int>							_availableUserIndexQueue;

		std::unordered_map<int, User*>			_userSessionDic;
		std::unordered_map<const char*, User*>	_userIDDic; // char*는 key로 사용 못함
	};
}