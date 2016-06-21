#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <vector>

#define SERVERPORT	23452
#define BUFSIZE		512

void QuitWithError(std::wstring msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg.c_str(), MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void DisplayError(std::wstring msg)
{
	/*
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg.c_str(), (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);*/
}

void ProcessClient(SOCKET clientSocket)
{
	int retval;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(clientSocket, (SOCKADDR *)&clientaddr, &addrlen);

	char clientIP[32] = { 0, };
	inet_ntop(AF_INET, &(clientaddr.sin_addr), clientIP, 32 - 1);

	while (1)
	{
		// 데이터 받기
		retval = recv(clientSocket, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			DisplayError(L"recv() failed");
			break;
		}
		else if (retval == 0)
		{
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		printf("[TCP/%s:%d] %s\n", clientIP, ntohs(clientaddr.sin_port), buf);

		// 데이터 보내기
		retval = send(clientSocket, buf, retval, 0);
		if (retval == SOCKET_ERROR) {
			DisplayError(L"send() failed");
			break;
		}
	}

	// closesocket()
	closesocket(clientSocket);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", clientIP, ntohs(clientaddr.sin_port));
}

int main()
{

	int retval;

	// winsock initialization
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		QuitWithError(L"socket() failed");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
		QuitWithError(L"bind() failed");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		QuitWithError(L"listen() failed");

	// 데이터 통신에 사용할 변수
	SOCKET clientSocket;
	SOCKADDR_IN clientAddress;
	int addressLength;
	HANDLE hThread;

	while (1)
	{
		// accept()
		addressLength = sizeof(clientAddress);
		clientSocket = accept(listen_sock, (SOCKADDR*)&clientAddress, &addressLength);
		if (clientSocket == INVALID_SOCKET)
		{
			DisplayError(L"accept() failed");
			break;
		}

		// 접속 클라이언트의 정보 출력
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, 32 - 1);
		std::cout << std::endl << 
			"[TCP 서버] 클라이언트 접속: IP 주소=" << clientIP << 
			" 포트 번호=" << ntohs(clientAddress.sin_port) << std::endl;

	
		auto newThread = new std::thread(ProcessClient, clientSocket);
		if (newThread->joinable() == false)
			closesocket(clientSocket);
	}
	WSACleanup();
	return 0;
}