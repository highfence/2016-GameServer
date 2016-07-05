#pragma once

namespace MySelectServerNetLib
{
	struct ServerConfig
	{
		unsigned short port;
		int backLogCount;

		int maxClientCount;
		int extraClientCount; // 로그인 단계에서 자를 수 있도록 여유분을 조금 준비한다.

		short maxClientRecvBufferSize;
		short maxClientSendBufferSize;

		short maxClientSockOptRecvBufferSize;
		short maxClientSockOptSendBufferSize;

		int	maxLobbyCount;
		int maxUserPerLobby;
		int maxRoomPerLobby;
		int maxUserPerRoom;
	};

	enum class SOCKET_CLOSE_CASE : short
	{
		SESSION_POOL_EMPTY = 1,
		SELECT_ERROR = 2,
		SOCKET_RECV_ERROR = 3,
		SOCKET_RECV_BUFFER_PROCESS_ERROR = 4,
		SOCKET_SEND_ERROR = 5,
	};

	const int MAX_IP_LENGTH = 32;
	const int MAX_PACKET_BODY_SIZE = 1024;

	struct ClientSession
	{
		bool	IsConnected() { return socket > 0 ? true : false; }

		void	Clear()
		{
			sequence = 0;
			socket = 0;
			IP[0] = '\0';
			remainingDataSizeInRecvBuf = 0;
			sendSize = 0;
		}

		int			index = 0; // 서버로부터 할당받은 세션 인덱스
		long long	sequence = 0; // 몇 번째로 접속한 세션인지
		int			socket = 0;
		char		IP[MAX_IP_LENGTH] = { 0, };

		char*		receiveBuffer = nullptr;
		int			remainingDataSizeInRecvBuf = 0;
		char*		remainingDataPos = nullptr; // 이전 루프에서 패킷으로 만들지 못하고 남은 잘린패킷의 시작지점

		char*		sendBuffer = nullptr;
		int			sendSize = 0;
	};

	struct RecvPacketInfo
	{
		int sessionIndex = 0;
		short packetId = 0;
		short packetBodySize = 0;
		char* dataAddress;
	};

#pragma pack(push,1)
	struct PacketHeader
	{
		short id;
		short bodySize;
	};

	const int PACKET_HEADER_SIZE = sizeof(PacketHeader);
#pragma pack(pop)

	enum class PACKET_ID : short
	{
		NTF_SYS_CLOSE_SESSION = 3,

	};
}