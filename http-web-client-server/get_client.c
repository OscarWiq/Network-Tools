#include "defs.h"

#define TIMEOUT 5.0

SOCKET connect_to_host(char *hostname, char *port);
void parse_url(char *url, char **hostname, char **port, char** path);
void send_request(SOCKET sock, char *hostname, char *port, char *path);

int main (int argc, char *argv[]) {
		
#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2, 2), &d)) {
		fprintf(stderr, "Init failed.\n");
		return 1;
	}
#endif

	if (argc < 2) {
		fprintf(stderr, "fatal error: no arguments\n expected: url\n");
		exit(0);
	}
	char *url = argv[1];

	char *hostname, *port, *path;
	parse_url(url, &hostname, &port, &path);

	SOCKET server_sock = connect_to_host(hostname, port);
	send_request(server_sock, hostname, port, path);

	const clock_t start = clock();

#define RESPONSE_SIZE 32768
	char res[RESPONSE_SIZE + 1];
	char *s_ptr;
	char *body = 0;
	char *ptr = res;
	char *end = res + RESPONSE_SIZE;
	
	//method types
	enum {length, chunked, connection};
	int encoding = 0;
	int remaining = 0;

    while(1) {

        if ((clock() - start) / CLOCKS_PER_SEC > TIMEOUT) {
            fprintf(stderr, "timeout after %.2f seconds\n", TIMEOUT);
            return 1;
        }

        if (ptr == end) {
            fprintf(stderr, "out of buffer space\n");
            return 1;
        }

        fd_set sread;
        FD_ZERO(&sread);
        FD_SET(server_sock, &sread);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        if (select(server_sock + 1, &sread, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERR());
            return 1;
        }

        if (FD_ISSET(server_sock, &sread)) {
            int bytes_recv = recv(server_sock, ptr, end - ptr, 0);
            if (bytes_recv < 1) {
                if (encoding == connection && body) {
                    printf("%.*s", (int)(end - body), body);
                }

                printf("\nConnection closed by peer.\n");
                break;
            }

            /*printf("Received (%d bytes): '%.*s'",
                    bytes_received, bytes_received, p);*/

            ptr += bytes_recv;
            *ptr = 0;

            if (!body && (body = strstr(res, "\r\n\r\n"))) {
                *body = 0;
                body += 4;

                printf("Headers received:\n%s\n", res);

                s_ptr = strstr(res, "\nContent-Length: ");
                if (s_ptr) {
                    encoding = length;
                    s_ptr = strchr(s_ptr, ' ');
                    s_ptr += 1;
                    remaining = strtol(s_ptr, 0, 10);

                } 
                else {
                    s_ptr = strstr(res, "\nTransfer-Encoding: chunked");
                    if (s_ptr) {
                        encoding = chunked;
                        remaining = 0;
                    } 
                    else {
                        encoding = connection;
                    }
                }
                printf("\nBody received: \n");
            }

            if (body) {
                if (encoding == length) {
                    if (ptr - body >= remaining) {
                        printf("%.*s", remaining, body);
                        break;
                    }
                } 
                else if (encoding == chunked) {
                    do {
                        if (remaining == 0) {
                            if ((s_ptr = strstr(body, "\r\n"))) {
                                remaining = strtol(body, 0, 16);
                                if (!remaining) {
                                    goto finish;
                                }

                                body = s_ptr + 2;
                            } 
                            else {
                                break;
                            }
                        }
                        if (remaining && ptr - body >= remaining) {
                            printf("%.*s", remaining, body);
                            body += remaining + 2;
                            remaining = 0;
                        }
                    } while (!remaining);
                }
            } //if (body)
        } //if FDSET
    } //end while(1)
finish:

	printf("\nClosing socket..\n");
	CLOSESOCKET(server_sock);

#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Done.\n");
	return 0;
}

SOCKET connect_to_host(char *hostname, char*port) {

	printf("Configuring remote addr..\n");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *peer_addr;
	if (getaddrinfo(hostname, port, &hints, &peer_addr)) {
		fprintf(stderr, "Call to getaddrinfo() failed. (%d)\n", GETSOCKETERR());
		exit(1);
	}

	printf("Remote addr is: ");
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
	SOCKET server_sock = socket(peer_addr->ai_family,
		peer_addr->ai_socktype,
		peer_addr->ai_protocol);
	if (!ISVALIDSOCKET(server_sock)) {
		fprintf(stderr, "Call to socket() failed. (%d)\n", GETSOCKETERR());
		exit(1);
	}

	printf("Connecting..\n");
	if (connect(server_sock, peer_addr->ai_addr, peer_addr->ai_addrlen)) {
		fprintf(stderr, "Call to connect() failed. (%d)\n", GETSOCKETERR());
		exit(1);
	}
	freeaddrinfo(peer_addr);

	printf("Connected.\n");

	return server_sock;
}

void parse_url(char *url, char **hostname, char **port, char **path) {

	printf("URL: %s\n", url);

	char *ptr;
	ptr = strstr(url, "://");

	char *protocol = 0;
	if (ptr) {
		protocol = url;
		*ptr = 0;
		ptr += 3;
	}
	else
		ptr = url;

	if (protocol) {
		if (strcmp(protocol, "http")) {
			fprintf(stderr, "fatal error: bad protocol '%s'. \n expected: http", protocol);
			exit(1);
		}
	}

	*hostname = ptr;
	while (*ptr && *ptr != ':' && *ptr != '/' && *ptr != '#')
		++ptr;

	*port = "80";
	if (*ptr == ':') {
		*ptr++ = 0;
		*port = ptr;
	}

	while (*ptr && *ptr != '/' && *ptr != '#')
		++ptr;

	*path = ptr;
	if (*ptr == '/')
		*path = ptr + 1;

	*ptr = 0;

	while (*ptr && *ptr != '#')
		++ptr;

	if (*ptr == '#')
		*ptr = 0;

	printf("hostname: %s\n", *hostname);
	printf("port: %s\n", *port);
	printf("path: %s\n", *path);
}

void send_request(SOCKET sock, char *hostname, char *port, char *path) {

    char buffer[2048];

    sprintf(buffer, "GET /%s HTTP/1.1\r\n", path);
    sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "User-Agent: OW get_client v1.0\r\n");
    sprintf(buffer + strlen(buffer), "\r\n");

    send(sock, buffer, strlen(buffer), 0);
    printf("\nHeaders sent:\n%s", buffer);
}
