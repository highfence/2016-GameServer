#include <winsock2.h>

#define SERVER_PORT 23452
#define BUFSIZE		512

struct SocketInfo
{
	SOCKET socket;
	char buf[BUFSIZE + 1];
	int recvBytes;
	int sendBytes;
};

int nTotalSockets = 0;
SocketInfo* socketInfoArray[FD_SETSIZE];

// 소켓 정보 관리 함수
bool AddSocketInfo(SOCKET socket);
void RemoveSocketInfo(int nIndex);

// 오류 출력 함수
void err_quit(char* msg);
void err_display(char* msg);

int main(int argc, char*argv[])
{
	int retval;

	// winsock 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) // 2,2의 의미는 뭘까?
		return 1;

	// socket()
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) err_quit("socket() failed");

	// bind()
	sockaddr_in serverAddress;
	ZeroMemory(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(SERVER_PORT);
	retval = bind(listenSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
	if (retval == SOCKET_ERROR) err_quit("bind() failed");

	// listen()
	retval = listen(listenSocket, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen() failed");

	// 넌블로킹 소켓으로 전환. 이 부분이 핵심인 듯
	u_long on = 1;
	retval = ioctlsocket(listenSocket, FIONBIO, &on);
	if (retval == SOCKET_ERROR) err_display("ioctlsocket() failed");

	fd_set readSet, writeSet;
	SOCKET clientSocket;
	sockaddr_in clientAddress;
	int addrLen;

	while (1)
	{
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_SET(listenSocket, &readSet);

		for (int i = 0; i < nTotalSockets; ++i)
		{
			if (socketInfoArray[i]->recvBytes > socketInfoArray[i]->sendBytes)
				FD_SET(socketInfoArray[i]->socket, &writeSet);
			else
				FD_SET(socketInfoArray[i]->socket, &readSet);
		}



	}


}