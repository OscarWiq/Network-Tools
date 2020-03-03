#include "defs.h"

#if defined(_WIN32)
#include <conio.h>
#endif

int main (int argc, char *argv[]) {

#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2, 2), &d)) {
		fprintf(stderr, "Init failed.\n");
		return 1;
	}
#endif

	if (argc < 3) {
		fprintf(stderr, "fatal error: no arguments\n expected: hostname port\n");
		return 1;
	}

	printf("Configuring remote addr..\n");
	struct addrinfo addr_info;
	memset(&addr_info, 0, sizeof(addr_info));
	addr_info.ai_socktype = SOCK_STREAM;
	struct addrinfo *peer_addr;
	if (getaddrinfo(argv[1], argv[2], &addr_info, &peer_addr)) {
		fprintf(stderr, "Call to getaddrinfo failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Remote addr: ");
	char addr_buffer[100], protocol_buffer[100];
	getnameinfo(peer_addr->ai_addr,
		peer_addr->ai_addrlen,
		addr_buffer,
	  	sizeof(addr_buffer),
	  	protocol_buffer,
	   	sizeof(protocol_buffer),
	   	NI_NUMERICHOST);
	printf("%s %s\n", addr_buffer, protocol_buffer);

	printf("Configuring socket..\n");
	SOCKET peer = socket(peer_addr->ai_family,
		peer_addr->ai_socktype,
		peer_addr->ai_protocol);
	if (!ISVALIDSOCKET(peer)) {
		fprintf(stderr, "Call to socket() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Connecting..\n");
	if (connect(peer, peer_addr->ai_addr, peer_addr->ai_addrlen)) {
		fprintf(stderr, "Call to connect() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}
	freeaddrinfo(peer_addr);

	printf("Connected.\n");
	printf("Send data: (text followed by enter). \n");
	while (1) {

		fd_set sread;
		FD_ZERO(&sread);
		FD_SET(peer, &sread);
#if !defined(_WIN32)
		//add stdin to the sread set, works bc 0 is the file descriptor for stdin
		//FD_SET(fileno(stdin), &sread) would accomplish the same feat
		FD_SET(0, &sread);
#endif

		//set up timeout for select()
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000; //100 milliseconds

		if (select(peer + 1, &sread, 0, 0, &timeout) < 0) {
			fprintf(stderr, "Call to select() failed. (%d)\n", GETSOCKETERR());
			return 1;
		}

		if (FD_ISSET(peer, &sread)) {
			char read[4096];
			int bytes_recv = recv(peer, read, 409, 0);
			if (bytes_recv < 1) {
				printf("Connection closed by peer.\n");
				break;
			}
			printf("Received (%d bytes): %.*s", bytes_recv, bytes_recv, read);
		}
#if defined(_WIN32)
		if (_khbit()) {
#else
		if (FD_ISSET(0, &sread)) {
#endif		
			// In order to send a request, example:
			// GET / HTTP/1.1
			// Host: example.com (followed by another enter)
			char read[4096];
			if (!fgets(read, 4096, stdin)) break;
			printf("Sending: %s", read);
			int bytes_sent = send(peer, read, strlen(read), 0);
			printf("%d bytes sent.\n", bytes_sent);
		}	
	} //end of while(1)

	printf("Closing socket..\n");
	CLOSESOCKET(peer);

#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Done.");
	return 0;
}