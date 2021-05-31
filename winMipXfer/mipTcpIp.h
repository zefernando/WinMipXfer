#ifndef _SOCKHELP_H_
#define _SOCKHELP_H_


#include <stdio.h>
#include <string.h>
#include <WinSock2.h>

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

struct in_addr *atoaddr(char *);
int make_connection(/* char *service, int type, char *netaddress */);
int sock_read(/* int sockfd, char *buf, size_t count */);
int sock_write(/* int sockfd, const char *buf, size_t count */);
int sock_gets(/* int sockfd, char *str, size_t count */);
int sock_puts(/* int sockfd, const char *str */);


#endif
