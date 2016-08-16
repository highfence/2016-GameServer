#pragma once

#include "sql.h"
#include "sqlext.h"

class DBManager
{
public:
	static DBManager* getInstance();

	bool connectDB(std::wstring dbName, std::wstring id, std::wstring pw);
	void disconnectDB();

	SQLHENV getHEnv() { return _hEnv; }
	SQLHDBC getHDbc() { return _hDbc; }
	SQLHSTMT getHStmt() { return _hStmt; }
private:
public:
private:
	static DBManager* _instance;

	SQLHENV _hEnv;
	SQLHDBC _hDbc;
	SQLHSTMT _hStmt;
};