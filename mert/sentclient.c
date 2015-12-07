/* Copyright (c) 2001 by David Chiang. All rights reserved.*/

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#include "sentserver.h"

int main (int argc, char *argv[]) {
  int sock, port;
  char *s;
  struct hostent *hp;
  struct sockaddr_in server;
  int errors = 0;

  if (argc < 3) {
    fprintf(stderr, "Usage: sentclient host[:port] command [args ...]\n");
    exit(1);
  }

  s = strrchr(argv[1], ':');

  if (s == NULL) {
    port = DEFAULT_PORT;
  } else {
    *s = '\0';
    port = atoi(s+1);
  }

  sock = socket(AF_INET, SOCK_STREAM, 0);

  hp = gethostbyname(argv[1]);
  if (hp == NULL) {
    fprintf(stderr, "unknown host %s\n", argv[1]);
    exit(1);
  }

  bzero((char *)&server, sizeof(server));
  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
  server.sin_family = hp->h_addrtype;
  server.sin_port = htons(port);

  while (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("connect()");
    sleep(1);
    errors++;
    if (errors > 5)
      exit(1);
  }

  close(0);
  close(1);
  dup2(sock, 0);
  dup2(sock, 1);

  execvp(argv[2], argv+2);
}
