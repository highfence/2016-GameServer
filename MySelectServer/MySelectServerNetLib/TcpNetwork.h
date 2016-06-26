#pragma once

#define FD_SETSIZE 1024

#pragma comment(lib,"ws2_32")
#include <winSock2.h>
#include <WS2tcpip.h>

#include "Define.h"
#include "ServerNetErrorCode.h"
#include "ILogger.h"

#include <vector>
#include <queue>

namespace MySelectServerNetLib
{
	class TcpNetwork
	{
	public:
		TcpNetwork();
		~TcpNetwork();

		NET_ERROR_CODE	Init(const ServerConfig* config, ILogger* logger);
		NET_ERROR_CODE	SendData(const int sessionIndex, const short packetID, const short size, const char* message);
		void			Run();
		RecvPacketInfo	GetPacketInfo();

	private:
		NET_ERROR_CODE	InitServerSocket();
		NET_ERROR_CODE	BindAndListen(const short port, int backLogCount);

		void			CreateSessionPool(const int maxClientCount);
		NET_ERROR_CODE	NewSession();
		void			AllocSessionToClient(const int sessionIndex, const SOCKET fd, const char* ip);
		int				AllocIndexToClient();
		void			CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET socket, const int sessionIndex);


		void			Run_ProcessClients(fd_set& excSet, fd_set& readSet, fd_set& writeSet);

		bool			Run_ReceiveProcess(const ClientSession& session, fd_set& readSet);
		NET_ERROR_CODE	ReceiveSocket(const int sessionIndex);
		NET_ERROR_CODE	ProcessPacketFromReceiveBuffer(const int sessionIndex);
		void			AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos);

		void			Run_WriteProcess(const ClientSession& session, fd_set& writeSet);
		NetError		FlushSendBuffer(const int sessionIndex);
		NetError		SendSocket(const SOCKET socket, const char* message, const int size);

		void			SetSocketOption(const SOCKET socket);

	private:
		ServerConfig	_config;
		SOCKET			_serverSocket;

		fd_set			_fdSet;
		unsigned		_connectedSessionCount = 0;
		long long		_connectedSeq = 0;

		std::vector<ClientSession>	_clientSessionPool;
		std::queue<int>				_clientSessionPoolIndex;

		std::queue<RecvPacketInfo>	_packetQueue;

		ILogger*			_logger;
	};
}