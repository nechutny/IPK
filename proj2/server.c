#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_SIZE	65535

int sockfd;

/**
 * Handle SIGINT and close socket
 */
void term()
{
	 close(sockfd);
	 printf("Exiting\n");
	 exit(0);
}


/**
 * Main function
 *
 * @param	int	argc	Number of arguments
 * @param	char*	argv	Array of arguments
 *
 * @return	int	Exit code
 */
int main(int argc, char* argv[])
{
	// Register SIGINT handler
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);

	int n;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;
	char mesg[MAX_SIZE];

	// Arguments
	int port = 5656;
	if(argc == 3)
	{
		port = atoi(argv[2]);
	}

	// Open socket
	sockfd=socket(AF_INET, SOCK_DGRAM, 0);
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(port);
	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	// Mian loop
	for (;;)
	{
		// Receive and send back
		len = sizeof(cliaddr);
		n = recvfrom(sockfd,mesg, 1000, 0, (struct sockaddr *)&cliaddr,&len);
		sendto(sockfd, mesg, n, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));

		//printf("-------------------------------------------------------\n");
		//mesg[n] = 0;
		//printf("Received the following:\n");
		//printf("%s", mesg);
		//printf("-------------------------------------------------------\n");
	}

	return 0;
}

