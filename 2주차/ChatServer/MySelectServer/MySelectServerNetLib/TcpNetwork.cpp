#include "TcpNetwork.h"


namespace MySelectServerNetLib
{
	TcpNetwork::TcpNetwork()
	{
	}


	TcpNetwork::~TcpNetwork()
	{
	}

	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* config, ILogger* logger)
	{
		// ���� ���� �ε�
		memcpy(&_config,config,sizeof(_config));

		// �ΰ� ����
		_logger = logger;

		// ���� ���� ���� �� �ʱ�ȭ
		auto result = InitServerSocket();
		if (result != NET_ERROR_CODE::NONE)
			return result;

		// bind�ϰ� listen�Ѵ�.
		result = BindAndListen(_config.port, _config.backLogCount);
		if (result != NET_ERROR_CODE::NONE)
			return result;

		// accept�ϴ� ������ �� ���̹Ƿ� readFD�� �������ش�.
		FD_ZERO(&_fdSet);
		FD_SET(_serverSocket, &_fdSet);

		CreateSessionPool(_config.maxClientCount + _config.extraClientCount);

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::SendData(const int sessionIndex, const short packetID, const short bodySize, const char* body)
	{
		auto& session = _clientSessionPool[sessionIndex];

		auto pos = session.sendSize;

		// ���� ���۰� ���Ĺ��ȴ�
		if ((pos + bodySize + PACKET_HEADER_SIZE) > _config.maxClientSendBufferSize)
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;

		PacketHeader packetHeader{ packetID,bodySize };
		// ���
		memcpy(&session.sendBuffer[pos], (char*)&packetHeader, PACKET_HEADER_SIZE);
		// ����
		memcpy(&session.sendBuffer[pos + PACKET_HEADER_SIZE], body, bodySize);
		session.sendSize += bodySize + PACKET_HEADER_SIZE;

		return NET_ERROR_CODE::NONE;
	}

	// ���� ����
	void TcpNetwork::Run()
	{
		/*
		readFD�� ������ �����Ͱ� �ִ��� �˾ƺ��� ���� ���ϵ�
		writeFD�� ���ŷ���� �ʰ� �ٷ� �����͸� ���� �� �ִ��� �˾ƺ��� ���� ���ϵ�
		exceptionFD�� ���ܰ� �߻��ߴ��� �˾ƺ��� ���� ���ϵ�
		*/
		auto readSet = _fdSet;
		auto writeSet = _fdSet;
		auto exceptionSet = _fdSet;

		// select timeout ����
		timeval timeout{ 0,1000 }; // sec, micro sec

		// ���� �� �ֳ�, ���� �� �ִ°� �ֳ� 1ms���� Ȯ���غ���.
		auto selectResult = select(0, &readSet, &writeSet, &exceptionSet, &timeout);

		// ���� �� �ְų� ���� �� �ִ°� �ִ��� Ȯ���Ѵ�.
		switch (selectResult)
		{
		case 0: // ���� �͵�, �� �͵� ����.
			return;
		case -1: // ����
			// TODO : �α� �����
			break;
		default:
			break;
		}

		// �������Ͽ� ������ ���� ���
		if (FD_ISSET(_serverSocket, &readSet))
			NewSession();
		else // �������Ͽ� ���� �� ���� ��� == Ŭ���̾�Ʈ ���Ͽ� ���� �� �ְų� Ŭ���̾�Ʈ ���Ͽ� ���� ���� �� �ִ� ���
			Run_ProcessClients(exceptionSet, readSet, writeSet);
		// �׷���, ���� ���Ͽ��� ���� �� �ְ� Ŭ���̾�Ʈ ���Ͽ��� ���� �� ���� ��쿡�� �� ƽ �ǳʶٰ� �Ǵ� �� �ƴѰ�?
		// �� �κ��� ó�����ִ� ���� ���� ��� ������ �ɱ�?

	}

	// ��Ŷ ť���� �ϳ��� �Ҷ� ��� ��Ŷ�� �����ش�.
	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;

		// ��Ŷť�� ���빰�� �־�� ��Ŷ������ �����ֵ� ���� �Ѵ�.
		if (_packetQueue.empty() == false)
		{
			packetInfo = _packetQueue.front();
			_packetQueue.pop();
		}

		return packetInfo;
	}

	NET_ERROR_CODE TcpNetwork::InitServerSocket()
	{
		// winsock ���� ����
		WORD winsockVersion = MAKEWORD(2, 2);
		WSADATA wsaData;
		WSAStartup(winsockVersion, &wsaData);

		// ������ ����, �����ڸ� ���´�.
		_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_serverSocket < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;

		// ���� �ɼ� ����
		auto n = 1;
		if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::BindAndListen(const short port, int backLogCount)
	{
		// ip, port ����
		// accept�� �� ���̹Ƿ� INADDR_ANY
		sockaddr_in serverAddress;
		ZeroMemory(&serverAddress, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // ������ ������ �̷��� �ؾ��Ѵ�.
		serverAddress.sin_port = htons(port);

		// bind()
		if (bind(_serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;

		// listen()
		if (listen(_serverSocket, backLogCount) == SOCKET_ERROR)
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;

		// �α�
		_logger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockfd(%d)", __FUNCTION__, _serverSocket);

		return NET_ERROR_CODE::NONE;
	}

	// ����Ǯ�� �̸� �� ����� �ΰ� ��Ȱ���Ѵ�.
	void TcpNetwork::CreateSessionPool(const int maxClientCount)
	{
		for (int i = 0; i < maxClientCount; ++i)
		{
			ClientSession session;
			ZeroMemory(&session, sizeof(session));
			session.index = i;
			session.receiveBuffer = new char[_config.maxClientRecvBufferSize];
			session.sendBuffer = new char[_config.maxClientSendBufferSize];

			_clientSessionPool.push_back(session);
			_clientSessionPoolIndex.push(session.index);
		}
	}

	// ���� �߰�
	NET_ERROR_CODE TcpNetwork::NewSession()
	{
		sockaddr_in clientAddress;
		auto clientLength = static_cast<int>(sizeof(clientAddress));
		//int length_ClientAddress = sizeof(clientAddress); // �� �̷��� �ȵ�������?
		auto clientSocket = accept(_serverSocket, (SOCKADDR*)&clientAddress, &clientLength);

		if (clientSocket < 0) // ���� ����
		{
			_logger->Write(LOG_TYPE::L_ERROR, "%s | Wrong socket %d cannot accept", __FUNCTION__, clientSocket);
			return NET_ERROR_CODE::ACCEPT_API_ERROR;
		}

		auto newSessionIndex = AllocIndexToClient();
		if (newSessionIndex == -1)
		{
			_logger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%d)  >= MAX_SESSION", __FUNCTION__, clientSocket);

			// ���� Ǯ�� �ڸ��� ����. �ٷ� �߶������.
			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, clientSocket, -1);
			return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}

		char clientIP[MAX_IP_LENGTH] = { 0, };
		// 2���� IP�� 10���� IP�� ��ȯ
		inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, MAX_IP_LENGTH - 1);

		SetSocketOption(clientSocket);

		// ���� ���� ���� fdSet�� ����Ѵ�.
		FD_SET(clientSocket, &_fdSet);

		// ���� ���� ����
		AllocSessionToClient(newSessionIndex, clientSocket, clientIP);

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::AllocSessionToClient(const int sessionIndex, const SOCKET socket, const char* ip)
	{
		++_connectedSeq;
		auto& session = _clientSessionPool[sessionIndex];
		session.sequence = _connectedSeq; // ���°�� ����� ��������
		session.socket = (int)socket;
		memcpy(session.IP, ip, MAX_IP_LENGTH - 1);

		++_connectedSessionCount; // ���� ����� ������ ����

		_logger->Write(LOG_TYPE::L_INFO, "%s | New Session. FD(%d), m_ConnectSeq(%d), IP(%s)", __FUNCTION__, socket, _connectedSeq, ip);
	}

	int TcpNetwork::AllocIndexToClient()
	{
		if (_clientSessionPoolIndex.empty())
			return -1;

		int index = _clientSessionPoolIndex.front();
		_clientSessionPoolIndex.pop();
		return index;
	}

	void TcpNetwork::CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET socket, const int sessionIndex)
	{
		// ����Ǯ�� ������ ����.
		if (closeCase == SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
		{
			closesocket(socket);
			FD_CLR(socket, &_fdSet);
			return;
		}

		// �̹� ���� ����Ǿ� �ִ� ���
		if (_clientSessionPool[sessionIndex].IsConnected() == false)
			return;

		closesocket(socket);
		FD_CLR(socket, &_fdSet);

		_clientSessionPool[sessionIndex].Clear();
		--_connectedSessionCount;
		// ���� �ٽ� �� �� �ִ� �ε����ϱ� push�Ѵ�.
		_clientSessionPoolIndex.push(sessionIndex);

		// ���� �����ٰ� �������� �˷��ش�.
		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	void TcpNetwork::Run_ProcessClients(fd_set& excSet, fd_set& readSet, fd_set& writeSet)
	{
		// ��� ���ǿ� ���� Ȯ���Ѵ�.
		for (int i = 0; i < _clientSessionPool.size(); ++i)
		{
			auto& currentSession = _clientSessionPool[i];

			// ���� �ȵȳ��̸� �� �н�
			if (currentSession.IsConnected() == false)
				continue;

			SOCKET socket = currentSession.socket;
			auto index = currentSession.index;

			// ���� üũ
			if (FD_ISSET(currentSession.socket, &excSet))
			{
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, socket, index);
				continue;
			}

			// �б�
			auto result = Run_ReceiveProcess(currentSession, readSet);
			if(result == false) // ���Ͽ� ���� ������ �ִ�. ���� �������� �Ѿ��.
				continue;

			// ����
			Run_WriteProcess(currentSession, writeSet);
		}
	}

	// ���� ���� ���������� bool, ������ ������ false�� ��ȯ�Ѵ�.
	bool TcpNetwork::Run_ReceiveProcess(const ClientSession& session, fd_set& readSet)
	{
		// �ش� ���ǿ� ���� ���� �ִ��� Ȯ���Ѵ�.
		if (FD_ISSET(session.socket, &readSet) == false)
			return true;

		// �д´�.
		auto result = ReceiveSocket(session.index);
		if (result != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, session.socket, session.index);
			return false;
		}

		// ��Ŷ���� ���� �����Ѵ�.
		result = ProcessPacketFromReceiveBuffer(session.index);
		if (result != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, session.socket, session.index);
			return false;
		}

		// ���������� �б⿡ ���õ� ��� ���μ����� �����ߴ�!
		return true;
	}

	void TcpNetwork::Run_WriteProcess(const ClientSession& session, fd_set& writeSet)
	{
		// �� �� �ִ� �������� ���� Ȯ��
		if (FD_ISSET(session.socket, &writeSet) == false)
			return;

		// ������ �;��� �ֵ��� �� ������.
		auto result = FlushSendBuffer(session.index);
		// �����߻�
		if (result.error != NET_ERROR_CODE::NONE)
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, session.socket, session.index);
	}

	// �ش� ������ send buffer�� �����͸� Ŭ���̾�Ʈ���� ������.
	NetError TcpNetwork::FlushSendBuffer(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		auto socket = session.socket;

		// �׻� ������ �������� �ʾҴ��� Ȯ��
		if (session.IsConnected() == false)
			return NetError(NET_ERROR_CODE::CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE);

		// ���ݱ��� ������; �׾Ƴ����� �� ����������.
		auto result = SendSocket(socket, session.sendBuffer, session.sendSize);

		// �����߻�
		if (result.error != NET_ERROR_CODE::NONE)
			return result;

		// �� ������� ���� �ִ�.
		// �������� ���� ���� ������ ��������.
		auto sentSize = result.value;
		if (sentSize < session.sendSize)
		{
			memmove(session.sendBuffer,
				session.sendBuffer + sentSize,
				session.sendSize - sentSize);
		}
		else
			session.sendSize = 0;

		return result;
	}

	// �ش� �������� �����͸� ������.
	// ���� �Ŀ� ��ŭ �����µ� �����ߴ��� ����� ��ȯ�Ѵ�.
	NetError TcpNetwork::SendSocket(const SOCKET socket, const char* message, const int size)
	{
		NetError result(NET_ERROR_CODE::NONE);

		// ���� �� �ִ� ��͸� �����Ŵ�.
		if (size <= 0)
			return result;

		// ������, ���� ���� �޾ƿ´�.
		result.value = send(socket, message, size, 0);

		// ���� ����� �Ⱥ������� ����
		if (result.value <= 0)
			result.error = NET_ERROR_CODE::SEND_SIZE_ZERO;

		return result;
	}

	// ���Ͽ��� ������ �о�´�.
	// �ش� ���Ͽ� ���� ���� ���� �� �������־�� �Ѵ�.
	// const session&�� ���� �ʰ� sessionIndex�� �޴� ������ �����͸� �����ϰ� ���� session�� ������ �Լ� �ȿ��� �ٲ�� �����̴�.
	NET_ERROR_CODE TcpNetwork::ReceiveSocket(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		auto socket = session.socket;

		// �׻� ������ �������� �ʾҴ��� Ȯ�����ش�.
		if (session.IsConnected() == false)
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;

		// ���� �����͸� ������ ������ ä�����
		int receivePos = 0;

		// ���� receiveBuffer�� �����ִ� �����Ͱ� ���� ���
		if (session.remainingDataSizeInRecvBuf > 0)
		{
			// ���� �������� ��Ŷ���� ������ ���ϰ� ���� �߸� ��Ŷ�� ������ ���� �պκ����� ����ش�.
			memcpy(session.receiveBuffer, session.remainingDataPos, session.remainingDataSizeInRecvBuf);
			// ���� ���� �����ʹ� �̹� �ִ� ������ �ڿ��� ä�� ���̴�.
			receivePos += session.remainingDataSizeInRecvBuf;
		}

		// �޾Ƽ� ���ۿ� ä���.
		auto receivedSize = recv(socket, session.receiveBuffer + receivePos, (MAX_PACKET_BODY_SIZE * 2), 0); // �� *2�� ���ִ��� �� �𸣰ڴ�.

		// ���� ���� �� ������
		if(receivedSize == 0)
			return NET_ERROR_CODE::RECV_REMOTE_CLOSE;

		// �������� ������ ��������
		if (receivedSize < 0)
		{
			auto error = WSAGetLastError();
			if (ERROR != WSAEWOULDBLOCK)
				return NET_ERROR_CODE::RECV_API_ERROR;
			else // WWSAWOULDBLOCK�� ���ִ� �����شٴ� ���̹Ƿ� ��������.
				return NET_ERROR_CODE::NONE;
		}

		// �ش� ���ǿ� �����ִ� �� ������ ����� ���� ���� ������ ����� �����ش�.
		session.remainingDataSizeInRecvBuf += receivedSize;

		return NET_ERROR_CODE::NONE;
	}

	// �ش� ������ receiveBuffer���� ��Ŷ�� ���� ��Ŷť�� ����ִ´�.
	NET_ERROR_CODE TcpNetwork::ProcessPacketFromReceiveBuffer(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];

		auto readPos = 0;
		const auto entireDataSize = session.remainingDataSizeInRecvBuf;
		PacketHeader* packetHeader;
		
		// �����ִ� ������ ������
		auto& remainingDataSize = session.remainingDataSizeInRecvBuf;

		// ��Ŷ ����� ����⿡ ����� �����Ͱ� �ִ� ���� ��� ��Ŷ�� �����.
		while (session.remainingDataSizeInRecvBuf >= PACKET_HEADER_SIZE)
		{
			// ����κ� ��ġ ����
			packetHeader = (PacketHeader*)session.receiveBuffer + readPos;
			readPos += PACKET_HEADER_SIZE;

			// ����� �о�� �ٵ� �ִ��� �ϸ�
			if (packetHeader->bodySize > 0)
			{
				// ������ �ڵ忡���� <�� �Ǿ��־�����, bodySize�� �����ִ� ������� Ŭ ��쿡 ��Ŷ �ȸ���� �ѱ�� �ǹ��̹Ƿ�, >�� �´� �� ����.
				if (packetHeader->bodySize > session.remainingDataSizeInRecvBuf)
					break;
				if (packetHeader->bodySize > MAX_PACKET_BODY_SIZE)
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
			}

			// ������� ������ �ٵ���� ����⿡ ����ϰų�, ����� ���� �Ǵ� ����̴�.
			// ��Ŷ�� ���� ť�� �־������.
			AddPacketQueue(sessionIndex, packetHeader->id, packetHeader->bodySize, session.receiveBuffer + readPos);

			// �̹� ó�������ϱ� �� �ڷ� ����.
			readPos += packetHeader->bodySize;
			// �̹� �������� ��Ŷ�� ���� ��ŭ remainingData�� �ٿ��ش�.
			remainingDataSize -= readPos;
		}

