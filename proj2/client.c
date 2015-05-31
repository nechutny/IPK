#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>


#define OUTPUT_FILE_PREFIX "ipkperf-"
#define MAX_SIZE	65535

// Structures
typedef struct
{
	int port;
	int size;
	int rate;
	unsigned int timeout;
	int interval;
	char* server;
	int pid;
} arguments;


// Prototypes
arguments parseArguments(int argc, char* argv[]);
void sendData(arguments arg);
void receiveData(arguments arg);
void term(int sig);
uint32_t getTimeDiff();
int hostnameToIp(char * hostname , char* ip);


int sockfd;
struct sockaddr_in servaddr, cliaddr;
uint32_t count = 0, rtt_sum = 0, out_of_order = 0, diff_min = -1, diff_max = 0, time_fix = 0;
struct timeval startTime;
char file_time[20];
arguments arg;


int main(int argc, char**argv)
{
	time_t ti;
	struct tm *foo;
	time( &ti);
	foo = localtime( &ti );
	strftime(file_time, 20, "%Y-%m-%d-%H:%M:%S", foo);

	// Register SIGINT handler
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGUSR1, &action, NULL);

	arg = parseArguments(argc, argv);

	gettimeofday(&startTime, NULL);

	sockfd = socket(AF_INET,SOCK_DGRAM, 0);

	int res = hostnameToIp(arg.server, arg.server);
	if(res != 0)
	{
		fprintf(stderr, "Error: Could not translate hostname to IP.");
		exit(1);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(arg.server);
	servaddr.sin_port=htons(arg.port);

	arg.pid = fork();
	if(arg.pid == 0)
	{
		receiveData(arg);
	}
	else if(arg.pid > 0)
	{
		sendData(arg);
	}
	else
	{
		fprintf(stderr, "Error: Fork error.\n");
		exit(1);
	}

	term(SIGINT);

	return 0;
}


/**
 * Calculate time diff in microsends from now
 *
 * Required set global variable startTime
 *
 * @return	uint32_t	Diff
 */
uint32_t getTimeDiff()
{
	struct timeval now;
	gettimeofday(&now, NULL);

	uint32_t result = (((unsigned long)now.tv_sec * 1000000) + now.tv_usec) - (((unsigned long)startTime.tv_sec * 1000000) + startTime.tv_usec);

	return result;
}


/**
 * Main loop sending data to server and signals to fork child
 *
 * @param	arguments	arg	Arguments structure
 */
