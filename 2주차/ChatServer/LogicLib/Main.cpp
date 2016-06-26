#include <thread>
#include <chrono>

#include "../MySelectServer/MySelectServerNetLib/ServerNetErrorCode.h"
//#include "../ServerNetLib/ServerNetErrorCode.h"
#include "../MySelectServer/MySelectServerNetLib/Define.h"
//#include "../ServerNetLib/Define.h"
#include "../MySelectServer/MySelectServerNetLib/TcpNetwork.h"
//#include "../ServerNetLib/TcpNetwork.h"
#include "ConsoleLogger.h"
#include "LobbyManager.h"
#include "PacketProcess.h"
#include "UserManager.h"
#include "Main.h"

using LOG_TYPE = MySelectServerNetLib::LOG_TYPE;
using NET_ERROR_CODE = MySelectServerNetLib::NET_ERROR_CODE;

namespace NLogicLib
{
	Main::Main()
	{
	}

	Main::~Main()
	{
		Release();
	}

	ERROR_CODE Main::Init()
	{
		m_pLogger = std::make_unique<ConsoleLog>();

		LoadConfig();

		m_pNetwork = std::make_unique<MySelectServerNetLib::TcpNetwork>();
		auto result = m_pNetwork->Init(m_pServerConfig.get(), m_pLogger.get());
			
		if (result != NET_ERROR_CODE::NONE)
		{
			m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | Init Fail. NetErrorCode(%s)", __FUNCTION__, (short)result);
			return ERROR_CODE::MAIN_INIT_NETWORK_INIT_FAIL;
		}

		
		m_pUserMgr = std::make_unique<UserManager>();
		m_pUserMgr->Init(m_pServerConfig->maxClientCount);

		m_pLobbyMgr = std::make_unique<LobbyManager>();
		m_pLobbyMgr->Init({ m_pServerConfig->maxLobbyCount, 
							m_pServerConfig->maxLobbyUserCount,
							m_pServerConfig->maxRoomCountByLobby, 
							m_pServerConfig->maxRoomUserCount },
						m_pNetwork.get(), m_pLogger.get());

		m_pPacketProc = std::make_unique<PacketProcess>();
		m_pPacketProc->Init(m_pNetwork.get(), m_pUserMgr.get(), m_pLobbyMgr.get(), m_pLogger.get());

		m_IsRun = true;

		m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Init Success. Server Run", __FUNCTION__);
		return ERROR_CODE::NONE;
	}

	void Main::Release() { }

	void Main::Stop()
	{
		m_IsRun = false;
	}

	void Main::Run()
	{
		while (m_IsRun)
		{
			m_pNetwork->Run();

			while (true)
			{
				auto packetInfo = m_pNetwork->GetPacketInfo();

				if (packetInfo.packetId == 0)
				{
					break;
				}

				m_pPacketProc->Process(packetInfo);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(0));
		}
	}

	ERROR_CODE Main::LoadConfig()
	{
		m_pServerConfig = std::make_unique<MySelectServerNetLib::ServerConfig>();
		
		wchar_t sPath[MAX_PATH] = { 0, };
		::GetCurrentDirectory(MAX_PATH, sPath);

		wchar_t inipath[MAX_PATH] = { 0, };
		_snwprintf_s(inipath, _countof(inipath), _TRUNCATE, L"%s\\ServerConfig.ini", sPath);

		m_pServerConfig->port = (unsigned short)GetPrivateProfileInt(L"Config", L"port", 0, inipath);
		m_pServerConfig->backLogCount = GetPrivateProfileInt(L"Config", L"backLogCount", 0, inipath);
		m_pServerConfig->maxClientCount = GetPrivateProfileInt(L"Config", L"maxClientCount", 0, inipath);

		m_pServerConfig->maxClientSockOptRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"maxClientSockOptRecvBufferSize", 0, inipath);
		m_pServerConfig->maxClientSockOptSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"maxClientSockOptSendBufferSize", 0, inipath);
		m_pServerConfig->maxClientRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"maxClientRecvBufferSize", 0, inipath);
		m_pServerConfig->maxClientSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"maxClientSendBufferSize", 0, inipath);

		m_pServerConfig->extraClientCount = GetPrivateProfileInt(L"Config", L"extraClientCount", 0, inipath);
		m_pServerConfig->maxLobbyCount = GetPrivateProfileInt(L"Config", L"maxLobbyCount", 0, inipath);
		m_pServerConfig->maxLobbyUserCount = GetPrivateProfileInt(L"Config", L"maxLobbyUserCount", 0, inipath);
		m_pServerConfig->maxRoomCountByLobby = GetPrivateProfileInt(L"Config", L"maxRoomCountByLobby", 0, inipath);
		m_pServerConfig->maxRoomUserCount = GetPrivateProfileInt(L"Config", L"maxRoomUserCount", 0, inipath);

		m_pLogger->Write(MySelectServerNetLib::LOG_TYPE::L_INFO, "%s | port(%d), Backlog(%d)", __FUNCTION__, m_pServerConfig->port, m_pServerConfig->backLogCount);
		return ERROR_CODE::NONE;
	}
		
}