#include "defs.h"

#define BUF_SIZE 1024
#define MAX_REQ_SIZE 2047

struct client_info {
	socklen_t ci_addrlen;
	struct sockaddr_storage ci_addr;
	char ci_addrbuffer[128];
	SOCKET ci_sock;
	char ci_request[MAX_REQ_SIZE + 1];
	int ci_received;
	struct client_info *next;
};
typedef struct client_info client_info;

const char *get_content_type(const char *path);
SOCKET make_socket(const char *hostname, const char *port);
client_info* get_client(client_info **client_list, SOCKET sock);
void drop_client(client_info **client_list, client_info *client);
const char *get_client_addr(client_info *info);
fd_set wait_on_clients(client_info **client_list, SOCKET server_sock);
void send_400(client_info **client_list, client_info *client);
void send_404(client_info **client_list, client_info *client);
void send_resource(client_info **client_list, client_info *client, const char *path);

int main () {

#if defined(_WIN32)
	WSADATA d;
	if (WSAstartup(MAKEWORD(2,2), &d)) {
		fprintf(stderr, "Init failed\n");
		return 1;
	}
#endif

	SOCKET server_sock = make_socket(0, "8080");
	client_info *client_list = 0;

	while (1) {

		fd_set sread;
		sread = wait_on_clients(&client_list, server_sock);

		if (FD_ISSET(server_sock, &sread)) {
			client_info *client = get_client(&client_list, -1);

			client->ci_sock = accept(server_sock,
				(struct sockaddr*)&(client->ci_addr),
				&(client->ci_addrlen));
			if (!ISVALIDSOCKET(client->ci_sock)) {
				fprintf(stderr, "Call to accept() failed. (%d)\n", GETSOCKETERR());
				return 1;
			}

			printf("New connection from %s.\n", get_client_addr(client));
		}

		client_info *client = client_list;
		while (client) {
			
			client_info *next = client->next;

			if (FD_ISSET(client->ci_sock, &sread)) {
				if (MAX_REQ_SIZE == client->ci_received) {
					send_400(&client_list, client);
					client = next;
					continue;
				}

			}

			int bytes_recv = recv(client->ci_sock,
				client->ci_request,
				MAX_REQ_SIZE - client->ci_received,
				0);

			if (bytes_recv < 1) {
				printf("Disconnection from %s - unexpected.\n", get_client_addr(client));
				drop_client(&client_list, client);
			}
			else {
				client->ci_received += bytes_recv;
				client->ci_request[client->ci_received] = 0;

				char *s_ptr = strstr(client->ci_request, "\r\n\r\n");
				if (s_ptr) {
					*s_ptr = 0;

					if (strncmp("GET /", client->ci_request, 5)) {
						send_400(&client_list, client);
					}
					else {
						char *path = client->ci_request + 4;
						char *path_end = strstr(path, " ");
						
						if (!path_end) {
							send_400(&client_list, client);
						}
						else {
							*path_end = 0;
							send_resource(&client_list, client, path);
						}
					}
				}// if (s_ptr)
			}

			client = next;
		}// while(client)
	}// while (1)

	printf("\nClosing socket..\n");
	CLOSESOCKET(server_sock);

#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Done.\n");
	return 0;
}

const char *get_content_type(const char *path) {
	const char *ext = strrchr(path, '.');
	if (ext) {
		if (strcmp(ext, ".css") == 0) return "text/css";
		if (strcmp(ext, ".txt") == 0) return "text/plain";
		if (strcmp(ext, ".csv") == 0) return "text/csv";
		if (strcmp(ext, ".jpg") == 0) return "image/jpeg";
		if (strcmp(ext, ".jpeg") == 0) return "image/jpg";
		if (strcmp(ext, ".gif") == 0) return "image/gif";
		if (strcmp(ext, ".htm") == 0) return "text/html";
		if (strcmp(ext, ".html") == 0) return "text/html";
		if (strcmp(ext, ".js") == 0) return "application/javascript";
		if (strcmp(ext, ".json") == 0) return "application/json";
		if (strcmp(ext, ".ico") == 0) return "image/x-icon";
		if (strcmp(ext, ".png") == 0) return "image/png";
		if (strcmp(ext, ".pdf") == 0) return "application/pdf";
		if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
	}

	return "aplication/octet-stream";
}

client_info *get_client(client_info **client_list, SOCKET sock) {

	client_info *info = *client_list;// get linked-root

	while (info) {
		if (info->ci_sock == sock)
			break;

		info = info->next;
	}

	// if found the loop breaks and we return
	if (info)
		return info;

	//otherwise make new struct to continue search
	client_info *new_info = (client_info*)calloc(1, sizeof(client_info));
	if (!new_info) {
		fprintf(stderr, "fatal error: out of memory\n");
		exit(1);
	}

	// set addrlen to proper size, needed for accept() later
	new_info->ci_addrlen = sizeof(new_info->ci_addr);
	// set new struct to beginning of linked-list
	new_info->next = *client_list;
	*client_list = new_info;

	return new_info;
}

