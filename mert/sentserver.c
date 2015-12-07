/* Copyright (c) 2001 by David Chiang. All rights reserved.*/

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sched.h>
#include <pthread.h>

#include "sentserver.h"

#define MAX_CLIENTS 32

struct clientinfo {
  int s;
  struct sockaddr_in sin;
};

struct line {
  char *s;
  int status;
  struct line *next;
} *head, **ptail;

int n_sent = 0, n_received=0, n_flushed=0;

#define STATUS_RUNNING 0
#define STATUS_ABORTED 1
#define STATUS_FINISHED 2

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int n_clients = 0;
int s;

char * read_line(int fd);
void done (void);

struct line * queue_get() {
  struct line *cur;
  char *s;

  pthread_mutex_lock(&queue_mutex);

  /* First, check for aborted sentences. */

  for (cur = head; cur != NULL; cur = cur->next) {
    if (cur->status == STATUS_ABORTED) {
      cur->status = STATUS_RUNNING;

      pthread_mutex_unlock(&queue_mutex);

      return cur;
    }
  }

  /* Otherwise, read a new one. */

  s = read_line(0);

  if (s) {
    cur = malloc(sizeof (struct line));
    cur->s = s;
    cur->status = STATUS_RUNNING;
    cur->next = NULL;

    *ptail = cur;
    ptail = &cur->next;

    n_sent++;

    pthread_mutex_unlock(&queue_mutex);

    return cur;
  } else {

    if (head == NULL) {
      fprintf(stderr, "Reached end of file. Exiting.\n");
      done();
    } else
      ptail = NULL;

    pthread_mutex_unlock(&queue_mutex);

    return NULL;
  }

}

void queue_abort(struct line *node) {

  pthread_mutex_lock(&queue_mutex);

  node->status = STATUS_ABORTED;

  pthread_mutex_unlock(&queue_mutex);
}

void queue_print() {
  struct line *cur;

  fprintf(stderr, "Queue\n");

  for (cur = head; cur != NULL; cur = cur->next) {
    switch(cur->status) {
    case STATUS_RUNNING:
      fprintf(stderr, "running  "); break;
    case STATUS_ABORTED:
      fprintf(stderr, "aborted  "); break;
    case STATUS_FINISHED:
      fprintf(stderr, "finished "); break;
      
    }

    fprintf(stderr, cur->s);
  }
}

void queue_finish(struct line *node, char *s) {
  struct line *next;
  pthread_mutex_lock(&queue_mutex);

  free(node->s);
  node->s = s;
  node->status = STATUS_FINISHED;
  n_received++;

  /* Flush out finished nodes */
  while (head && head->status == STATUS_FINISHED) {

    fputs(head->s, stdout);
    free(head->s);

    next = head->next;
    free(head);
    
    head = next;

    n_flushed++;

    if (head == NULL) { /* empty queue */
      if (ptail == NULL) {
	fprintf(stderr, "All sentences finished. Exiting.\n");
	done();
      } else
	ptail = &head;
    }
  }

  fflush(stdout);
  fprintf(stderr, "%d sentences sent, %d sentences finished, %d sentences flushed\n", n_sent, n_received, n_flushed);

  pthread_mutex_unlock(&queue_mutex);

}

char * read_line(int fd) {
  int size = 80;
  char *s = malloc(size+2);
  int result, errors=0;
  int i = 0;

  result = read(fd, s+i, 1);

  while (1) {
    if (result < 0) {
      perror("read()");
      errors++;
      if (errors > 5) {
	free(s);
	return NULL;
      } else {
	sleep(1); /* retry after delay */
      }
    } else if (result == 0 || s[i] == '\n') {
      break;
    } else {
      i++;
      
      if (i == size) {
	size = size*2;
	s = realloc(s, size+2);
      }
    }    

    result = read(fd, s+i, 1);
  }

  if (result == 0 && i == 0) { /* end of file */
    free(s);
    return NULL;
  }
    
  s[i] = '\n';
  s[i+1] = '\0';

  return s;
}

void * new_client(void *arg) {
  struct clientinfo *client = (struct clientinfo *)arg;
  struct line *cur;
  int len;
  char *s;

  int flags;

  pthread_mutex_lock(&clients_mutex);
  n_clients++;
  pthread_mutex_unlock(&clients_mutex);

  fprintf(stderr, "Client connected (%d connected)\n", n_clients);

  for (;;) {

    cur = queue_get();
    
    if (cur) {
      fprintf(stderr, "Sending to client: %s", cur->s);
      write(client->s, cur->s, strlen(cur->s));
    } else {
      close(client->s);
      pthread_mutex_lock(&clients_mutex);
      n_clients--;
      pthread_mutex_unlock(&clients_mutex);
      fprintf(stderr, "Client finished (%d connected)\n", n_clients);
      pthread_exit(NULL);
    }
    
    s = read_line(client->s);
    if (s) {
      fprintf(stderr, "Client returned: %s", s);
      queue_finish(cur, s);
    } else {
      pthread_mutex_lock(&clients_mutex);
      n_clients--;
      pthread_mutex_unlock(&clients_mutex);

      fprintf(stderr, "Client died (%d connected)\n", n_clients);
      queue_abort(cur);

      close(client->s);
      free(client);

      pthread_exit(NULL);
    }

  }

}

void done (void) {
  close(s);
  exit(0);
}

int main (int argc, char *argv[]) {
  struct sockaddr_in sin, from;
  int g, len;
  struct clientinfo *client;
  int port;
  int opt;
  int errors = 0;

  pthread_t tid;

  /*pid_t pid;

  pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    exit(EXIT_SUCCESS);
    }*/

  if (argc >= 2)
    port = atoi(argv[1]);
  else
    port = DEFAULT_PORT;

  /* Initialize data structures */
  head = NULL;
  ptail = &head;

  /* Set up listener */
  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  opt = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(port);
  while (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	perror("bind()");
	sleep(1);
	errors++;
	if (errors > 100)
	  exit(1);
  }

  len = sizeof(sin);
  getsockname(s, (struct sockaddr *) &sin, &len);

  fprintf(stderr, "Listening on port %hd\n", ntohs(sin.sin_port));

  while (listen(s, MAX_CLIENTS) < 0) {
	perror("listen()");
	sleep(1);
	errors++;
	if (errors > 100)
	  exit(1);
  }

  for (;;) {
    len = sizeof(from);
    g = accept(s, (struct sockaddr *)&from, &len);
    if (g < 0) {
      perror("accept()");
      sleep(1);
      continue;
    }
    client = malloc(sizeof(struct clientinfo));
    client->s = g;
    bcopy(&from, &client->sin, len);

    pthread_create(&tid, NULL, new_client, client);
  }

}



