#include "TcpNetwork.h"
#include <thread>

/**
	서버에 접속 시도.
	접속 성공 여부를 반환한다.
*/
bool TcpNetwork::ConnectTo(const char* hostIP, const int portNum)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == INVALID_SOCKET)
		return false;

	// 서버 주소 셋팅
	sockaddr_in serverAdress;
	ZeroMemory(&serverAdress, sizeof(serverAdress));
	serverAdress.sin_family = AF_INET;
	inet_pton(AF_INET, hostIP, (void*)&serverAdress.sin_addr.s_addr);// ip 셋팅
	serverAdress.sin_port = htons(portNum); // port 셋팅

	// 서버에 연결
	if (connect(_socket, (SOCKADDR*)&serverAdress, sizeof(serverAdress)) != 0)
		return false;
	// 여기 왔으면 연결 성공

	_isConnected = true;

	// 연결 성공했으니 Update 루프를 돌자.
	_thread = std::thread([&]() { Update(); });

	return true;
}

/**
	서버와의 연결을 해제한다.
*/
void TcpNetwork::DisConnect()
{
	if (_isConnected)
	{
		closesocket(_socket);
		Clear();
	}

	if (_thread.joinable())
		_thread.join();
}

/**
	패킷 전송
*/
void TcpNetwork::SendPacket(const short packetID, const short dataSize, char* pData)
{
	char data[MAX_PACKET_SIZE] = { 0, };

	// 헤더
	PacketHeader packetHeader{ packetID,dataSize };
	memcpy(data, (char*)&packetHeader, PACKET_HEADER_SIZE);

	// 데이터
	if (dataSize > 0)
		memcpy(data + PACKET_HEADER_SIZE, pData, dataSize);

	// 전송
	send(_socket, data, dataSize + PACKET_HEADER_SIZE, 0);
}

// Update() 루프
void TcpNetwork::Update()
{
	while (_isConnected)
	{
		RecvData();
		RecvBufferProcess();
	}
}

// 클라이언트한테 패킷을 하나씩 준다.
RecvPacketInfo TcpNetwork::GetPacket()
{
	std::lock_guard<std::mutex> guard(_mutex);

	if (_packetQueue.empty()) {
		return RecvPacketInfo();
	}

	auto packet = _packetQueue.front();
	_packetQueue.pop_front();
	return packet;
}

// 네트워크에서 데이터를 받아서 버퍼에 채워넣는다.
void TcpNetwork::RecvData()
{
	char recvBuffer[MAX_PACKET_SIZE];

	auto recvSize = recv(_socket, recvBuffer, MAX_PACKET_SIZE, 0);

	if (recvSize == 0) // 접속 실패
		return;
	if (recvSize < 0)
		if (WSAGetLastError() != WSAEWOULDBLOCK) // WSAEWOULDBLOCK의 경우에는 잠깐 기달려달라는 뜻으로 문제 없는 경우이다.
			return;

	// 버퍼 오버플로우
	if ((_recvSize + recvSize) >= MAX_SOCK_RECV_BUFFER)
		return;

	memcpy(&_recvBuffer[_recvSize], recvBuffer, recvSize);
	_recvSize += recvSize;
}

// 버퍼에 채워진 패킷을 잘라서 패킷큐에 넣어준다.
void TcpNetwork::RecvBufferProcess()
{
	auto readPos = 0;
	const auto dataSize = _recvSize;
	PacketHeader* pPacketHeader;

	while ((dataSize - readPos) > PACKET_HEADER_SIZE)
	{
		pPacketHeader = (PacketHeader*)&_recvBuffer[readPos];
		readPos += PACKET_HEADER_SIZE;
		
		if (pPacketHeader->bodySize > (dataSize - readPos))
			break;

		if (pPacketHeader->bodySize > MAX_PACKET_SIZE)
			return; // NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET

		// 클라이언트의 패킷 큐에 채워준다.
		AddPacketQueue(pPacketHeader->id, pPacketHeader->bodySize, _recvBuffer + readPos);

		readPos += pPacketHeader->bodySize;
	}
	
	_recvSize -= readPos;

	// 아직 패킷 찌꺼기가 남아있으면 recieve buffer에 다시 채워놓는다.
	// 이렇게 하는 이유는 가끔씩 패킷이 잘려서 올 때가 있기 때문, 기다리면 다시 붙어서 온다.
	if (_recvSize > 0)
		memcpy(_recvBuffer, _recvBuffer + readPos, _recvSize);
}

void TcpNetwork::AddPacketQueue(const short pktId, const short bodySize, char* pDataPos)
{
	RecvPacketInfo packetInfo;
	packetInfo.packetId = pktId;
	packetInfo.packetBodySize = bodySize;
	packetInfo.pData = new char[bodySize];
	memcpy(packetInfo.pData, pDataPos, bodySize);

	std::lock_guard<std::mutex> guard(_mutex);
	_packetQueue.push_back(packetInfo);
}

void TcpNetwork::Clear()
{
	_isConnected = false;
	_recvSize = 0;
	_packetQueue.clear();
}
