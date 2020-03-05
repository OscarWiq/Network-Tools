#include "defs.h"

#ifndef AI_ALL
#define AI_ALL 0x0100
#endif

int main (int argc, char *argv[]) {

	if (argc < 2) {
		fprintf(stderr, "fatal error: no arguments\n expected: hostname\n");
		return 1;
	}

#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2,2), &d)) {
		fprintf(stderr, "Init failed.\n");
		return 1;
	}
#endif

	printf("Resolving hostname: '%s'\n", argv[1]);
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_ALL;
	struct addrinfo *peer_addr;
	if (getaddrinfo(argv[1], 0, &hints, &peer_addr)) {
		fprintf(stderr, "Call to getaddrinfo failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Remote addr is: \n");
	struct addrinfo *addr = peer_addr;
	do {
		char addr_buffer[100];
		getnameinfo(addr->ai_addr,
			addr->ai_addrlen,
			addr_buffer,
			sizeof(addr_buffer),
			0,
			0,
			NI_NUMERICHOST);
		printf("\t%s\n", addr_buffer);
	} while ((addr = addr->ai_next));

	freeaddrinfo(peer_addr);

#if defined(_WIN32)
	WSACleanup();
#endif

	return 0;
}