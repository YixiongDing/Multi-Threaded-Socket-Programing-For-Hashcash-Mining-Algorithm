/* Yixiong Ding, <yixiongd@student.unimelb.edu.au>
 * 25 May, 2017
 * The University of Melbourne */

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

#include "server.h"

#define UINT64MAXIMUM "ffffffffffffffff"
#define MAXCLIENTS 100
#define MAXWORKERS 255
#define MAXWORKINQ 10

int work_in_queue = 0;
int client_num = 0;

const static char* log_file = "log.txt";

pthread_t client_thread;
pthread_t worker_threads[MAXWORKERS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int main (int argc, char **argv) {

    /* Set the default portno to 12345 */
    int sockfd, connfd, portno = 12345;
    struct sockaddr_in serv_addr;
   
    /* Handle Ctrl-C signal, exit all threads safely */
    signal(SIGINT, SIGINT_handler);

    /* Remove the pre-existing log file */
    remove(log_file);

    if (argc == 2) {

    /* Get the portno */
     portno = atoi(argv[1]);
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Create TCP socket */
    sockfd = socket(AF_INET,SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERRO, Can't open socket\n");
        exit(0);
    }

    /* Bind and listen the socket */
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERRO, Can't bind\n");
        exit(0);
    }
    
    if (listen(sockfd, 10) < 0) {
        perror("ERRO, Can't listen\n");
        exit(0);
    }

    /* Enter a loop to get client connection */
    while (1) {
        
        /* Accept a connection from client */               
        connfd = accept(sockfd, NULL, 0);
        client_num++;
        
        if(client_num > MAXCLIENTS) {
            puts("ERRO, Exceed maximum clients\n");
            close(connfd);
            client_num--;
            continue;
        }

        if (connfd < 0) {
            perror("ERRO, Can't accept\n");
            exit(0);
        }
        
        /* Create a pthread to handle the client */
        if (pthread_create(&client_thread, NULL, handle_client, (void*)(long)connfd)) {
            perror("ERRO, Can't create a new thread\n");
            exit(0);
        }
    }
    close(sockfd);
    return 0;
}

/* Signal handler for Ctrl-C */
void SIGINT_handler(int signal) {
    if(client_num != 0) {
        for(int i=0; i< MAXWORKERS; i++) {
            if(worker_threads[i] != 0) {
                pthread_cancel(worker_threads[i]);
            }
        }
    
       pthread_cancel(client_thread);
    }
    puts("\nAll threads killed, the server exits safely!\n");
    exit(0);
}

/* Intercept a string from another string */
char* substring(char* ch,int pos,int length) {
    char* pch=ch;
    char* subch=(char*)calloc(sizeof(char),length+1);
    int i;
    pch = pch + pos;
    for(i=0; i<length; i++) {
        subch[i] = *(pch++);
    }
    subch[length] = '\0';
    return subch;      
}

/* Calculate the target from difficulty */
void calc_target(char *hex, BYTE result[]) {
    BYTE alpha_256[32];
    BYTE beta_256[32];
    BYTE base_2[32];
    uint256_init(alpha_256);
    uint256_init(beta_256);
    uint256_init(base_2);
    uint256_init(result);
    alpha_256[31] = (unsigned)strtol(substring(hex,0,2), NULL, 16);
    uint32_t alpha = (uint32_t)alpha_256[31];  
    uint32_t expo =(uint32_t) 8*(alpha-3);
    beta_256[29] = (unsigned)strtol(substring(hex,2,2), NULL, 16);
    beta_256[30] = (unsigned)strtol(substring(hex,4,2), NULL, 16);
    beta_256[31] = (unsigned)strtol(substring(hex,6,2), NULL, 16);
    base_2[31] = 0x2;
    uint256_exp(result, base_2, expo);
    uint256_mul(result, beta_256, result);
}

/* Calculate h(h(x)) from seed and nonce */
void get_hash(BYTE *hash, char *seed, unsigned long long nonce) {
    BYTE buf[40];
    int i = 0, j = 0;
    SHA256_CTX ctx;

    /* Put seed to buffer */
    for (i = 0; i < 32; i++) {
        j = i * 2;
        if (seed[j] >= '0' && seed[j] <= '9') {
            buf[i] = (seed[j] - '0') << 4;
        }else {
            buf[i] = (seed[j] - 'a' + 10) << 4;
        }
        if (seed[j + 1] >= '0' && seed[j + 1] <= '9') {
            buf[i] |= seed[j + 1] - '0';
        }else {
            buf[i] |= seed[j + 1] - 'a' + 10;
        }
    }

    /* Put nonce to buffer */
    for (i = 0; i < 8; i++) {
        buf[32 + i] = (nonce >> ((7 - i)*8)) & 0xff;
    }

    /* Calculate hash twice */
    sha256_init(&ctx);
    sha256_update(&ctx, buf, 40);
    sha256_final(&ctx, hash);
    memcpy(buf, hash, 32);
    sha256_init(&ctx);
    sha256_update(&ctx, buf, 32);
    sha256_final(&ctx, hash);
}

/* Worker handler, used to find a valid PoW */
void* worker(void *d) {
    struct WorkThreadData *data = (struct WorkThreadData *)d;
    BYTE target[32], hash[32];
    char response[256];
    time_t now;
    char tmp[64];
    FILE *log_fp = NULL;

    /* Open the log file to logging */
    log_fp = fopen(log_file, "a+");
    if (log_fp == NULL) {
        perror("ERRO, Can't open the log file");
        pthread_exit(NULL);
    }
    
    /* Calculate the target */
    calc_target(data->difficulty,target);
    while (*(data->flag) == 0) {
        
        /* Calculatge the hash */
        get_hash(hash, data->seed, data->nonce);

        /* Compare target with hash */
        if(sha256_compare(target, hash)==1) {
        
            /* Successfully */
            sprintf(response, "SOLN %s %s %016llx\r\n", data->difficulty, data->seed, data->nonce);
            *(data->flag) = 1;

            /* Record information and write to log file */  
            now = time(NULL);
            strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&now));

            pthread_mutex_lock(&lock);
            fprintf(log_fp, "Time: %s\nClient IP: %s\nSock ID: %d\nResponse to client: %s\n",tmp, data->ip, data->fd, response);
            pthread_mutex_unlock(&lock);

            fclose(log_fp);
            write(data->fd, response, strlen(response));
            work_in_queue--;
            pthread_exit(NULL);
        }

        /* Try next value */
        data->nonce += 1;
    }
    pthread_exit(NULL);
}

