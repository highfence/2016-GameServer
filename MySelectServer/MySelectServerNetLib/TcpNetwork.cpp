#include "TcpNetwork.h"


namespace MySelectServerNetLib
{
	TcpNetwork::TcpNetwork()
	{
	}


	TcpNetwork::~TcpNetwork()
	{
		for (auto& client : _clientSessionPool)
		{
			if (client.receiveBuffer)
				delete[] client.receiveBuffer;
			if (client.sendBuffer)
				delete[] client.sendBuffer;
		}
	}

	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* config, ILogger* logger)
	{
		// 서버 셋팅 로드
		memcpy(&_config,config,sizeof(_config));

		// 로거 셋팅
		_logger = logger;

		// 서버 소켓 생성 후 초기화
		auto result = InitServerSocket();
		if (result != NET_ERROR_CODE::NONE)
			return result;

		// bind하고 listen한다.
		result = BindAndListen(_config.port, _config.backLogCount);
		if (result != NET_ERROR_CODE::NONE)
			return result;
		
		// 서버 소켓을 fdSet에 등록하여 앞으로 select할때 정보를 받아볼 수 있도록 한다.
		FD_ZERO(&_fdSet);
		FD_SET(_serverSocket, &_fdSet);

		CreateSessionPool(_config.maxClientCount + _config.extraClientCount);

		return NET_ERROR_CODE::NONE;
	}

	MySelectServerNetLib::NET_ERROR_CODE TcpNetwork::SendData(const int sessionIndex, const short packetID, const short bodySize, const char* body)
	{
		auto& session = _clientSessionPool[sessionIndex];

		auto pos = session.sendSize;

		// 보낼 버퍼가 넘쳐버렸당
		if ((pos + bodySize + PACKET_HEADER_SIZE) > _config.maxClientSendBufferSize)
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;

		PacketHeader packetHeader{ packetID,bodySize };
		// 헤더
		memcpy(&session.sendBuffer[pos], (char*)&packetHeader, PACKET_HEADER_SIZE);
		// 몸통
		memcpy(&session.sendBuffer[pos + PACKET_HEADER_SIZE], body, bodySize);
		session.sendSize += bodySize + PACKET_HEADER_SIZE;

		return NET_ERROR_CODE::NONE;
	}

	// 메인 루프
	void TcpNetwork::Run()
	{
		/*
		readFD는 수신할 데이터가 있는지 알아보고 싶은 소켓들
		writeFD는 블로킹되지 않고 바로 데이터를 보낼 수 있는지 알아보고 싶은 소켓들
		exceptionFD는 예외가 발생했는지 알아보고 싶은 소켓들
		*/
		auto readSet = _fdSet;
		auto writeSet = _fdSet;
		auto exceptionSet = _fdSet;

		// select timeout 설정
		timeval timeout{ 0,1000 }; // sec, micro sec

		// 읽을 게 있나, 보낼 수 있는게 있나 1ms동안 확인해본다.
		auto selectResult = select(0, &readSet, &writeSet, &exceptionSet, &timeout);

		// 읽을 게 있거나 보낼 수 있는게 있는지 확인한다.
		switch (selectResult)
		{
		case 0: // 읽을 것도, 쓸 것도 없다.
			return;
		case -1: // 오류
			// TODO : 로그 남기기
			break;
		default:
			break;
		}

		// 서버소켓에 읽을게 있을 경우
		if (FD_ISSET(_serverSocket, &readSet))
			NewSession();
		else // 서버소켓에 읽을 게 없을 경우 == 클라이언트 소켓에 읽을 게 있거나 클라이언트 소켓에 무언가 보낼 게 있는 경우
			Run_ProcessClients(exceptionSet, readSet, writeSet);
		// 그런데, 서버 소켓에도 읽을 게 있고 클라이언트 소켓에도 읽을 게 있을 경우에는 한 틱 건너뛰게 되는 것 아닌가?
		// 그 부분을 처리해주는 것이 성능 향상에 도움이 될까?

	}

	// 패킷 큐에서 하나씩 뚝뚝 떼어서 패킷을 나눠준다.
	// 더이상 패킷이 없으면 .packetId가 0인 패킷을 리턴
	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;
		// 패킷큐에 내용물이 있어야 패킷정보를 떼어주든 말든 한다.
		if (_packetQueue.empty() == false)
		{
			packetInfo = _packetQueue.front();
			_packetQueue.pop();
		}

		return packetInfo;
	}

	NET_ERROR_CODE TcpNetwork::InitServerSocket()
	{
		// winsock 버전 셋팅
		WORD winsockVersion = MAKEWORD(2, 2);
		WSADATA wsaData;
		WSAStartup(winsockVersion, &wsaData);

		// 소켓을 생성, 접근자를 얻어온다.
		_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_serverSocket < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;

		// 소켓 옵션 설정
		auto n = 1;
		if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::BindAndListen(const short port, int backLogCount)
	{
		// ip, port 셋팅
		// accept에 쓸 것이므로 INADDR_ANY
		sockaddr_in serverAddress;
		ZeroMemory(&serverAddress, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // 아이피 셋팅은 이렇게 해야한다.
		serverAddress.sin_port = htons(port);
		
		// bind()
		if (bind(_serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) < 0)
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;

		// listen()
		if (listen(_serverSocket, backLogCount) == SOCKET_ERROR)
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;

		// 로그
		_logger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockfd(%d)", __FUNCTION__, _serverSocket);

		return NET_ERROR_CODE::NONE;
	}

	// 세션풀을 미리 다 만들어 두고 재활용한다.
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

	// 세션 추가
	NET_ERROR_CODE TcpNetwork::NewSession()
	{
		sockaddr_in clientAddress;
		auto clientLength = static_cast<int>(sizeof(clientAddress));
		//int length_ClientAddress = sizeof(clientAddress); // 왜 이렇게 안돼있을까?
		auto clientSocket = accept(_serverSocket, (SOCKADDR*)&clientAddress, &clientLength);

		if (clientSocket < 0) // 소켓 에러
		{
			_logger->Write(LOG_TYPE::L_ERROR, "%s | Wrong socket %d cannot accept", __FUNCTION__, clientSocket);
			return NET_ERROR_CODE::ACCEPT_API_ERROR;
		}

		auto newSessionIndex = AllocIndexToClient();
		if (newSessionIndex == -1)
		{
			_logger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%d)  >= MAX_SESSION", __FUNCTION__, clientSocket);

			// 세션 풀에 자리가 없다. 바로 잘라버린다.
			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, clientSocket, -1);
			return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}

		char clientIP[MAX_IP_LENGTH] = { 0, };
		// 2진수 IP를 10진수 IP로 변환
		inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, MAX_IP_LENGTH - 1);

		SetSocketOption(clientSocket);

		// 새로 만든 놈을 fdSet에 등록한다.
		FD_SET(clientSocket, &_fdSet);

		// 세션 정보 셋팅
		AllocSessionToClient(newSessionIndex, clientSocket, clientIP);

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::AllocSessionToClient(const int sessionIndex, const SOCKET socket, const char* ip)
	{
		++_connectedSeq;
		auto& session = _clientSessionPool[sessionIndex];
		session.sequence = _connectedSeq; // 몇번째로 연결된 세션인지
		session.socket = (int)socket;
		memcpy(session.IP, ip, MAX_IP_LENGTH - 1);

		++_connectedSessionCount; // 현재 연결된 세션의 갯수

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
		// 세션풀에 여분이 없다.
		if (closeCase == SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
		{
			closesocket(socket);
			FD_CLR(socket, &_fdSet);
			return;
		}

		// 이미 연결 종료되어 있는 경우
		if (_clientSessionPool[sessionIndex].IsConnected() == false)
			return;

		closesocket(socket);
		FD_CLR(socket, &_fdSet);

		_clientSessionPool[sessionIndex].Clear();
		--_connectedSessionCount;
		// 이제 다시 쓸 수 있는 인덱스니까 push한다.
		_clientSessionPoolIndex.push(sessionIndex);

		// 세션 닫혔다고 로직한테 알려준다.
		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	void TcpNetwork::Run_ProcessClients(fd_set& excSet, fd_set& readSet, fd_set& writeSet)
	{
		// 모든 세션에 대해 확인한다.
		for (int i = 0; i < _clientSessionPool.size(); ++i)
		{
			auto& currentSession = _clientSessionPool[i];

			// 연결 안된놈이면 걍 패스
			if (currentSession.IsConnected() == false)
				continue;

			SOCKET socket = currentSession.socket;
			auto index = currentSession.index;

			// 에러 체크
			if (FD_ISSET(currentSession.socket, &excSet))
			{
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, socket, index);
				continue;
			}

			// 읽기
			auto result = Run_ReceiveProcess(currentSession, readSet);
			if(result == false) // 소켓에 뭔가 문제가 있다. 다음 세션으로 넘어간다.
				continue;

			// 쓰기
			Run_WriteProcess(currentSession, writeSet);
		}
	}

	// 에러 없이 수행했으면 true, 에러가 났으면 false를 반환한다.
	bool TcpNetwork::Run_ReceiveProcess(const ClientSession& session, fd_set& readSet)
	{
		// 해당 세션에 읽을 놈이 있는지 확인한다.
		if (FD_ISSET(session.socket, &readSet) == false)
			return true;

		// 읽는다.
		auto result = ReceiveSocket(session.index);
		if (result != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, session.socket, session.index);
			return false;
		}

		// 패킷으로 만들어서 저장한다.
		result = ProcessPacketFromReceiveBuffer(session.index);
		if (result != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, session.socket, session.index);
			return false;
		}

		// 정상적으로 읽기에 관련된 모든 프로세스를 수행했다!
		return true;
	}

	void TcpNetwork::Run_WriteProcess(const ClientSession& session, fd_set& writeSet)
	{
		// 쓸 수 있는 상태인지 먼저 확인
		if (FD_ISSET(session.socket, &writeSet) == false)
			return;

		// 보내고 싶었던 애들을 싹 보낸다.
		auto result = FlushSendBuffer(session.index);
		// 에러발생
		if (result.error != NET_ERROR_CODE::NONE)
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, session.socket, session.index);
	}

	// 해당 세션의 send buffer의 데이터를 클라이언트에게 보낸다.
	NetError TcpNetwork::FlushSendBuffer(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		auto socket = session.socket;

		// 그새 연결이 끊기지는 않았는지 확인
		if (session.IsConnected() == false)
			return NetError(NET_ERROR_CODE::CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE);

		// 지금까지 보내고싶어서 쌓아놨던걸 싹 보내버린다.
		auto result = SendSocket(socket, session.sendBuffer, session.sendSize);

		// 에러발생
		if (result.error != NET_ERROR_CODE::NONE)
			return result;

		// 다 못비웠을 수도 있다.
		// 못보내고 남은 놈은 앞으로 땡겨주자.
		auto sentSize = result.value;
		if (sentSize < session.sendSize)
		{
			memmove(session.sendBuffer,
				session.sendBuffer + sentSize,
				session.sendSize - sentSize);
			session.sendSize -= sentSize;
		}
		else
			session.sendSize = 0;

		return result;
	}

	// 해당 소켓으로 데이터를 보낸다.
	// 보낸 후에 얼만큼 보내는데 성공했는지 사이즈를 반환한다.
	NetError TcpNetwork::SendSocket(const SOCKET socket, const char* message, const int size)
	{
		NetError result(NET_ERROR_CODE::NONE);

		// 보낼 게 있는 놈것만 보낼거다.
		if (size <= 0)
			return result;

		// 보내고, 보낸 양을 받아온다.
		result.value = send(socket, message, size, 0);

		// 뭔가 제대로 안보내지면 에러
		if (result.value <= 0)
			result.error = NET_ERROR_CODE::SEND_SIZE_ZERO;

		return result;
	}

	// 소켓에서 정보를 읽어온다.
	// 해당 소켓에 읽을 것이 있을 때 실행해주어야 한다.
	// const session&을 받지 않고 sessionIndex를 받는 이유는 데이터를 수신하고 나면 session의 내용이 함수 안에서 바뀌기 때문이다.
	NET_ERROR_CODE TcpNetwork::ReceiveSocket(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];
		auto socket = session.socket;

		// 그새 연결이 끊기지는 않았는지 확인해준다.
		if (session.IsConnected() == false)
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;

		// 받은 데이터를 버퍼의 어디부터 채울건지
		int receivePos = 0;

		// 아직 receiveBuffer에 남아있는 데이터가 있을 경우
		if (session.remainingDataSizeInRecvBuf > 0)
		{
			// 이전 루프에서 패킷으로 만들지 못하고 남긴 잘린 패킷을 버퍼의 가장 앞부분으로 당겨준다.
			memcpy(session.receiveBuffer, session.remainingDataPos, session.remainingDataSizeInRecvBuf);
			// 새로 받은 데이터는 이미 있던 데이터 뒤에다 채울 것이다.
			receivePos += session.remainingDataSizeInRecvBuf;
		}
		else
			session.remainingDataPos = 0;
		// 받아서 버퍼에 채운다.
		auto receivedSize = recv(socket, session.receiveBuffer + receivePos, (MAX_PACKET_BODY_SIZE * 2), 0); // 왜 *2를 해주는지 잘 모르겠다.

		// 만약 받은 게 없으면
		if(receivedSize == 0)
			return NET_ERROR_CODE::RECV_REMOTE_CLOSE;

		// 받으려다 에러가 나버리면
		if (receivedSize < 0)
		{
			auto error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK)
				return NET_ERROR_CODE::RECV_API_ERROR;
			else // WWSAWOULDBLOCK은 좀있다 보내준다는 뜻이므로 문제없다.
				return NET_ERROR_CODE::NONE;
		}

		// 해당 세션에 남아있는 총 데이터 사이즈에 새로 받은 데이터 사이즈를 더해준다.
		session.remainingDataSizeInRecvBuf += receivedSize;

		return NET_ERROR_CODE::NONE;
	}

	// 해당 세션의 receiveBuffer에서 패킷을 만들어서 패킷큐에 집어넣는다.
	NET_ERROR_CODE TcpNetwork::ProcessPacketFromReceiveBuffer(const int sessionIndex)
	{
		auto& session = _clientSessionPool[sessionIndex];

		auto readPos = 0;
		const auto entireDataSize = session.remainingDataSizeInRecvBuf;
		PacketHeader* packetHeader;
		
		// 남아있는 데이터 사이즈
		auto& remainingDataSize = session.remainingDataSizeInRecvBuf;

		// 패킷 헤더를 만들기에 충분한 데이터가 있는 동안 계속 패킷을 만든다.
		while (remainingDataSize >= PACKET_HEADER_SIZE)
		{
			// 헤더 만들기
			packetHeader = (PacketHeader*)session.receiveBuffer + readPos;
			readPos += PACKET_HEADER_SIZE;

			// 헤더를 읽어보니 바디가 있더라 하면
			if (packetHeader->bodySize > 0)
			{
				// 교수님 코드에서는 <로 되어있었으나, bodySize가 남아있는 사이즈보다 클 경우에 패킷 안만들고 넘기는 의미이므로, >가 맞는 것 같다.
				if (packetHeader->bodySize + PACKET_HEADER_SIZE > remainingDataSize)
				{
					readPos -= PACKET_HEADER_SIZE;
					break;
				}
				if (packetHeader->bodySize > MAX_PACKET_BODY_SIZE)
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
			}

			// 여기까지 왔으면 바디까지 만들기에 충분하거나, 헤더만 만들어도 되는 경우이다.
			// 패킷을 만들어서 큐에 넣어버린다.
			AddPacketQueue(sessionIndex, packetHeader->id, packetHeader->bodySize, session.receiveBuffer + readPos);

			// 이미 처리했으니까 더 뒤로 간다.
			readPos += packetHeader->bodySize;
			// 이번 루프에서 패킷을 만든 만큼 remainingData를 줄여준다.
			remainingDataSize -= PACKET_HEADER_SIZE + packetHeader->bodySize;
		}

// 		// 루프를 다돌았는데, 패킷으로 못만들고 뒤에 잘린 패킷이 있는 경우 버퍼의 앞으로 당겨준다.
// 		// 이 경우는 패킷이 잘려서 날아온 경우이다.
// 		if (remainingDataSize > 0)
// 			memcpy(session.receiveBuffer, session.receiveBuffer + readPos, session.remainingDataSizeInRecvBuf);
		
		// 패킷으로 만들지 못하고 남겨진 잘린 패킷의 위치를 저장해둔다.
		// 다음 루프에서 recv하기 전에 버퍼 앞부분으로 당겨줄 것이다.
		session.remainingDataPos = session.receiveBuffer + readPos;


		return NET_ERROR_CODE::NONE;
	}

	// 패킷을 만들어서 패킷큐에 추가한다.
	void TcpNetwork::AddPacketQueue(const int sessionIndex, const short packetId, const short bodySize, char* pDataPos)
	{
		RecvPacketInfo packetInfo;
		packetInfo.sessionIndex = sessionIndex;
		packetInfo.packetId = packetId;
		packetInfo.packetBodySize = bodySize;

		// 데이터를 복사하지 않고 버퍼의 해당 주소를 넘겨주는 이유는
		// 로직과 네트워크 라이브러리가 멀티스레드로 동작하지 않고 네트워크 루프 1회당 무조건 로직 루프에서 패킷큐를 전부 해독하기 때문이다.
		// 하지만, 패킷이 짤렸을 경우 ProcessPacketFromReceiveBuffer()함수에서의 잘렸을 경우 memcpy()하는 코드때문에
		// 원래 가리키려고 했던 번지보다 더 뒤를 가리키게 되기 때문에 버그가 발생할 것이라고 생각한다.
		packetInfo.dataAddress = pDataPos;


		_packetQueue.push(packetInfo);
	}

	// 소켓 옵션 설정
	void TcpNetwork::SetSocketOption(const SOCKET socket)
	{
		// 우아한 종료 여부를 설정
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(socket, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		// 왜 이렇게 하는지 잘 모르겠다.
		int size1 = _config.maxClientSockOptRecvBufferSize;
		int size2 = _config.maxClientSockOptSendBufferSize;
		setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
		setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));
	}

}