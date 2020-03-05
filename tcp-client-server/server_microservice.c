#include "defs.h"
#include <ctype.h>

int main () {

#if defined(_WIN32)
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
	hints.ai_socktype = SOCK_STREAM;// tcp
	hints.ai_flags = AI_PASSIVE;// bind wildcard, i.e listen on any viable interface

	// pass 0 (NULL) as first arg in conjunction with AI_PASSIVE flagged
	// in order to generate addr suitable for bind()
	struct addrinfo *bind_addr;
	getaddrinfo(0, "8080", &hints, &bind_addr);

	printf("Configuring socket..\n");
	SOCKET slisten = socket(bind_addr->ai_family,
		bind_addr->ai_socktype,
		bind_addr->ai_protocol);
	if (!ISVALIDSOCKET(slisten)) {
		fprintf(stderr, "Call to socket() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Binding socket to local addr..\n");
	if (bind(slisten, bind_addr->ai_addr, bind_addr->ai_addrlen)) {
		fprintf(stderr, "Call to bind() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}
	freeaddrinfo(bind_addr);

	printf("Listening..\n");
	int queue = 10;
	if (listen(slisten, queue) < 0) {
		fprintf(stderr, "Call to listen() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	fd_set master;// holds all active sockets
	FD_ZERO(&master);
	FD_SET(slisten, &master);
	SOCKET smax = slisten;// holds largest descriptor
	
	printf("Waiting for connections..\n");
	while (1) {
		fd_set sread;
		sread = master;
		// pass a timeout arg of 0 (NULL) to select()
		// to avoid returns until master set is ready to be read from
		if (select(smax + 1, &sread, 0, 0, 0) < 0) {
			fprintf(stderr, "Call to select() failed. %s\n", GETSOCKETERR());
			return 1;
		}

		SOCKET i;
		for (i = 1; i <= smax; ++i) {
			if (FD_ISSET(i, &sread)) {

				if (i == slisten) {
					struct sockaddr_storage client_addr;
					socklen_t client_size = sizeof(client_addr);
					SOCKET sclient = accept(slisten,
						(struct sockaddr*)&client_addr,
						&client_size);
					if (!ISVALIDSOCKET(sclient)) {
						fprintf(stderr, "Call to accept() failed. %s\n", GETSOCKETERR());
						return 1;
					}

					FD_SET(sclient, &master);
					if (sclient > smax) 
						smax = sclient;

					char addr_buffer[100];
					getnameinfo((struct sockaddr*)&client_addr,
						client_size,
						addr_buffer,
						sizeof(addr_buffer),
						0,
						0,
						NI_NUMERICHOST);
					printf("Connection from: %s\n", addr_buffer);
				}
				else {
					char read[1024];
					int bytes_recv = recv(i, read, 1024, 0);
					if (bytes_recv < 1) {
						FD_CLR(i, &master);
						CLOSESOCKET(i);
						continue;
					}

                    		int j;
                    		for (j = 0; j < bytes_recv; ++j) {
                        		if (j % 2 == 0) read[j] = toupper(read[j]);
                        		else read[j] = tolower(read[j]);
                    		}
                    		send(i, read, bytes_recv, 0);

				}
			}// FD_ISSET
		}// for i <= smax
	}// while (1)

	printf("Closing listening socket..\n");
	CLOSESOCKET(slisten);
#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Done.");
	return 0;
}
