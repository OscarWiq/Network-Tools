#include "defs.h"

const unsigned char *show_name(const unsigned char *msg,
				const unsigned char *ptr,
				const unsigned char *end);

void show_dns_msg(const char *dns_msg, int message_len);

int main (int argc, char *argv[]) {

	if (argc < 3) {
		fprintf(stderr, "fatal error: no arguments\n expected: hostname query-type");
		exit(0);
	}

	if (strlen(argv[1]) > 255) {
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

	printf("Configuring remote addr..\n");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	struct addrinfo *peer_addr;
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
    	char *label_len = ptr;
    	ptr++;
    	if (h_ptr != argv[1]) 
    		++h_ptr;
    	
    	while (*h_ptr && *h_ptr != '.')
    		*ptr++ = *h_ptr++;

    	*label_len = ptr - label_len - 1;// set to new label
    }

    *ptr++ = 0;
    *ptr++ = 0x00; *ptr++ = query_type;// QTYPE
    *ptr++ = 0x00; *ptr++ = 0x01;// QCLASS

    // compare p to query beginning for size
    const int query_size = ptr - query;

    // TODO: implement select() to check
    // for timeouts and retry, since UDP isnt reliable
    // if this is not implemented, the query
    // might be lost in transit and the
    // program waits for a reply that never comes
    int bytes_sent = sendto(peer_sock,
    	query,
    	query_size,
    	0,
    	peer_addr->ai_addr,
    	peer_addr->ai_addrlen);
    printf("%d bytes sent. \n", bytes_sent);

    show_dns_msg(query, query_size);

    char read[1024];
    int bytes_recv = recvfrom(peer_sock, read, 1024, 0, 0, 0);
    printf("%d bytes received.\n", bytes_recv);
    show_dns_msg(read, bytes_recv);
    printf("\n");

    freeaddrinfo(peer_addr);
    CLOSESOCKET(peer_sock);

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}	

const unsigned char *show_name(const unsigned char *msg,
				const unsigned char *ptr,
				const unsigned char *end) {

	if (ptr + 2 > end) {
		fprintf(stderr, "end of msg.\n");
		exit(1);
	}

	// 0xC0 is 0b11000000
	if ((*ptr & 0xC0) == 0xC0) {// check for name ptr
		// take lower 6 bits of *ptr and all 8 bits of ptr[1] to indicate the ptr
		const int k = ((*ptr & 0x3F) << 8) + ptr[1];
		ptr += 2;
		printf("ptr (%d)", k);
		// we know that ptr[1] is still within the msg
		// because of the of the earlier check that
		// ptr was at least 2 bytes from the end
		show_name(msg, msg + k, end);// we now know the name ptr, pass new value
		return ptr;
	}
	else {
		// if name is not a ptr, print one label at a time
		const int label_len = *ptr++;// store len of curr label
		if (ptr + label_len + 1 > end) { // make sure within buffer bounds
			fprintf(stderr, "end of msg.\n");
			exit(1);
		}

		printf("%.*s", label_len, ptr);
		ptr += label_len;
		if (*ptr) {// if next byte != 0
			printf(".");
			return show_name(msg, ptr, end);
		}
		else {
			return ptr + 1;
		}
	}
}

