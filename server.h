/* Yixiong Ding, <yixiongd@student.unimelb.edu.au>
 * 25 May, 2017
 * The University of Melbourne */

#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#include "uint256.h"
#include "sha256.h"

/* The structure of worker threads */
struct WorkThreadData {
    int fd;
	char *ip;
    char *difficulty;
    char *seed;
    char *solution;
    unsigned long long nonce;
    int worker_count;
    int *flag;
};


/* Worker handler, used to find a valid PoW */
void* worker(void *d); 

/* Intercept a string from another string */
char* substring(char* ch,int pos,int length);

/* Calculate the target from difficulty */
void calc_target(char *hex, BYTE result[]);

/* Calculate h(h(x)) from seed and nonce */
void get_hash(BYTE *hash, char *seed, unsigned long long nonce);

/* Handle all the request, and respond */
void handle_request(char *ip, int fd, char *request, char *response, int* flag);

/* The thread handler, used to handle clients connection */
void* handle_client(void *data);

/* Signal handler for Ctrl-C */
void SIGINT_handler(int signal);

#endif
