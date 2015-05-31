/**
 * Connection
 *
 * @author	Stanislav Nechutný - xnechu01
 */
#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdbool.h>


typedef enum {
	P01Hello,
	P02Hi,
	P03Params,
	P04Login,
	P05UID,
	P06Result,
	P07End,
	P08OK,
	P10Error
} packetType;

typedef enum {
	reqNone,
	reqLogin,
	reqUID,
	reqGUID,
	reqName,
	reqHome,
	reqShell,
} requestedData;

typedef struct {
	char data[1];
} packetEmpty;

typedef struct {
	requestedData req[6];
} packetParams;

typedef struct {
	int data;
} packetUID;

typedef struct {
	char data[220];
} packetChars;

typedef struct {
	char data[220];
} packetError;


typedef struct {
	packetType type;
	union {
		packetEmpty empty;
		packetParams params;
		packetUID UID;
		packetChars chars;
		packetError error;
	} data;
} packet;

typedef union {
	char io[256];
	packet data;
} transfer;


class Connection
{
	protected:
		int _socket;
		struct sockaddr_in _sin;

	public:
		/**
		 * Connect
		 */
		bool connectTo(char* host, int port);

		bool bindServer(int port);

		Connection* acceptConnection();

		bool send(char* message, int length);

		int receive(char* message, int length);

		void disconnect();

		void injectSocket(int sock);
};
#endif