void show_dns_msg(const char *dns_msg, int msg_len) {

	if (msg_len < 12) {
		fprintf(stderr, "msg too short.\n");
		exit(1);
	}

	const unsigned char *msg = (const unsigned char *)dns_msg;

	// print raw message
	//for (int i = 0; i < msg_len; ++i) {
	//	unsigned char r = msg[i];
	//	printf("%02d:  %02X %03d  '%c'\n", i, r, r, r);
	//}
	//printf("\n");

	printf("ID: %0X %0X\n", msg[0], msg[1]);

	// qr is the most significant bit of msg[2]
	// check if it is set and if it is then
	// we know that msg is a response and not a query
	const int qr_bit = (msg[2] & 0x80) >> 7;
	printf("QR: %d %s\n", qr_bit, qr_bit ? "response" : "query");

	const int opcode = (msg[2] & 0x78) >> 3;
	printf("OPCODE: %d ", opcode);
	switch (opcode) {
		
		case 0:
			printf("standard\n");
			break;
		case 1:
			printf("reverse\n");
			break;
		case 2:
			printf("status\n");
			break;
		default:
			printf("unknown");
			break;
	}

	const int aa = (msg[2] & 0x04) >> 2;
	printf("AA: %d %s\n", aa, aa ? "authoritative" : "");

	const int tc = (msg[2] & 0x02) >> 1;
	printf("TC: %d %s\n", tc, tc ? "message truncated" : "");

	const int rd = (msg[2] & 0x01);
	printf("RD: %d %s\n", rd, rd ? "recursion requested" : "");

	if (qr_bit) {
		const int rcode = msg[3] & 0x07;
		printf("RCODE: %d", rcode);
		switch (rcode) {
			case 0:
				printf("success\n");
				break;
			case 1:
				printf("format error\n");
				break;
			case 2:
				printf("server failure\n");
				break;
			case 3:
				printf("name error\n");
				break;
			case 4:
				printf("not implemented\n");
				break;
			case 5:
				printf("refused\n");
				break;
			default:
				printf("unknown\n");
				break;
		}

		if (rcode != 0)
			return;
	}

	const int qdcount = (msg[4] << 8) + msg[5];
	printf("QDCOUNT: %d\n", qdcount);

	const int ancount = (msg[6] << 8) + msg[7];
	printf("ANCOUNT: %d\n", ancount);

	const int nscount = (msg[8] << 8) + msg[9];
	printf("NSCOUNT: %d\n", nscount);

	const int arcount = (msg[10] << 8) + msg[11];
	printf("ARCOUNT: %d\n", arcount);

	const unsigned char *ptr = msg + 12;
	const unsigned char *end = msg + msg_len;

	if (qdcount) {

		for (int i = 0; i < qdcount; ++i) {
			if (ptr >= end) {
				fprintf(stderr, "end of msg.\n");
				exit(1);
			}

			printf("QUERY %2d\n", i + 1);
			printf("  name:  ");
			ptr = show_name(msg, ptr, end);
			printf("\n");

			if (ptr + 4 > end) {
				fprintf(stderr, "end of msg.\n");
				exit(1);
			}

			const int query_type = (ptr[0] << 8) + ptr[1];
			printf("  type: %d\n", query_type);
			ptr += 2;

			const int qclass = (ptr[0] << 8) + ptr[1];
			printf(" class: %d\n", qclass);
			ptr += 2;
		}
	}

	if (nscount || arcount || ancount) {

		for (int i = 0; i < nscount + arcount + ancount; ++i) {
			if (ptr >= end) {
				fprintf(stderr, "end of msg.\n");
				exit(1);
			}

			printf("Answer: %2d\n", i + 1);
			printf("  name:  \n");
			ptr = show_name(msg, ptr, end);
			printf("\n");

			if (ptr + 10 > end) {
				fprintf(stderr, "end of msg.\n");
				exit(1);
			}

			const int query_type = (ptr[0] << 8) + ptr[1];
			printf("  type: \n", query_type);
			ptr += 2;

			const int qclass = (ptr[0] << 8) + ptr[1];
			printf(" class: %d\n", qclass);
			ptr += 2;

			const unsigned int ttl = (ptr[0] << 24 + (ptr[1] << 16)) + (ptr[2] << 8) + ptr[3];
			printf("   ttl:\n", ttl);
			ptr += 4;

			const int rd_len = (ptr[0] << 8) + ptr[1];
			printf(" rdlen: %d\n", rd_len);
			ptr += 2;

			if (ptr + rd_len > end) {
				fprintf(stderr, "end of msg.\n");
				exit(1);
			}

			// A
			if (rd_len == 4 && query_type == 1) {
				printf("Address: ");
				printf("%d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
			}
			// AAAA
			else if (rd_len == 16 && query_type == 28) {
				printf("Adress: ");
				for (int j = 0; j < rd_len; j += 2) {
					printf("%02x%02x", ptr[j], ptr[j + 1] );
					if (j + 2 < rd_len)
						printf(":");
				}
				printf("\n");
			}
			// MX
			else if (query_type == 15 && rd_len > 3) {
				const int pref = (ptr[0] << 8) + ptr[1];
				printf("  pref: %d\n", pref);
				printf("MX: ");
				show_name(msg, ptr + 2, end);
				printf("\n");
			}
			// CNAME
			else if (query_type == 5) {
				printf("CNAME: ");
				show_name(msg, ptr, end);
				printf("\n");
			}

			ptr += rd_len;
		}
	}

	if (ptr != end)
		printf("__unread data exists__\n");
	printf("\n");
}
