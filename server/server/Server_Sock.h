//#pragma once
#ifndef _SERVER_SOCK_H
#define _SERVER_SOCK_H
#include<WinSock2.h>
#include<Windows.h>
#pragma comment (lib, "ws2_32.lib")

class Server_sock
{
public:
	//Server_sock();
	Server_sock(int port);
	~Server_sock();
	Server_sock(const Server_sock&) = delete;
	Server_sock &operator =(const Server_sock&) = delete;
	void WaitForClientConnect();
	static DWORD WINAPI CreateClientThread(LPVOID lpParameter);


private:
	WORD winsock_ver;
	WSADATA wsa_data;
	SOCKET sock_svr;
	SOCKET sock_clt;
	HANDLE h_thread;
	SOCKADDR_IN addr_svr;
	SOCKADDR_IN addr_clt;
	int sock_port;
	int addr_len;

};
#endif // !_SERVER_SOCK_H