void sendData(arguments arg)
{
	char *sendline = malloc(arg.size);
	memset(sendline, 1,  arg.size);

	uint32_t time = 0;

	int sleep = 1000000/arg.rate;

	unsigned tout = arg.timeout*1000000;

	do
	{
		memcpy(sendline, &count, sizeof(count));
		time = getTimeDiff();
		//printf("Send data %u\n", count);
		memcpy(sendline+4, &time, sizeof(time));

		sendto(sockfd, sendline, arg.size, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

		count++;
		if(count%(arg.interval*arg.rate) == 0)
		{
			kill(arg.pid, SIGUSR1);
		}
		usleep(sleep);

	}
	while(tout > time);

	if(count%(arg.interval*arg.rate) != 0)
	{
		kill(arg.pid, SIGUSR1);
	}

	free(sendline);
}


/**
 * Receive data from server and set corresponding values to global variables
 *
 * @param	arguments	arg	Arguments structure
 */
void receiveData(arguments arg)
{
	char *recvline = malloc(arg.size+1);
	memset(recvline, 0,  arg.size);
	int n = 1;
	uint32_t tim = 0, seq = 0, last_seq = 0, diff = 0;
	count = -1;
	rtt_sum = 0;
	out_of_order = 0;
	diff_min = -1;
	diff_max = 0;
	uint32_t last_tim = 0;

	struct timeval tv;
	tv.tv_sec = 10;  /* 10 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));



	while (n != 0)
	{
		n = recvfrom(sockfd, recvline, arg.size, 0, NULL, NULL);
		count++;
		memcpy(&seq, recvline, 4);
		memcpy(&tim, recvline+4, 4);

		if(seq != last_seq && last_tim != tim)
		{
			diff = getTimeDiff()-tim;
			if(seq < last_seq)
			{
				out_of_order++;
			}
			else
			{
				last_seq = seq;
			}

			printf("Received #%d\t- #%d\t- %d.%dms\n", count, seq, (diff/1000), (diff%1000));
			time_fix = 0;
			rtt_sum += diff;
			if(diff_min > diff)
			{
				diff_min = diff;
			}
			if(diff_max < diff)
			{
				diff_max = diff;
			}
			last_tim = tim;
		}
	}

	free(recvline);
}


/**
 * Convert hostname to IP
 *
 * @param	char*	hostname	Pointer to char containing hostname to translate
 * @param	char*	ip		Pinter to allocated char used as return of tranlsated ip
 *
 * @return	int	0 - OK, 1 - Fail
 *
 */
int hostnameToIp(char * hostname , char* ip)
{
	//int sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *h;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0)
	{
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		h = (struct sockaddr_in *) p->ai_addr;
		strcpy(ip , inet_ntoa( h->sin_addr ) );
	}

	freeaddrinfo(servinfo); // all done with this structure
	return 0;
}


/**
 * Handle SIGINT and close socket and SIGUSR1
 *
 * For SIGUSR1 do calculations for correct output
 *
 * @param	int	sig	Signal code
 */
void term(int sig)
{
	if(sig == SIGUSR1)
	{
		printf("=== [ Interval %d/%d ] === \n", count, arg.interval*arg.rate);
		time_fix = getTimeDiff();
		time_t ti;
		struct tm *foo;
		time( &ti);
		foo = localtime( &ti );
		char tim[20];
		strftime(tim, 20, "%Y-%m-%d-%H:%M:%S", foo);

		char *filename = malloc(sizeof(char)*96);

		sprintf(filename, "%s%s-%d-%d", OUTPUT_FILE_PREFIX, arg.server, arg.size, arg.rate);

		FILE* fp = fopen(filename, "a");
		// calculate interval
		fprintf(fp, "%s, %d, %d, %d, %d, %d, %.3f, %.3f, %.3f\n",
			tim,							// Time
			arg.interval,						// IntLen
			arg.interval*arg.rate,					// Pckt Sent
			count,							// Pckt Recv
			arg.interval*arg.rate*arg.size,				// Bytes Send
			count*arg.size,						// Bytes Recv
			(((double)rtt_sum)/count)/1000,				// Avg Delay
			((double)(diff_max-diff_min)/1000),			// Jitter PDV(x) = t(x) - A(x)
			(out_of_order == 0) ? 0.0 : (out_of_order/count)	// OutOfOrder %
		);
		fclose(fp);
		free(filename);
		count = -1;
		rtt_sum = 0;
		out_of_order = 0;
		diff_min = -1;
		diff_max = 0;
		time_fix = getTimeDiff()-time_fix;
	}
	else
	{
		close(sockfd);
		//printf("Exiting\n");
		exit(0);
	}
}


/**
 * Parse program arguments to structure and validate them
 *
 * On error print message to stderr and exit program with code 1
 *
 * @param	int	argc	Number of arguments
 * @param	char*	argv	Array of arguments
 *
 * @return	arguments	Set structure
 */
arguments parseArguments(int argc, char* argv[])
{
	arguments result;

	result.port = 5656;
	result.size = 100;
	result.rate = 10;
	result.timeout = -1;
	result.interval = -1;

	// 1 for skip app name
	int i;
	for(i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0)
		{
			result.port = atoi(argv[i+1]);
			if(result.port <= 0 || result.port > 65535)
			{
				fprintf(stderr, "Error: Port number is out of range.\n");
				exit(1);
			}
			i++;
		}
		if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0)
		{
			result.size = atoi(argv[i+1]);
			if(result.size < 10)
			{
				fprintf(stderr, "Error: --size is too low.\n");
				exit(1);
			}
			if(result.size > MAX_SIZE-1)
			{
				fprintf(stderr, "Error: --size is too hight.\n");
				exit(1);
			}
			i++;
		}
		if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--rate") == 0)
		{
			result.rate = atoi(argv[i+1]);
			if(result.rate < 1)
			{
				fprintf(stderr, "Error: --rate is too low.\n");
				exit(1);
			}
			i++;
		}
		if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0)
		{
			result.timeout = atoi(argv[i+1]);
			if(result.timeout < 1)
			{
				fprintf(stderr, "Error: --timeout is too low.\n");
				exit(1);
			}
			i++;
		}
		if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0)
		{
			result.interval = atoi(argv[i+1]);
			if(result.interval < 1)
			{
				fprintf(stderr, "Error: --interval is too low.\n");
				exit(1);
			}
			i++;
		}
	}

	result.server = argv[argc-1];
	if(argc < 2)
	{
		fprintf(stderr, "Error: Specify at least server hostname/ip.\n");
		exit(1);
	}

	// interval not set, so calculate default value (100/rate)
	if(result.interval == -1)
	{
		result.interval = 100/result.rate;
	}

	return result;
}
