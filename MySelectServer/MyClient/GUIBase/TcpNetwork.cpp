#include "TcpNetwork.h"
#include <thread>

/**
	������ ���� �õ�.
	���� ���� ���θ� ��ȯ�Ѵ�.
*/
bool TcpNetwork::ConnectTo(const char* hostIP, const int portNum)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == INVALID_SOCKET)
		return false;

	// ���� �ּ� ����
	sockaddr_in serverAdress;
	ZeroMemory(&serverAdress, sizeof(serverAdress));
	serverAdress.sin_family = AF_INET;
	inet_pton(AF_INET, hostIP, (void*)&serverAdress.sin_addr.s_addr);// ip ����
	serverAdress.sin_port = htons(portNum); // port ����

	// ������ ����
	if (connect(_socket, (SOCKADDR*)&serverAdress, sizeof(serverAdress)) != 0)
		return false;
	// ���� ������ ���� ����

	_isConnected = true;

	// ���� ���������� Update ������ ����.
	_thread = std::thread([&]() { Update(); });

	return true;
}

/**
	�������� ������ �����Ѵ�.
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
	��Ŷ ����
*/
void TcpNetwork::SendPacket(const short packetID, const short dataSize, char* pData)
{
	char data[MAX_PACKET_SIZE] = { 0, };
	// ���
	PacketHeader packetHeader{ packetID,dataSize };
	memcpy(data, (char*)&packetHeader, PACKET_HEADER_SIZE);

	// ������
	if (dataSize > 0)
		memcpy(data + PACKET_HEADER_SIZE, pData, dataSize);

	// ����
	send(_socket, data, dataSize + PACKET_HEADER_SIZE, 0);
}

// Update() ����
void TcpNetwork::Update()
{
	while (_isConnected)
	{
		RecvData();
		RecvBufferProcess();
	}
}

// Ŭ���̾�Ʈ���� ��Ŷ�� �ϳ��� �ش�.
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

// ��Ʈ��ũ���� �����͸� �޾Ƽ� ���ۿ� ä���ִ´�.
void TcpNetwork::RecvData()
{
	char recvBuffer[MAX_PACKET_SIZE];

	auto recvSize = recv(_socket, recvBuffer, MAX_PACKET_SIZE, 0);

	if (recvSize == 0) // ���� ����
		return;
	if (recvSize < 0)
		if (WSAGetLastError() != WSAEWOULDBLOCK) // WSAEWOULDBLOCK�� ��쿡�� ��� ��޷��޶�� ������ ���� ���� ����̴�.
			return;

	// ���� �����÷ο�
	if ((_recvSize + recvSize) >= MAX_SOCK_RECV_BUFFER)
		return;

	memcpy(&_recvBuffer[_recvSize], recvBuffer, recvSize);
	_recvSize += recvSize;
}

// ���ۿ� ä���� ��Ŷ�� �߶� ��Ŷť�� �־��ش�.
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

		// Ŭ���̾�Ʈ�� ��Ŷ ť�� ä���ش�.
		AddPacketQueue(pPacketHeader->id, pPacketHeader->bodySize, _recvBuffer + readPos);

		readPos += pPacketHeader->bodySize;
	}
	
	_recvSize -= readPos;

	// ���� ��Ŷ ��Ⱑ ���������� recieve buffer�� �ٽ� ä�����´�.
	// �̷��� �ϴ� ������ ������ ��Ŷ�� �߷��� �� ���� �ֱ� ����, ��ٸ��� �ٽ� �پ �´�.
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
