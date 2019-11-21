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
	struct addrinfo *addr = NULL, *ptr = NULL, hints;
	SOCKET listen_socket = INVALID_SOCKET;
	SOCKET client_socket = INVALID_SOCKET;
	char Buffer[BUFFER_LEN];
	int result, bytecount;

	// Initialize Winsock
	result = WSAStartup(MAKEWORD(2, 2), &runtime->_wsa_data);
	if (result != 0) {
		LOG(ERROR) << "WSAStartup failed with error: " << result;
		goto cleanup;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	result = getaddrinfo(NULL, runtime->_port.c_str(), &hints, &addr);
	if (result != 0) {
		LOG(ERROR) << "getaddrinfo failed with error: " << result;
		goto cleanup;
	}

	// Create socket
	listen_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (listen_socket == INVALID_SOCKET) {
		LOG(ERROR) << "socket failed with error: " << WSAGetLastError();
		goto cleanup;
	}

	// Force rebind
	BOOL bOptVal = TRUE;
	result = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, sizeof(int));
	if (result == SOCKET_ERROR) {
		LOG(ERROR) << "setsockopt failed with error: " << WSAGetLastError();
		goto cleanup;
	}

	// Bind socket
	result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);
	if (result == SOCKET_ERROR) {
		LOG(ERROR) << "bind failed with error: " << WSAGetLastError();
		goto cleanup;
	}
	freeaddrinfo(addr);
	addr = NULL;

	// Listening on clients
	result = listen(listen_socket, SOMAXCONN);
	if (result == SOCKET_ERROR) {
		LOG(ERROR) << "listen failed with error: " << WSAGetLastError();
		goto cleanup;
	}


	while (runtime->_server_running)
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
				std::string string_msg(Buffer, bytecount);
				Message msg = NOOP_MSG;

				// Parse message
				// (there is only one message type at the moment, but this may get more complicated eventually)
				if (!_strnicmp(Buffer, "reload", bytecount)) {
					msg = RELOAD_MSG;
				}
				else
				{
					LOG(ERROR) << "Unable to parse message: '" << Buffer << "'";
				}

				// Enqueue message for main thread
				if (msg != NOOP_MSG) {
					std::lock_guard<std::mutex> lock(runtime->_message_queue_mutex);
					runtime->_message_queue.push(msg);
					LOG(INFO) << "Enqueued message: '" << Buffer << "' (in runtime @" << runtime << ")";
				}
			}
		}
	}

cleanup:
	if (listen_socket != INVALID_SOCKET) {
		closesocket(listen_socket);
		listen_socket = INVALID_SOCKET;
	}
	if (client_socket != INVALID_SOCKET) {
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
	}
	if (NULL != addr) {
		freeaddrinfo(addr);
		addr = NULL;
	}
	WSACleanup();

	return 0;
}

void reshade::runtime::init_remote()
{
	LOG(INFO) << "Init remote control in runtime @" << this;

	_server_thread = CreateThread(NULL, 0, server_thread, (LPVOID)this, 0, NULL);
}
void reshade::runtime::deinit_remote()
{
	LOG(INFO) << "Deinit remote control in runtime @" << this;

	_server_running = false;
	WaitForSingleObject(_server_thread, INFINITE);

	_remote_initialized = false;
}
void reshade::runtime::handle_remote_calls()
{
	std::unique_lock<std::mutex> lock(_message_queue_mutex, std::try_to_lock);
	if (lock.owns_lock()) {
		while (!_message_queue.empty()) {
			Message msg = _message_queue.front();
			_message_queue.pop();

			switch (msg) {
			case RELOAD_MSG:
				LOG(INFO) << "[Remote] Reloading shaders";
				load_effects();
			default:
				LOG(ERROR) << "[Remote] Unknown message: #" << msg;
			}
		}
	}
	else {
		LOG(INFO) << "Could not lock message queue mutex";
	}
}

#endif // RESHADE_REMOTE
