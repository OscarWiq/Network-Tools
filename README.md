# Network-Tools
Basic network tools written in C

list-interfaces: 
- reads network adapters and lists relevant information such as addresses, addr-types and interface names.
	
timeserver: 
- responds to a request with current date and time
	

tcp-client: 
- functional tcp client, send basic requests and connect to tcp-servers. useful to test server-code. 

	example request: "./client example.com 80" then:
	"GET / HTTP/1.1"
	"Host: example.com"
	
tcp-server: 
- basic chatserver, responds to the client program and relays messages to all non-sender/non-listener sockets.
- basic microservice, manipulates received string with toupper()/tolower() and sends it back to client.

udp client & server:
- client utlizes same code as tcp except for ai_socktype = SOCK_DGRAM, useful to test server-code.
- server: TODO: microservice like tcp but instead udp

dns:
- utility to send dns queries: a, aaaa, mx, txt, any. by default to 1.1.1.1 but easily changed.
- usage: gcc dns_query.c -o query; ./query hostname type
- example: query example.com a -> returns a complete dns message which is printed out. 