// 		// ������ �ٵ��Ҵµ�, ��Ŷ���� ������� �ڿ� �߸� ��Ŷ�� �ִ� ��� ������ ������ ����ش�.
// 		// �� ���� ��Ŷ�� �߷��� ���ƿ� ����̴�.
// 		if (remainingDataSize > 0)
// 			memcpy(session.receiveBuffer, session.receiveBuffer + readPos, session.remainingDataSizeInRecvBuf);
		
		// ��Ŷ���� ������ ���ϰ� ������ �߸� ��Ŷ�� ��ġ�� �����صд�.
		// ���� �������� recv�ϱ� ���� ���� �պκ����� ����� ���̴�.
		session.remainingDataPos = session.receiveBuffer + readPos;


		return NET_ERROR_CODE::NONE;
	}

	// ��Ŷ�� ���� ��Ŷť�� �߰��Ѵ�.
	void TcpNetwork::AddPacketQueue(const int sessionIndex, const short packetId, const short bodySize, char* pDataPos)
	{
		RecvPacketInfo packetInfo;
		packetInfo.sessionIndex = sessionIndex;
		packetInfo.packetId = packetId;
		packetInfo.packetBodySize = bodySize;

		// �����͸� �������� �ʰ� ������ �ش� �ּҸ� �Ѱ��ִ� ������
		// ������ ��Ʈ��ũ ���̺귯���� ��Ƽ������� �������� �ʰ� ��Ʈ��ũ ���� 1ȸ�� ������ ���� �������� ��Ŷť�� ���� �ص��ϱ� �����̴�.
		// ������, ��Ŷ�� ©���� ��� ProcessPacketFromReceiveBuffer()�Լ������� �߷��� ��� memcpy()�ϴ� �ڵ嶧����
		// ���� ����Ű���� �ߴ� �������� �� �ڸ� ����Ű�� �Ǳ� ������ ���װ� �߻��� ���̶�� �����Ѵ�.
		packetInfo.dataAddress = pDataPos;
	}

	// ���� �ɼ� ����
	void TcpNetwork::SetSocketOption(const SOCKET socket)
	{
		// ����� ���� ���θ� ����
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(socket, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		// �� �̷��� �ϴ��� �� �𸣰ڴ�.
		int size1 = _config.maxClientSockOptRecvBufferSize;
		int size2 = _config.maxClientSockOptSendBufferSize;
		setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
		setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));
	}

}