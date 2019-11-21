// TODO: Clean this code that is vastly a copy paste of https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/netds/winsock/simple/client/simplec.cpp

#pragma comment(lib, "Ws2_32.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strsafe.h>

#define DEFAULT_PORT        "36150"   // Default server port
#define DEFAULT_PAYLOAD     "reload"  // Message sent to ReShade instance

//
// Function: Usage
// 
// Description:
//    Print the parameters and exit.
//
void Usage(char *progname)
{
	fprintf(stderr, "Usage\n%s -p [protocol] -n [server] -e [endpoint] -l [iterations] [-4] [-6]\n"
		"Where:\n"
		"\t-n server     - is the string address or name of server\n"
		"\t-p port       - is the port to connect to\n"
		"\t-d data       - is the payload sent to ReShade instance\n"
		"\n"
		"Defaults are localhost, 36150 and 'reload'\n",
		progname
	);
	WSACleanup();
	exit(1);
}


int __cdecl main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET conn_socket = INVALID_SOCKET;
	struct addrinfo
		*results = NULL,
		*addrptr = NULL,
		hints;
	const char
		*server_name = "localhost",
		*port = DEFAULT_PORT,
		*payload = DEFAULT_PAYLOAD;
	char
		hoststr[NI_MAXHOST],
		servstr[NI_MAXSERV];
	int
		retval,
		loopflag = 0,
		maxloop = -1,
		i;

	// Parse the command line
	if (argc >1)
	{
		for (i = 1; i < argc; i++)
		{
			if ((strlen(argv[i]) == 2) && ((argv[i][0] == '-') || (argv[i][0] == '/')))
			{
				switch (tolower(argv[i][1]))
				{
				case 'n':
					server_name = argv[++i];
					break;

				case 'p':
					port = argv[++i];
					break;

				case 'd':
					payload = argv[++i];
					break;

				default:
					Usage(argv[0]);
					break;
				}
			}
			else
				Usage(argv[0]);
		}
	}

	// Load Winsock
	if ((retval = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		fprintf(stderr, "WSAStartup failed with error %d\n", retval);
		WSACleanup();
		return -1;
	}

	// Make sure the wildcard port wasn't specified
	if (_strnicmp(port, "0", 1) == 0)
		Usage(argv[0]);

	//
	// Resolve the server name
	//
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	retval = getaddrinfo(server_name, port, &hints, &results);
	if (retval != 0)
	{
		fprintf(stderr, "getaddrinfo failed: %d\n", retval);
		goto cleanup;
	}

	// Make sure we got at least one address
	if (results == NULL)
	{
		fprintf(stderr, "Server (%s) name could not be resolved!\n", server_name);
		goto cleanup;
	}

	//
	// Walk through the list of addresses returned and connect to each one.
	//    Take the first successful connection.
	//
	addrptr = results;
	while (addrptr)
	{
		conn_socket = socket(addrptr->ai_family, addrptr->ai_socktype, addrptr->ai_protocol);
		if (conn_socket == INVALID_SOCKET)
		{
			fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
			goto cleanup;
		}

		//
		// Notice that nothing in this code is specific to whether we 
		// are using UDP or TCP.
		// We achieve this by using a simple trick.
		//    When connect() is called on a datagram socket, it does not 
		//    actually establish the connection as a stream (TCP) socket
		//    would. Instead, TCP/IP establishes the remote half of the
		//    ( LocalIPAddress, LocalPort, RemoteIP, RemotePort) mapping.
		//    This enables us to use send() and recv() on datagram sockets,
		//    instead of recvfrom() and sendto()

		retval = getnameinfo(
			addrptr->ai_addr,
			(socklen_t)addrptr->ai_addrlen,
			hoststr,
			NI_MAXHOST,
			servstr,
			NI_MAXSERV,
			NI_NUMERICHOST | NI_NUMERICSERV
		);
		if (retval != 0)
		{
			fprintf(stderr, "getnameinfo failed: %d\n", retval);
			goto cleanup;
		}

		printf("Client attempting connection to: %s port: %s\n", hoststr, servstr);

		retval = connect(conn_socket, addrptr->ai_addr, (int)addrptr->ai_addrlen);
		if (retval == SOCKET_ERROR)
		{
			closesocket(conn_socket);
			conn_socket = INVALID_SOCKET;

			addrptr = addrptr->ai_next;
		}
		else
		{
			break;
		}
	}

	freeaddrinfo(results);
	results = NULL;

	// Make sure we got a connection established
	if (conn_socket == INVALID_SOCKET)
	{
		printf("Unable to establish connection...\n");
		goto cleanup;
	}
	else
	{
		printf("Connection established...\n");
	}

	//
	// Send the data
	//
	retval = send(conn_socket, payload, lstrlen(payload) + 1, 0);
	if (retval == SOCKET_ERROR)
	{
		fprintf(stderr, "send failed: error %d\n", WSAGetLastError());
		goto cleanup;
	}

cleanup:

	//
	// clean up the client connection
	//

	if (conn_socket != INVALID_SOCKET)
	{
		// Indicate no more data to send
		retval = shutdown(conn_socket, SD_SEND);
		if (retval == SOCKET_ERROR)
		{
			fprintf(stderr, "shutdown failed: %d\n", WSAGetLastError());
		}

		// Close the socket
		retval = closesocket(conn_socket);
		if (retval == SOCKET_ERROR)
		{
			fprintf(stderr, "closesocket failed: %d\n", WSAGetLastError());
		}

		conn_socket = INVALID_SOCKET;
	}

	if (results != NULL)
	{
		freeaddrinfo(results);
		results = NULL;
	}

	WSACleanup();

	return 0;
}
