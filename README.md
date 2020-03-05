# Network-Tools
Basic network tools written in C

- list-interfaces: reads network adapters and lists relevant information such as addresses, addr-types and interface names.
- timeserver: answers to a request with current date and time
- tcp-client: functional tcp client, send basic requests and connect to tcp-servers. useful to test server-code. 
	example request: "./client example.com 80" then:
	"GET / HTTP/1.1"
	"Host: example.com"
- tcp-server: - basic chatserver, responds to the client program and relays messages to all non-sender/non-listener sockets.
	      - basic microservice, manipulates received string with toupper()/tolower() and sends it back to client.