/* Handle all the request, and respond */
void handle_request(char *ip, int fd, char *request, char *response, int* flag) {

    /* Use strtok to split the request */
    int mes_len = strlen(request);
    char *x = substring(request,0,4);
    char *s = strtok(request, " ");
    char *difficulty = NULL;
    char *seed = NULL;
    char *solution = NULL;
    char *worker_count_s = NULL;
    int worker_count = 0;
    unsigned long long nonce = 0;
    int i = 0;
    struct WorkThreadData *data = NULL;
    char uint64_maximum_s[17];
    unsigned long long uint64_maximum;
    unsigned long long remainder;
    BYTE target[32], hash[32];
    
    /* Received a PING, response PONG */
    if (strcmp(x, "PING") == 0) {
        strcpy(response, "PONG\r\n");
        return;
    }

    /* Received a PONG, response ERRO */
    if (strcmp(x, "PONG") == 0) {
        strcpy(response, "ERRO, 'PONG' is reserved for server responses\r\n");
        return;
    }

    /* Received a OKAY, response ERRO */
    if (strcmp(x, "OKAY") == 0) {
        strcpy(response, "ERRO, 'OKAY' is reserved for server responses\r\n");
        return;
    }

    /* Received a OKAY, response ERRO */
    if (strcmp(x, "ERRO") == 0) {
        strcpy(response, "ERRO, 'ERRO' is reserved for server responses\r\n");
        return;
    }

    /* Received ABRT, response OKAY */
    if (strcmp(x, "ABRT") == 0) {
        *flag = 1;
        strcpy(response, "OKAY\r\n");

        /* Cancel the worker threads */
        for(int i=0; i< MAXWORKERS; i++) {
            if(worker_threads[i] != 0) {
                pthread_cancel(worker_threads[i]);
            }
        }
        return;
    }
    
    /* Received SOLN, check if a PoW is valid */
    if (strcmp(s, "SOLN") == 0) {
        
        /* Check if the message is complete */
        if(mes_len != 97 ) {
            strcpy(response, "ERRO, Wrong SOLN message\r\n");
            return;
        }

        /* Use strtok to split the request */
        difficulty = strtok(NULL, " ");
        seed = strtok(NULL, " ");
        solution = strtok(NULL, " \r\n");
        nonce = strtoull(solution, NULL, 16);

        /* Get target */
        calc_target(difficulty,target);

        /* Get hash */
        get_hash(hash, seed, nonce);

        /* Compare target with hash */
        if(sha256_compare(target, hash)==1) {

            /* Successfully */
            strcpy(response, "OKAY\r\n");
        }else {
            strcpy(response, "ERRO, It is not a solution\r\n");
        }
        return;
    }
    
    /* Received WORK, find a valid PoW */
    if (strcmp(s, "WORK") == 0) {
        
        /* Check if the message is complete */
        if(mes_len != 100 ) {
            strcpy(response, "ERRO, Wrong WORK message length\r\n");
            strcpy(request, "WRONGLEN");
            return;
        }

        /* If there are 10 work messages in the queue, reject the new work messages */
        if(work_in_queue > MAXWORKINQ - 1) {
            strcpy(response, "ERRO, Too much work in the queue!\r\n");
            strcpy(request, "TOOMUCHWORK");
            return;
        }
        
        /* Use strtok to split the request */
        difficulty = strtok(NULL, " ");
        seed = strtok(NULL, " ");
        solution = strtok(NULL, " ");
        worker_count_s = strtok(NULL, " \r\n");
        worker_count = strtol(worker_count_s, NULL, 16);
        nonce = strtoull(solution, NULL, 16);
        *flag = 0;
        work_in_queue++;
           
        /* Calculate work space for workers */
        strcpy(uint64_maximum_s, UINT64MAXIMUM);
        uint64_maximum = strtoull(uint64_maximum_s, NULL, 16);
        remainder = uint64_maximum - nonce; 
    
        /* Create a number of worker threads to find PoW */
        for (i = 0; i < worker_count; i++) {
                
            /* Write info into worker thread data */
            data =(struct WorkThreadData *) malloc(sizeof(struct WorkThreadData));
            data->fd = fd;
            data->ip = strdup(ip);
            data->difficulty = strdup(difficulty);
            data->seed = strdup(seed);
            data->solution = strdup(solution);
            data->nonce = nonce + (remainder/worker_count)*i;
            data->worker_count = worker_count;
            data->flag = flag;

            /* Create worker thread */
            if (pthread_create(&worker_threads[i], NULL, worker, data)) {
                perror("ERRO, Can't create a new thread\n");
                exit(0);
            }
        }
        return;
    }
    
    /* Received others, respond ERRO */
    strcpy(response, "ERRO, Invalid command\r\n");
}

