#include "defs.h"

// TODO: implement show_name & show_dns_msg
// msg = start of msg,
// ptr = ptr to name to print,
// end = one past end of msg to not read past end
const unsigned char *show_name(const unsigned char *msg,
				const unsigned char *ptr,
				const unsigned char *end);

// msg = start of msg
void show_dns_msg(const char *msg, int msg_len);

int main (int argc, char *argv[]) {

	if (argc < 3) {
		fprintf(stderr, "fatal error: no arguments\n expected: hostname type");
		exit(0);
	}

	if (strlen(argv[1] > 255)) {
		fprintf(stderr, "fatal error: hostname too long\n expected: max 255 chars");
		exit(1);
	}

	unsigned char query_type;
	if (strcmp(argv[2], "a") == 0) 
		query_type = 1;
	else if (strcmp(argv[2], "mx") == 0) 
		query_type = 15;
	else if (strcmp(argv[2], "txt") == 0) 
		query_type = 16;
	else if (strcmp(argv[2], "aaaa") == 0)
		query_type = 28;
	else if (strcmp(argv[2], "any") == 0)
		query_type = 255;
	else {
		fprintf(stderr, "fatal error: unknown type %s\n expected: a, aaaa, txt, mx or any", argv[2]);
		exit(1);
	}

#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2,2), &d)) {
		fprintf(stderr, "Init failed.\n");
		return 1;
	}
#endif

	printf("Configuring remote addr..");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	struct addrinfo peer_addr;
	if (getaddrinfo("1.1.1.1", "53", &hints, &peer_addr)) {
		fprintf(stderr, "Call to getaddrinfo() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	printf("Configuring socket..\n");
	SOCKET peer_sock;
	peer_sock = socket(peer_addr->ai_family,
		peer_addr->ai_socktype,
		peer_addr->ai_protocol);
	if (!ISVALIDSOCKET(peer_sock)) {
		fprintf(stderr, "Call to socket() failed. (%d)\n", GETSOCKETERR());
		return 1;
	}

	char query[1024] = {0xAB, 0xCD, // ID
                        0x01, 0x00, // Recursion is set
                        0x00, 0x01, // QDCOUNT 
                        0x00, 0x00, // ANCOUNT
                        0x00, 0x00, // NSCOUNT
                        0x00, 0x00}; // ARCOUNT

    char *ptr = query + 12;// set to end of query header
    char *h_ptr = argv[1];// loop through hostname arg

    // when *h_ptr = 0 hostname is fully read
    while (*h_ptr) {

    	// store pos of label start
    	// indicates the len of upcoming label
    	char *label_len = p;
    	p++;
    	if (h_ptr != argv[1]) 
    		++h;
    	
    	while (*h_ptr && *h_ptr != '.')
    		*p++ = *h++;

    	*label_len = p - label_len - 1;// set to new label
    }

    *ptr++ = 0;
    *ptr++ = 0x00; *ptr++ = query_type;// QTYPE
    *ptr++ = 0x00; *ptr++ = 0x01;// QCLASS

    // compare p to query beginning for size
    const int query_size = p - query;

    // TODO: implement select() to check
    // for timeouts and retry, since UDP isnt reliable
    // if this is not implemented, the query
    // might be lost in transit and the
    // program waits for a reply that never comes
    int bytes_sent = sento(peer_sock,
    	query,
    	query_size,
    	0,
    	peer_addr->ai_addr,
    	peer_addr->ai_addrlen);
    printf("%d bytes sent. \n", bytes_sent);

    // call show_dns_msg(query, query_size);

    char read[1024];
    int bytes_recv = recvfrom(peer_sock, read, 1024, 0, 0, 0);
    printf("%d bytes received.\n", bytes_recv);
    // call show_dns_msg(read, bytes_recv)
    // printf(\n);

    freeaddrinfo(peer_addr);
    CLOSESOCKET(peer_sock);

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}	

