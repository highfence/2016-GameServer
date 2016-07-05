#pragma once
#include <memory>

#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace MySelectServerNetLib
{
	struct ServerConfig;
	class ILogger;
	class TcpNetwork;
}

namespace MyLogicLib
{
	class UserManager;
	class LobbyManager;
	class PacketProcessor;

	class MainLogic
	{
	public:
		MainLogic();
		~MainLogic();

		ERROR_CODE	Init();

		void		Run();
		
		void		Stop();

	private:
		ERROR_CODE	LoadConfig();
		void		Release();

	private:
		bool		_isRunning = false;

		std::unique_ptr<MySelectServerNetLib::ServerConfig> _serverConfig;
		std::unique_ptr<MySelectServerNetLib::ILogger>		_logger;

		std::unique_ptr<MySelectServerNetLib::TcpNetwork>	_netWork;
		std::unique_ptr<PacketProcessor>					_packetProcessor;
		std::unique_ptr<UserManager>						_userManager;
		std::unique_ptr<LobbyManager>						_lobbyManager;

	};
}