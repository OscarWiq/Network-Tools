#include "defs.h"
#include <ctype.h>

int main () {

#if defined (_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2,2), &d)) {
		fprintf(stderr, "Init failed.\n");
		return 1;
	} 
#endif

	printf("Configuring local addr..\n");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;// ipv4
	hints.ai_socktype = SOCK_DGRAM;// UDP
	hints.ai_flags = AI_PASSIVE;// bind wildcard, i.e listen to any viable interface

	//generate suitable addr for bind()
	//with AI_PASSIVE & null passed as first arg
	struct addrinfo *bind_addr;
	getaddrinfo(0, "8080", &hints, &bind_addr);

	printf("Configuring socket..\n");
	SOCKET listen_sock = socket(bind_addr->ai_family,
		bind_addr->ai_socktype,
		bind_addr->ai_protocol);
	if (!ISVALIDSOCKET(listen_sock)) {
		fprintf(stderr, "Call to socket() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Binding socket to local addr..\n");
	if (bind(listen_sock, bind_addr->ai_addr, bind_addr->ai_addrlen)) {
		fprintf(stderr, "Call to bind() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}
	freeaddrinfo(bind_addr);

	fd_set(master);
	FD_ZERO(&master);
	FD_SET(listen_sock, &master);
	SOCKET max_sock = listen_sock;

	printf("Waiting for connections..\n");
	while (1) {

		fd_set sread;
		sread = master;
		if (select(max_sock + 1, &sread, 0, 0, 0) < 0) {
			fprintf(stderr, "Call to select() failed. (%d)\n", GETSOCKETERR());
			return 1;
		}

		if (FD_ISSET(listen_sock, &sread)) {
			struct sockaddr_storage client_addr;
			socklen_t client_size = sizeof(client_addr);

			char read[1024];
			int bytes_recv = recvfrom(listen_sock,
				read,
			 	1024,
			 	0,
			 	(struct sockaddr *)&client_addr,
			 	&client_size);
			if (bytes_recv < 1) {
				fprintf(stderr, "Connection closed. (%d)\n", GETSOCKETERR());
				return 1;
			}

			for (int j = 0; j < bytes_recv; ++j) {
				if (j % 2 == 0) read[j] = toupper(read[j]);
				else read[j] = tolower(read[j]); 
			}
			sendto(listen_sock,
				read,
				bytes_recv,
				0,
				(struct sockaddr *)&client_addr,
				client_size);
		}// FD_ISSET
	}// while (1)

	printf("Closing listening socket..\n");
	CLOSESOCKET(listen_sock);

#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Done.");

	return 0;
}
