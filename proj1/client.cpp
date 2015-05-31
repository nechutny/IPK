#include "connection.cpp"

#define BUFFER_SIZE 256

int main(int argc, char* argv[])
{
	Connection* server = new Connection;
	char* host = (char*)malloc(sizeof(char)*255);
	int port = 0;

	for(int i = 0; i < argc; i++)
	{
		if(strcmp(argv[i], "-p") == 0)
		{
			if(argc <= i+1)
			{
				fprintf(stderr, "Arg error\n");
				return -1;
			}
			port = atoi(argv[i+1]);
		}
		else if(strcmp(argv[i], "-h") == 0)
		{
			if(argc <= i+1)
			{
				fprintf(stderr, "Arg error\n");
				return -1;
			}
			strcpy(host, argv[i+1]);
		}
	}

	if(!server->connectTo(host, port))
	{
		fprintf(stderr, "Connect error\n");
		return -1;
	}

	transfer data;

	// Hello
	data.data.type = P01Hello;
	server->send(data.io, BUFFER_SIZE);
	server->receive(data.io, BUFFER_SIZE);

	// Params
	data.data.type = P03Params;
	data.data.data.params.req[0] = data.data.data.params.req[1] = data.data.data.params.req[2] = data.data.data.params.req[3] = data.data.data.params.req[4] = data.data.data.params.req[5] = reqNone;

	int j = 0;
	for(int i = 0; i < argc; i++)
	{
		if(j >= 6)
		{
			break;
		}
		if(strcmp(argv[i], "-L") == 0)
		{
			data.data.data.params.req[j] = reqLogin;
			j++;
		}
		if(strcmp(argv[i], "-U") == 0)
		{
			data.data.data.params.req[j] = reqUID;
			j++;
		}
		if(strcmp(argv[i], "-G") == 0)
		{
			data.data.data.params.req[j] = reqGUID;
			j++;
		}
		if(strcmp(argv[i], "-N") == 0)
		{
			data.data.data.params.req[j] = reqName;
			j++;
		}
		if(strcmp(argv[i], "-H") == 0)
		{
			data.data.data.params.req[j] = reqHome;
			j++;
		}
		if(strcmp(argv[i], "-S") == 0)
		{
			data.data.data.params.req[j] = reqShell;
			j++;
		}
	}

	server->send(data.io, BUFFER_SIZE);

	// Request for data
	int uid = 0;
	int login = 0;
	int num = 0;
	for(int i = 0; i < argc; i++)
	{
		if(uid)
		{
			if(argv[i][0] == '-')
			{
				uid = 0;

			}
			else
			{
				data.data.type = P05UID;
				data.data.data.UID.data = atoi(argv[i]);
				server->send(data.io, BUFFER_SIZE);
				num++;
			}
		}
		if(login)
		{
			if(argv[i][0] == '-')
			{
				login = 0;
			}
			else
			{
				data.data.type = P04Login;
				sprintf((char*)&data.data.data.chars.data, "%s", argv[i]);
				server->send(data.io, BUFFER_SIZE);
				num++;
			}
		}

		if(strcmp(argv[i],"-u") == 0)
		{
			uid = 1;
		}
		if(strcmp(argv[i],"-l") == 0)
		{
			login = 1;
		}
	}

	// Receive data
	int recv = 0;
	do
	{
		recv = server->receive(data.io, BUFFER_SIZE);
		if(data.data.type == P06Result)
		{
			num--;
			printf("%s\n", data.data.data.chars.data+1);
		}
		if(data.data.type == P10Error)
		{
			fprintf(stderr, "Error: %s\n", data.data.data.chars.data);
			num--;
		}
		if(num == 0)
		{ // Everything received, so FINISH HER!!!
			data.data.type = P07End;
			server->send(data.io, BUFFER_SIZE);
		}
	} while(recv > 0);

	server->disconnect();

	return 0;
}
