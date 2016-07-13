#include <thread>
#include <chrono>

#include "../MySelectServerNetLib/ServerNetErrorCode.h"
#include "../MySelectServerNetLib/Define.h"
#include "../MySelectServerNetLib/TcpNetwork.h"

#include "UserManager.h"
#include "MainLogic.h"
#include "ConsoleLogger.h"
#include "PacketProcessor.h"
#include "LobbyManager.h"


using LOG_TYPE = MySelectServerNetLib::LOG_TYPE;
using NET_ERROR_CODE = MySelectServerNetLib::NET_ERROR_CODE;

namespace MyLogicLib
{
	MainLogic::MainLogic()
	{

	}
	MainLogic::~MainLogic()
	{
		Release();
	}

	ERROR_CODE MainLogic::Init()
	{
		_logger = std::make_unique<ConsoleLogger>();

		// ini에서 설정 불러오기
		LoadConfig();

		// 네트워크 라이브러리 초기화
		_netWork = std::make_unique<MySelectServerNetLib::TcpNetwork>();
		auto result = _netWork->Init(_serverConfig.get(), _logger.get());
		if (result != NET_ERROR_CODE::NONE)
		{
			_logger->Write(LOG_TYPE::L_ERROR, "%s | Init Fail. NetErrorCode(%s)", __FUNCTION__, (short)result);
			return ERROR_CODE::MAIN_INIT_NETWORK_INIT_FAIL;
		}

		// 유저 매니저
		_userManager = std::make_unique<UserManager>();
		_userManager->Init(_serverConfig->maxClientCount);

		// 로비 매니저
		_lobbyManager = std::make_unique<LobbyManager>();
		LobbyManagerConfig lobbySetting;
		lobbySetting.maxLobbyCount = _serverConfig->maxLobbyCount;
		lobbySetting.maxRoomPerLobby = _serverConfig->maxRoomPerLobby;
		lobbySetting.maxUserPerLobby = _serverConfig->maxUserPerLobby;
		lobbySetting.maxUserPerRoom = _serverConfig->maxUserPerRoom;
		_lobbyManager->Init(lobbySetting, _netWork.get(), _logger.get());

		// 패킷 프로세서
		_packetProcessor = std::make_unique<PacketProcessor>();
		_packetProcessor->Init(_netWork.get(), _userManager.get(), _lobbyManager.get(), _logger.get());


		_isRunning = true;

		// 초기화 성공 로그
		_logger->Write(LOG_TYPE::L_INFO, "%s | Init Success. Server Run", __FUNCTION__);

		return ERROR_CODE::NONE;
	}


	void MainLogic::Run()
	{
		while (_isRunning)
		{
			// 읽을 패킷, 보낼 패킷 쭉 처리
			_netWork->Run();

			while (true)
			{
				auto packetInfo = _netWork->GetPacketInfo();

				if(packetInfo.packetId == 0)
					break;

				_packetProcessor->Process(packetInfo);
			}

			// 나 혼자 cpu를 독점하지 않겠다.
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
		}
	}

	void MainLogic::Stop()
	{
		_isRunning = false;
	}

	// ini파일에서 셋팅 로드
	ERROR_CODE MainLogic::LoadConfig()
	{
		_serverConfig = std::make_unique<MySelectServerNetLib::ServerConfig>();

		wchar_t sPath[MAX_PATH] = { 0, };
		::GetCurrentDirectoryW(MAX_PATH, sPath);

		wchar_t iniPath[MAX_PATH] = { 0, };
		_snwprintf_s(iniPath, _countof(iniPath), _TRUNCATE, L"%s\\ServerConfig.ini", sPath);

		_serverConfig->port = (unsigned short)GetPrivateProfileInt(L"Config", L"port", 0, iniPath);
		_serverConfig->backLogCount = GetPrivateProfileInt(L"Config", L"backLogCount", 0, iniPath);
		_serverConfig->maxClientCount = GetPrivateProfileInt(L"Config", L"maxClientCount", 0, iniPath);

		_serverConfig->maxClientSockOptRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"maxClientSockOptRecvBufferSize", 0, iniPath);
		_serverConfig->maxClientSockOptSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"maxClientSockOptSendBufferSize", 0, iniPath);
		_serverConfig->maxClientRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"maxClientRecvBufferSize", 0, iniPath);
		_serverConfig->maxClientSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"maxClientSendBufferSize", 0, iniPath);

		_serverConfig->extraClientCount = GetPrivateProfileInt(L"Config", L"extraClientCount", 0, iniPath);
		_serverConfig->maxLobbyCount = GetPrivateProfileInt(L"Config", L"maxLobbyCount", 0, iniPath);
		_serverConfig->maxUserPerLobby = GetPrivateProfileInt(L"Config", L"maxLobbyUserCount", 0, iniPath);
		_serverConfig->maxRoomPerLobby = GetPrivateProfileInt(L"Config", L"maxRoomCountByLobby", 0, iniPath);
		_serverConfig->maxUserPerRoom = GetPrivateProfileInt(L"Config", L"maxRoomUserCount", 0, iniPath);

		_logger->Write(MySelectServerNetLib::LOG_TYPE::L_INFO, "%s | port(%d), Backlog(%d)", __FUNCTION__, _serverConfig->port, _serverConfig->backLogCount);
		return ERROR_CODE::NONE;
	}

	void MainLogic::Release()
	{

	}

}