//This needs to be compiled with:
//gcc win_iflist.c -o <name_of_executable>.exe -liphlpapi -lws2_32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

int main () {

	WSADATA d;
	if (WSAStartup(MAKEWORD(2, 2), &d)) {
		fprintf(stderr, "Init failed.\n");
		return 1;
	}

	DWORD adapter_size = 25000;
	PIP_ADAPTER_ADDRESSES adapters;
	do {
		adapters = (PIP_ADAPTER_ADDRESSES)malloc(adapter_size);

		if (!adapters) {
			fprintf(stderr, "Was not able to allocate %ld bytes to adapters.\n", adapter_size);
			WSACleanup();
			return 1;
		}

		int req = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0, adapters, &adapter_size);
		
		if (req == ERROR_BUFFER_OVERFLOW) {
			printf("GetAdaptersAddresses needs %ld bytes.\n", adapter_size);
			free(adapters);
		}
		else if (req == ERROR_SUCCESS) 
			break;
		
		else {
			fprintf(stderr, "GetAdaptersAddresses threw an error: %d.\n", req);
			free(adapters);
			WSACleanup();
			return 1;
		}
	} while (!adapters);


	PIP_ADAPTER_ADDRESSES adapter = adapters;
	while (adapter) {
		printf("\nAdapter: %S\n", adapter->FriendlyName);

		PIP_ADAPTER_UNICAST_ADDRESS addr = adapter->FirstUnicastAddress;
		while (addr) {
			printf("\t%s", addr->Address.lpSockaddr->sa_family == AF_INET ? "IPv4" : "IPv6");

			char str[100];

			getnameinfo(addr->Address.lpSockaddr, addr->Address.iSockaddrLength, str, sizeof(str), 0, 0, NI_NUMERICHOST);
			printf("\t%s\n", str);

			addr = addr->Next;
		}

		adapter = adapter->Next;
	}

	free(adapters);
	WSACleanup();
	return 0;

}