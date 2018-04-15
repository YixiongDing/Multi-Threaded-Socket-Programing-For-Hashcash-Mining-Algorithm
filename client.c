/* Yixiong Ding, <yixiongd@student.unimelb.edu.au>
 * 25 May, 2017
 * The University of Melbourne */

/**************************************************/

/* A simple client program for a server program

   To compile: gcc client.c -o client
   				   
   To run: start the server, then run the client */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int main(int argc, char**argv)
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];

	if (argc < 3) 
	{
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);

	
	/* Translate host name into peer's IP address ;
	 * This is name translation service by the operating system 
	 */
	server = gethostbyname(argv[1]);
	
	if (server == NULL) 
	{
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	
	/* Building data structures for socket */

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, 
			(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);

	serv_addr.sin_port = htons(portno);

	/* Create TCP socket -- active open 
	* Preliminary steps: Setup: creation of active open socket
	*/
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sockfd < 0) 
	{
		perror("ERROR opening socket");
		exit(0);
	}
	
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
	{
		perror("ERROR connecting");
		exit(0);
	}

	/* Do processing
	*/
	

	bzero(buffer,256);

    strcpy(buffer, "PING\r\n");
	
	//strcpy(buffer, "SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212147\r\n");

	//strcpy(buffer, "SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212605\r\n");

	//TEST
	//strcpy(buffer, "SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 100000002321262b\r\n");

	//strcpy(buffer, "WORK 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212605\r\n");
																									 //100000002321251e
	//strcpy(buffer, "SOLN 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 100000002321ed8f\r\n");
	//strcpy(buffer, "SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 10000000232123a2\r\n");



	//strcpy(buffer, "SOLN 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 100000002322a26f\r\n");
																									 //
	//strcpy(buffer, "WORK 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 100000002321ed8f\r\n");
	    
	//strcpy(buffer, "SOLN 1dffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023a1fa66\r\n");
	//strcpy(buffer, "SOLN 1dffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 0000111123a1fa66\r\n");
	
	//strcpy(buffer, "WORK 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 01\r\n");
	
    
	//strcpy(buffer, "WORK 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 02\r\n");
		    
	
	//strcpy(buffer, "WORK 1effffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 04\r\n");
																								   

	//strcpy(buffer, "WORK 1dffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 01\r\n");
	
	/* note: large computational effort (and thus time) required  */
	    	
	//strcpy(buffer, "WORK 1d29ffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 04\r\n");
	
	    //
	
    printf("%s\n", buffer);
    
    
	n = write(sockfd,buffer,strlen(buffer));

	if (n < 0) 
	{
		perror("ERROR writing to socket");
		exit(0);
	}
	
	bzero(buffer,256);

	n = read(sockfd,buffer,255);
	
	if (n < 0)
	{
		perror("ERROR reading from socket");
		exit(0);
	}

	printf("%s\n",buffer);

	return 0;
}
