#include "connection.cpp"

#define BUFFER_SIZE 256

void handleConnection(Connection* incomming);
bool searchInPasswdLogin(packetParams params, char* sLogin, char* message);
bool searchInPasswdUID(packetParams params, int UID, char* message);



int main(int argc, char* argv[])
{
	if(argc < 3 || strcmp(argv[1], "-p") != 0)
	{
		fprintf(stderr, "Arg error\n");
		return -1;
	}
	Connection* server = new Connection;

	if(!server->bindServer(atoi(argv[2])))
	{
		fprintf(stderr, "Error\n");
		return -1;
	}

	Connection* incomming;
	int forkId = 0;
	while(incomming = server->acceptConnection())
	{
		forkId = fork();
		if(forkId == 0)
		{
			handleConnection(incomming);
			exit(0);
		}
		/* Disconnect also from parent for correct disconnecting */
		incomming->disconnect();
	}

	server->disconnect();

	return 0;
}


void handleConnection(Connection* incomming)
{
	char* message = (char *)malloc(sizeof(char)*220);

	transfer data;
	packetParams params;

	int len = 0;
	int i = 0;
	do
	{
		memset(message, 0, BUFFER_SIZE);

		len = incomming->receive(data.io, BUFFER_SIZE);

		switch(data.data.type)
		{
			case P01Hello:
				data.data.type = P02Hi;
				break;

			case P03Params:
				for(int k = 0; k < 6; k++)
				{
					params.req[k] = data.data.data.params.req[k];
				}
				data.data.type = P08OK;
				break;

			case P04Login:
				data.data.type = P06Result;
				if(!searchInPasswdLogin(params, data.data.data.chars.data, message))
				{
					data.data.type = P10Error;
				}
				memcpy(&data.data.data.chars.data, message, 220*sizeof(char));
				break;

			case P05UID:
				data.data.type = P06Result;
				if(!searchInPasswdUID(params, data.data.data.UID.data, message))
				{
					data.data.type = P10Error;
				}

				memcpy(&data.data.data.chars.data, message, 220*sizeof(char));
				break;

			case P07End:
				incomming->disconnect();
				return;
		}

		incomming->send(data.io, BUFFER_SIZE);
	}
	while(len > 0);

	incomming->disconnect();

	free(message);
}



bool searchInPasswdUID(packetParams params, int UID, char* message)
{

	FILE* fp = fopen("/etc/passwd", "r");
	char *login = (char*)malloc(sizeof(char)*64);
	char *name = (char*)malloc(sizeof(char)*64);
	char *shell = (char*)malloc(sizeof(char)*64);
	char *home = (char*)malloc(sizeof(char)*64);
	memset(login, 0, sizeof(char)*64);
	memset(name, 0, sizeof(char)*64);
	memset(shell, 0, sizeof(char)*64);
	memset(home, 0, sizeof(char)*64);
	int guid = 0;
	int uid = 0;

	int ret = 1, i = 0, k = 0;
	char c;

	while(1)
	{
		c = fgetc(fp);
		if(feof(fp))
		{
			break;
		}

		if(c == ':' || i >= 64)
		{
			ret++;
			i = 0;
		}
		else if(c != '\n')
		{
			if(ret == 1)
			{
				login[i] = c;
			}
			else if(ret == 2)
			{
			}
			else if(ret == 3)
			{
				uid = uid*10 + c-'0';
			}
			else if(ret == 4)
			{
				guid = guid*10 + c-'0';
			}
			else if(ret == 5)
			{
				name[i] = c;
			}
			else if(ret == 6)
			{
				home[i] = c;
			}
			else if(ret == 7)
			{
				shell[i] = c;
			}
			i++;
		}
		else
		{
			if(UID == uid)
			{ // found
				memset(message, 0, sizeof(char)*220);
				for(k = 0; k < 6; k++)
				{
					if(params.req[k] == reqLogin)
					{
						sprintf(message, "%s %s", message, login);
					}
					else if(params.req[k] == reqUID)
					{
						sprintf(message, "%s %d", message, uid);
					}
					else if(params.req[k] == reqGUID)
					{
						sprintf(message, "%s %d", message, guid);
					}
					else if(params.req[k] == reqName)
					{
						sprintf(message, "%s %s", message, name);
					}
					else if(params.req[k] == reqHome)
					{
						sprintf(message, "%s %s", message, home);
					}
					else if(params.req[k] == reqShell)
					{
						sprintf(message, "%s %s", message, shell);
					}
				}

				return true;
			}
			ret = 1;
			i = 0;
			memset(login, 0, sizeof(char)*64);
			memset(name, 0, sizeof(char)*64);
			memset(shell, 0, sizeof(char)*64);
			memset(home, 0, sizeof(char)*64);
			guid = 0;
			uid = 0;
		}
	}

	strcpy(message, "UID not found");
	return false;
}

bool searchInPasswdLogin(packetParams params, char* sLogin, char* message)
{
	FILE* fp = fopen("/etc/passwd", "r");
	char *login = (char*)malloc(sizeof(char)*64);
	char *name = (char*)malloc(sizeof(char)*64);
	char *shell = (char*)malloc(sizeof(char)*64);
	char *home = (char*)malloc(sizeof(char)*64);
	memset(login, 0, sizeof(char)*64);
	memset(name, 0, sizeof(char)*64);
	memset(shell, 0, sizeof(char)*64);
	memset(home, 0, sizeof(char)*64);
	int guid = 0;
	int uid = 0;

	int ret = 1, i =0, k;
	char c;

	while(1)
	{
		c = fgetc(fp);
		if(feof(fp))
		{
			break;
		}

		if(c == ':' || i >= 64)
		{
			ret++;
			i = 0;
		}
		else if(c != '\n')
		{
			if(ret == 1)
			{
				login[i] = c;
			}
			else if(ret == 2)
			{
			}
			else if(ret == 3)
			{
				uid = uid*10 + c-'0';
			}
			else if(ret == 4)
			{
				guid = guid*10 + c-'0';
			}
			else if(ret == 5)
			{
				name[i] = c;
			}
			else if(ret == 6)
			{
				home[i] = c;
			}
			else if(ret == 7)
			{
				shell[i] = c;
			}
			i++;
		}
		else
		{
			if(strcasecmp(login, sLogin) == 0)
			{ // found
				memset(message, 0, sizeof(char)*BUFFER_SIZE);
				for(k = 0; k < 6; k++)
				{
					if(params.req[k] == reqLogin)
					{
						sprintf(message, "%s %s", message, login);
					}
					else if(params.req[k] == reqUID)
					{
						sprintf(message, "%s %d", message, uid);
					}
					else if(params.req[k] == reqGUID)
					{
						sprintf(message, "%s %d", message, guid);
					}
					else if(params.req[k] == reqName)
					{
						sprintf(message, "%s %s", message, name);
					}
					else if(params.req[k] == reqHome)
					{
						sprintf(message, "%s %s", message, home);
					}
					else if(params.req[k] == reqShell)
					{
						sprintf(message, "%s %s", message, shell);
					}
				}

				return true;
			}
			ret = 1;
			i = 0;
			memset(login, 0, sizeof(char)*64);
			memset(name, 0, sizeof(char)*64);
			memset(shell, 0, sizeof(char)*64);
			memset(home, 0, sizeof(char)*64);
			guid = 0;
			uid = 0;
		}
	}

	strcpy(message, "Login not found");
	return false;
}