/* The thread handler, used to handle clients connection */
void* handle_client(void *data) {
    char ip[16];
    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);
    FILE *log_fp = NULL;
    int connfd = (int)(long)data;
    char request[256], response[256];
    char tmp[64];
    time_t now;
    int *flag = (int *)malloc(sizeof(int) * 1024);
    int flag_count = 0;
    int i = 0;
    
    /* Open the log file to logging */
    log_fp = fopen(log_file, "a+");
    if(log_fp == NULL) {
        perror("ERRO, Can't open the log file\n");
        pthread_exit(NULL);
    }
   
    /* Get the client address from fd */
    if (getpeername(connfd, (struct sockaddr *)&cli_addr, &len) < 0) {
        perror("ERRO, Can't get perr name\n");
        pthread_exit(NULL);
    }
    strcpy(ip, inet_ntoa(cli_addr.sin_addr));

    /* Record the client connect information */
    now = time(NULL);
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    pthread_mutex_lock(&lock);
    fprintf(log_fp, "Time: %s\nClient IP: %s\nSock ID: %d\n-> Client Connected\n\n", tmp, ip, connfd);
    pthread_mutex_unlock(&lock);

    /* Read command from client */
    while (read(connfd, request, sizeof(request)) > 0) {
        
        /* Record the client message */
        now = time(NULL);
        strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&now));

        pthread_mutex_lock(&lock);
        fprintf(log_fp, "Time: %s\nClient IP: %s\nSock ID: %d\nRequest from client: %s\n", tmp, ip, connfd, request);
        pthread_mutex_unlock(&lock);

        handle_request(ip,connfd, request, response, flag + (flag_count++));
        
        /* Abort the work message of the current client */
        if (strcmp(request, "ABRT") == 0) {
            for (i = 0; i < flag_count; i++) {
                flag[i] = 1;
            }   
        }

        /* Write the response to the server */
        if (strcmp(request, "WORK") != 0) {

            /* If not 'WORK', write the response (WORK will do by other thread) */
            write(connfd, response, strlen(response));

            /* Record the server message */
            now = time(NULL);
            strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&now));

            pthread_mutex_lock(&lock);
            fprintf(log_fp, "Time: %s\nClient IP: %s\nSock ID: %d\nResponse to client: %s\n", tmp, ip, connfd, response); 
            pthread_mutex_unlock(&lock);  
        }
        
        /* Flushes the buffer */
        fflush(log_fp);
    }   
            
    /* Close the connection and the log file */
    client_num--;
    now = time(NULL);
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    pthread_mutex_lock(&lock);
    fprintf(log_fp, "Time: %s\nClient IP: %s\nSock ID: %d\n<- Client Disconnected\n\n", tmp, ip, connfd);
    pthread_mutex_unlock(&lock);

    close(connfd);
    fclose(log_fp);
    pthread_exit(NULL);
}
