/*
 * net.c
 *
 * Opdateret version, optimeret til Linux.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define TTL 1024  /* Cache entry time-to-live in seconds */

/*
 * net_open
 * Open a network connection to a host.
 */
int net_open(char *hostname, int port) {
    int s;
    struct sockaddr_in addr;
    struct hostent *hp = NULL;
   // time_t now;

   // now = time(NULL);

    memset(&addr, 0, sizeof(addr));

    if (inet_addr(hostname) == INADDR_NONE) {
		hp = gethostbyname(hostname);
		if (hp == NULL) {
			fprintf(stderr, "DNS lookup failed for hostname '%s': %s\n", hostname, hstrerror(h_errno));
			return -1;
		}
		memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
	} else {
		addr.sin_addr.s_addr = inet_addr(hostname);
	}


    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
        perror("Socket creation failed");
        return -1;
    }

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        close(s);
        return -1;
    }

    /* no printf here: the status line shows the connection, and text
     * left in Chimera's stdout buffer could end up in a page (input.c) */
    return s;
}

/*
 * net_bind
 * Open a socket, bind a port to it, and listen for connections.
 */
int net_bind(int port) {
    int s;
    struct sockaddr_in addr;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
        perror("Socket creation failed");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(s);
        return -1;
    }

    if (listen(s, 1) < 0) {
        perror("Listen failed");
        close(s);
        return -1;
    }

    return s;
}

/*
 * net_close
 * Close a network connection.
 */
void net_close(int s) {
    close(s);
}

/*
 * net_gethostname
 * Returns the full domain name of the local host.
 */
char *net_gethostname() {
    char myname[256];
    struct hostent *hp;
    static char domain[256];

    if (gethostname(myname, sizeof(myname)) < 0) {
        perror("gethostname failed");
        return NULL;
    }

    hp = gethostbyname(myname);
    if (hp == NULL) {
        perror("gethostbyname failed");
        return NULL;
    }

    strncpy(domain, hp->h_name, sizeof(domain) - 1);
    domain[sizeof(domain) - 1] = '\0';

    return domain;
}

/*
 * net_accept
 * Accept an incoming connection.
 */
int net_accept(int s) {
    struct sockaddr_in sock;
    socklen_t len = sizeof(sock);

    return accept(s, (struct sockaddr *)&sock, &len);
}
