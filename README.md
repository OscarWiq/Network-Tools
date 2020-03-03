# Network-Tools
Basic network tools written in C

- list-interfaces: reads network adapters and lists relevant information such as addresses, addr-types and interface names.
- timeserver: answers to a request with current date and time
- tcp-client-server: functional tcp client, send requests to sites. 
	example call: "./client example.com http" then:
	"GET / HTTP/1.1"
	"Host: example.com"
	TODO: tcp-server to communicate with client
