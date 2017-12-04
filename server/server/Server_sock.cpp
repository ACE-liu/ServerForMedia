#include"stdafx.h"
#include "Server_sock.h"
#include <iostream>
//#include <WS2tcpip.h>


#define MSG_BUF_SIZE 1024

Server_sock::Server_sock(int port)
{
	int ret;
	sock_port = port;
	winsock_ver = MAKEWORD(2, 2);
	addr_len = sizeof(SOCKADDR_IN);
	addr_svr.sin_family = AF_INET;
	addr_svr.sin_port = htons(sock_port);
	addr_svr.sin_addr.S_un.S_addr = ADDR_ANY;
	ret = WSAStartup(winsock_ver, &wsa_data);
	if (ret != 0)
	{
		//cerr << "WSA failed to start up!Error code: " << ::WSAGetLastError() << "\n";
		fprintf(stderr, "WSA failed to start up!Error code: %d\n", WSAGetLastError());
		system("pause");
		exit(1);
	}
	sock_svr =socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_svr == INVALID_SOCKET)
	{
		fprintf(stderr, "Failed to create server socket!Error code: %d\n", WSAGetLastError());
		WSACleanup();
		system("pause");
		exit(1);
	}
	ret = bind(sock_svr, (SOCKADDR*)&addr_svr, addr_len);
	if (ret != 0)
	{
		fprintf(stderr, "Failed to bind server socket!Error code:  %d\n", WSAGetLastError());
		WSACleanup();
		system("pause");
		exit(1);
	}
	//
	ret = listen(sock_svr, SOMAXCONN);
	if (ret == SOCKET_ERROR)
	{
		fprintf(stderr, "Server socket failed to listen!Error code:  %d\n", WSAGetLastError());
		WSACleanup();
		system("pause");
		exit(1);
	}
}

Server_sock::~Server_sock()
{
	closesocket(sock_svr);
	closesocket(sock_clt);
	WSACleanup();
	//cout << "Socket closed..." << endl;
	fprintf(stdout, "Socket closed...\n");
}
//
void Server_sock::WaitForClientConnect()
{
	while (true)
	{
		sock_clt = accept(sock_svr, (SOCKADDR*)&addr_clt, &addr_len);
		if (sock_clt == INVALID_SOCKET)
		{
			fprintf(stderr, "Failed to accept client!Error code: %d\n", WSAGetLastError());
			WSACleanup();
			system("pause");
			exit(1);
		}
		h_thread = CreateThread(NULL, 0, CreateClientThread, (LPVOID)sock_clt, 0, NULL);
		if (h_thread == NULL)
		{
			fprintf(stderr, "Failed to create a new thread.. Error code: %d\n", WSAGetLastError());
			WSACleanup();
			system("pause");
			exit(1);
		}
		CloseHandle(h_thread);
	}
}

DWORD WINAPI Server_sock::CreateClientThread(LPVOID lpParameter)
{
	SOCKET sock_clt = (SOCKET)lpParameter;
	char buf_msg[MSG_BUF_SIZE];
	int ret_val = 0;
	int snd_result = 0;
	do
	{
		memset(buf_msg, 0, MSG_BUF_SIZE);
		ret_val = recv(sock_clt, buf_msg, MSG_BUF_SIZE, 0);
		if (ret_val > 0)
		{
			if (strcmp(buf_msg, "exit") == 0)
			{
				//cout << "Client requests to close the connection..." << endl;
				break;
			}
			//cout << "Message received: " << buf_msg << endl;
			snd_result = send(sock_clt, buf_msg, MSG_BUF_SIZE, 0);
			if (snd_result == SOCKET_ERROR)
			{
				//cerr << "Failed to send message to client!Error code: " << ::GetLastError() << "\n";
				closesocket(sock_clt);
				system("pause");
				return 1;
			}
		}
		else if (ret_val == 0)
		{
			//cout << "connection closed..." << endl;
		}
		else
		{
			closesocket(sock_clt);
			system("pause");
			return 1;
		}
	} while (ret_val > 0);
	//
	ret_val = shutdown(sock_clt, SD_SEND);
	if (ret_val == SOCKET_ERROR)
	{
		closesocket(sock_clt);
		system("pause");
		return 1;
	}
	return 0;
}