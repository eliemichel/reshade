/**
* Copyright (C) 2019 Elie Michel. All rights reserved.
* License: https://github.com/crosire/reshade#license
*/

#if RESHADE_REMOTE

#pragma comment(lib, "Ws2_32.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include "log.hpp"
#include "runtime.hpp"

#define BUFFER_LEN 1024

DWORD WINAPI reshade::runtime::server_thread(LPVOID lpParam)
{
	reshade::runtime *runtime = (reshade::runtime*)lpParam;
	SOCKET listen_socket = runtime->_listen_socket;
	SOCKET client_socket = INVALID_SOCKET;
	char Buffer[BUFFER_LEN];
	int result, bytecount;

	for (;;)
	{
		if (client_socket == INVALID_SOCKET) {
			// Accept connections (move this in a handle_remote_calls?)
			client_socket = accept(listen_socket, NULL, NULL);
			if (client_socket == INVALID_SOCKET) {
				LOG(ERROR) << "accept failed with error: " << WSAGetLastError();
				goto cleanup;
			}
		}
		else
		{
			bytecount = recv(client_socket, Buffer, BUFFER_LEN, 0);
			if (bytecount == SOCKET_ERROR)
			{
				LOG(ERROR) << "recv failed with error: " << WSAGetLastError();
				goto cleanup;
			}
			else if (bytecount == 0)
			{
				// Client connection was closed
				result = shutdown(client_socket, SD_SEND);
				if (result == SOCKET_ERROR)
				{
					LOG(ERROR) << "shutdown failed with error: " << WSAGetLastError();
					goto cleanup;
				}

				closesocket(client_socket);
				client_socket = INVALID_SOCKET;
			}
			else
			{
				LOG(INFO) << "read " << bytecount << " bytes";
				// TODO
			}
		}
	}

cleanup:
	if (client_socket != INVALID_SOCKET) {
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
	}
	closesocket(listen_socket);
	WSACleanup();

	return 0;
}

void reshade::runtime::init_remote()
{
	int result;

	// Initialize Winsock
	result = WSAStartup(MAKEWORD(2, 2), &_wsa_data);
	if (result != 0) {
		LOG(ERROR) << "WSAStartup failed with error: " << result;
		return;
	}

	// Get socket address
	struct addrinfo *addr = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	result = getaddrinfo(NULL, _port.c_str(), &hints, &addr);
	if (result != 0) {
		LOG(ERROR) << "getaddrinfo failed with error: " << result;
		WSACleanup();
		return;
	}

	// Create socket
	_listen_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (_listen_socket == INVALID_SOCKET) {
		LOG(ERROR) << "socket failed with error: " << WSAGetLastError();
		freeaddrinfo(addr);
		WSACleanup();
		return;
	}

	// Bind socket
	result = bind(_listen_socket, addr->ai_addr, (int)addr->ai_addrlen);
	if (result == SOCKET_ERROR) {
		LOG(ERROR) << "bind failed with error: " << WSAGetLastError();
		freeaddrinfo(addr);
		closesocket(_listen_socket);
		WSACleanup();
		return;
	}
	freeaddrinfo(addr);

	// Listening on clients
	result = listen(_listen_socket, SOMAXCONN);
	if (result == SOCKET_ERROR) {
		LOG(ERROR) << "listen failed with error: " << WSAGetLastError();
		closesocket(_listen_socket);
		WSACleanup();
		return;
	}

	// Start socket thread
	_server_thread = CreateThread(NULL, 0, server_thread, (LPVOID)this, 0, NULL);

	_remote_initialized = true;
}
void reshade::runtime::deinit_remote()
{
	if (!_remote_initialized) return;

	WaitForSingleObject(_server_thread, INFINITE);

	_remote_initialized = false;
}
void reshade::runtime::handle_remote_calls()
{

}

#endif // RESHADE_REMOTE
