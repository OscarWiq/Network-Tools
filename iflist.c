#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>

int main () {

    struct ifaddrs *addrs, *addr;

    if (getifaddrs(&addrs) == -1) {
        printf("Init failed. \n");
        return -1;
    }

    addr = addrs;
    while (addr) {
	   if (addr->ifa_addr == NULL) {
		  addr = addr->ifa_next;
		  continue;
		}	

        int fam = addr->ifa_addr->sa_family;
        if (fam == AF_INET || fam == AF_INET6) {
            printf("%s\t", addr->ifa_name);
            printf("%s\t", fam == AF_INET ? "ipv4" : "ipv6");

            char str[100];
            const int fam_size = fam == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            getnameinfo(addr->ifa_addr, fam_size, str, sizeof(str), 0, 0, NI_NUMERICHOST);
            printf("\t%s\n", str);
        }
        addr = addr->ifa_next;
    }

    freeifaddrs(addrs);
    return 0;
}
