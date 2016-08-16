#include "pch.h"
#include "DbManager.h"

DBManager* DBManager::_instance = nullptr;

DBManager* DBManager::getInstance()
{
	if (_instance == nullptr)
		_instance = new DBManager();
	return _instance;
}

bool DBManager::connectDB(std::wstring dbName, std::wstring id, std::wstring pw)
{
	SQLWCHAR* odbcName = (SQLWCHAR*)dbName.c_str();
	SQLWCHAR* odbcId = (SQLWCHAR*)id.c_str();
	SQLWCHAR* odbcPw = (SQLWCHAR*)pw.c_str();

	if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv) != SQL_SUCCESS)
		return false;
	if (SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER) != SQL_SUCCESS)
		return false;
	if (SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc) != SQL_SUCCESS)
		return false;

	if (SQLConnect(_hDbc, odbcName, SQL_NTS, odbcId, SQL_NTS, odbcPw, SQL_NTS) != SQL_SUCCESS)
		return false;

	if (SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &_hStmt) != SQL_SUCCESS)
		return false;

	return true;
}

void DBManager::disconnectDB()
{
	if (_hStmt)
		SQLFreeHandle(SQL_HANDLE_STMT, _hStmt);
	if (_hDbc)
		SQLDisconnect(_hDbc);
	if (_hDbc)
		SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
	if (_hEnv)
		SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
}
