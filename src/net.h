#ifndef NET_H
#define NET_H

#include <time.h>

/* Constant to TTL */
#define TTL 1024  /* Cache entry time-to-live in seconds */

/* Network operations features */
int net_open(char *hostname, int port);          /* Open a network connection to a host */
int net_bind(int port);                          /* Bind a port and listen for connections*/
void net_close(int s);                           /* Close a network connection */
char *net_gethostname();                         /* Get the full domain name of the host */
int net_accept(int s);                           /* Accept an incoming connection */

#endif /* NET_H */