SOCKET make_socket(const char *hostname, const char *port) {
    
    printf("Configuring local address..\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_addr;
    getaddrinfo(hostname, port, &hints, &bind_addr);

    printf("Configuring socket..\n");
    SOCKET listen_sock = socket(bind_addr->ai_family,
            bind_addr->ai_socktype,
            bind_addr->ai_protocol);

    if (!ISVALIDSOCKET(listen_sock)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERR());
        exit(1);
    }

    printf("Binding socket to local address..\n");
    if (bind(listen_sock, bind_addr->ai_addr, bind_addr->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERR());
        exit(1);
    }
    freeaddrinfo(bind_addr);

    printf("Listening..\n");
    if (listen(listen_sock, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERR());
        exit(1);
    }

    return listen_sock;
}

void drop_client(client_info **client_list, client_info *client) {

	CLOSESOCKET(client->ci_sock);

	client_info **ptr = client_list;

	while (*ptr) {
		if (*ptr == client) {
			*ptr = client->next;
			free(client);
			return;
		}
		ptr = &(*ptr)->next;
	}

	fprintf(stderr, "exception: drop_client not found.\n");
	exit(1);
}

const char *get_client_addr(client_info *info) {
	
	getnameinfo((struct sockaddr*)&info->ci_addr,
		info->ci_addrlen,
		info->ci_addrbuffer,
		sizeof(info->ci_addrbuffer),
		0,
		0,
		NI_NUMERICHOST);

	return info->ci_addrbuffer;
}

fd_set wait_on_clients(client_info **client_list, SOCKET server_sock) {

	fd_set sread;
	FD_ZERO(&sread);
	FD_SET(server_sock, &sread);
	SOCKET max_sock = server_sock;

	client_info *info = *client_list;

	while (info) {
		FD_SET(info->ci_sock, &sread);
		if (info->ci_sock > max_sock)
			max_sock = info->ci_sock;
		info = info->next;
	}

	if (select(max_sock + 1, &sread, 0, 0, 0) < 0) {
		fprintf(stderr, "Call to select() failed. (%d)\n", GETSOCKETERR());
		exit(1);
	}

	return sread;
}

void send_400(client_info **client_list, client_info *client) {

	const char *s_400 = "HTTP/1.1 400 Bad Request\r\n"
		"Connection: close\r\n"
		"Content-Length: 11\r\n\r\nBad Request";

	send(client->ci_sock, s_400, strlen(s_400), 0);
	drop_client(client_list, client);
}

void send_404(client_info **client_list, client_info *client) {
	const char *s_404 = "HTTP/1.1 404 Not Found\r\n"
		"Connection: close\r\n"
		"Content-Length: 9\r\n\r\nNot Found";

	send(client->ci_sock, s_404, strlen(s_404), 0);
	drop_client(client_list, client);
}

void send_resource(client_info **client_list, client_info *client, const char *path) {

	printf("send_resource %s %s\n", get_client_addr(client), path);

	if (strcmp(path, "/") == 0)
		path = "/index.html";

	if (strlen(path) > 100) {
		send_400(client_list, client);
		return;
	}

	if (strstr(path, "..")) {
		send_404(client_list, client);
		return;
	}

	char abs_path[128];
	sprintf(abs_path, "public%s", path);

#if defined(_WIN32)
	char *ptr = abs_path;
	while (*ptr) {
		if (*ptr == '/')
			*ptr = '\\';
		++ptr;
	}
#endif	

	FILE *fp = fopen(abs_path, "rb");
	if (!fp) {
		send_404(client_list, client);
		return;
	}

	fseek(fp, 0L, SEEK_END);
	size_t content_len = ftell(fp);
	rewind(fp);

	const char *content_type = get_content_type(abs_path);

	char buffer[BUF_SIZE];

	sprintf(buffer, "HTTP/1.1 200 OK\r\n");
	send(client->ci_sock, buffer, strlen(buffer), 0);

	sprintf(buffer, "Connection: close\r\n");
	send(client->ci_sock, buffer, strlen(buffer), 0);

	sprintf(buffer, "Content-Length: %u\r\n", content_len);
	send(client->ci_sock, buffer, strlen(buffer), 0);

	sprintf(buffer, "Content-Type: %s\r\n", content_type);
	send(client->ci_sock, buffer, strlen(buffer), 0);

	sprintf(buffer, "\r\n");
	send(client->ci_sock, buffer, strlen(buffer), 0);

	int res = fread(buffer, 1, BUF_SIZE, fp);
	while (res) {
		send(client->ci_sock, buffer, res, 0);
		res = fread(buffer, 1, BUF_SIZE, fp);
	}

	fclose(fp);
	drop_client(client_list, client);
}
