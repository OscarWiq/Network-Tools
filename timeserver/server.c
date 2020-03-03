#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <ws2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#endif

#if defined(_WIN32)
#define GETSOCKETERR() (WSAGetLastError())
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)

#if !defined(IPV6_V6ONLY)
#define IPV6_V6ONLY 27
#endif

#else
#define GETSOCKETERR() (errno)
#define SOCKET int
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {

#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2, 2), &d)) {
		fprintf(stderr, "Init failed. \n");
		return 1;
	}
#endif

	printf("Configuring local addr..\n");
	struct addrinfo addr_info, *bind_addr;
	memset(&addr_info, 0, sizeof(addr_info));
	addr_info.ai_family = AF_INET6;
	addr_info.ai_socktype = SOCK_STREAM;
	addr_info.ai_flags = AI_PASSIVE;
	getaddrinfo(0, "8080", &addr_info, &bind_addr);

	printf("Configuring socket..\n");
	SOCKET slisten;
	slisten = socket(bind_addr->ai_family, bind_addr->ai_socktype, bind_addr->ai_protocol);
	if (!ISVALIDSOCKET(slisten)) {
		fprintf(stderr, "Call to socket() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	int opt = 0;
	if (setsockopt(slisten, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&opt, sizeof(opt))) {
		fprintf(stderr, "Call to setsockopt() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Binding socket to local addr..\n");
	if (bind(slisten, bind_addr->ai_addr, bind_addr->ai_addrlen)) {
		fprintf(stderr, "Call to bind() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}
	freeaddrinfo(bind_addr);

	printf("Listening..\n");
	if (listen(slisten, 10) < 0) {
		fprintf(stderr, "Call to listen() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Waiting for client..\n");
	struct sockaddr_storage client_addr;
	socklen_t client_size = sizeof(client_addr);
	SOCKET sclient = accept(slisten, (struct sockaddr*)&client_addr, &client_size);
	if (!ISVALIDSOCKET(sclient)) {
		fprintf(stderr, "Call to accept() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Client connected..\n");
	char buffer[100];
	getnameinfo((struct sockaddr*)&client_addr, client_size, buffer, sizeof(buffer), 0, 0, NI_NUMERICHOST);
	printf("%s\n", buffer);

	printf("Reading request..\n");
	char req[1024];
	int bytes_recv = recv(sclient, req, 1024, 0);
	printf("%d bytes received.\n", bytes_recv);

	printf("Sending response..\n");
	const char *res = 
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-type: text/plain\r\n\r\n"
		"Local time: ";
	int bytes_sent = send(sclient, res, strlen(res), 0);
	printf("%d of %d bytes sent.\n", bytes_sent, (int)strlen(res));

	time_t t;
	time(&t);
	char *t_msg = ctime(&t);
	bytes_sent = send(sclient, t_msg, strlen(t_msg), 0);
	printf("%d of %d bytes sent.\n", bytes_sent, (int)strlen(t_msg));


	printf("Closing connection..\n");
	CLOSESOCKET(sclient);

	printf("Closing socket..\n");
	CLOSESOCKET(slisten);



#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Done.\n");
	return 0;
}