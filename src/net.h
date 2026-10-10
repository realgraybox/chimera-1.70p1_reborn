#ifndef NET_H
#define NET_H

#include <time.h>

/* Konstant til TTL */
#define TTL 1024  /* Cache entry time-to-live in seconds */

/* Funktioner til netværksoperationer */
int net_open(char *hostname, int port);          /* Åbn en netværksforbindelse til en vært */
int net_bind(int port);                          /* Bind en port og lyt efter forbindelser */
void net_close(int s);                           /* Luk en netværksforbindelse */
char *net_gethostname();                         /* Hent det fulde domænenavn for værten */
int net_accept(int s);                           /* Accepter en indgående forbindelse */

#endif /* NET_H */
