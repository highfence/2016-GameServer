#pragma once

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>

#include "ErrorCode.h"
#include "PacketID.h"
#include "Packet.h"

const int MAX_PACKET_SIZE = 1024;
const int MAX_SOCK_RECV_BUFFER = 8016;

// 패킷 헤더 구조체
struct PacketHeader
{

};


// 패킷 구조체
struct RecvPacketInfo
{

};


class TcpNetwork
{
public:
	TcpNetwork() {}
	~TcpNetwork() {}

	bool ConnectTo(const char* hostIP, int portNum);
	
	bool IsConnected();

	void DisConnect();
	void SendPacket(const short packetID, const short dataSize, char* pData);

	void Update();

	RecvPacketInfo GetPacket();


private:
	void RecvData();
	
	void RecvBufferProcess();
	
	void AddPacketQueue(const short packetId, const short bodySize, char* pDataPos);

	void Clear();

	bool m_IsConnected = false;

	std::thread _thread;
	std::mutex _mutex;

	SOCKET _socket;

	int _recvSize = 0;
	char m_RecvBuffer[MAX_SOCK_RECV_BUFFER] = { 0, };

	std::deque<RecvPacketInfo> m_PacketQueue;
};