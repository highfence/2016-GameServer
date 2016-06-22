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

// ���� ���� ���� �Լ�
bool AddSocketInfo(SOCKET socket);
void RemoveSocketInfo(int nIndex);

// ���� ��� �Լ�
void err_quit(char* msg);
void err_display(char* msg);

int main(int argc, char*argv[])
{
	int retval;

	// winsock �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) // 2,2�� �ǹ̴� ����?
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

	// �ͺ��ŷ �������� ��ȯ. �� �κ��� �ٽ��� ��
	u_long on = 1;
	retval = ioctlsocket(listenSocket, FIONBIO, &on);
	if (retval == SOCKET_ERROR) err_display("ioctlsocket() failed");

	fd_set readSet, writeSet;
	SOCKET clientSocket;
	sockaddr_in clientAddress;
	int addrLen;

	while (1)
	{
		// ���� �� �ʱ�ȭ 
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

		// select()
		retval = select(0, &readSet, &writeSet, nullptr, nullptr);

	}


}