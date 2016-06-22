// text_server_test.C
// A test for text generation
// It generates numbers as strings every deltaTime
// Created by Pablo Figueroa, PhD
// Universidad de los Andes, Colombia

#include <stdio.h>                      // for fprintf, stderr, printf, etc
#include <stdlib.h> // for atoi, exit

#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs
#include "vrpn_Text.h"               // for vrpn_TRACKERCB, etc
#include "vrpn_Types.h"                 // for vrpn_float64

char	*TRACKER_NAME = "StrGenerator";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on

vrpn_Connection		*connection;
vrpn_Text_Sender	*txtSender;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

int main (int argc, char * argv [])
{
	int port;
	long int counter=0;
	char msg[256];

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		return -1;
	}
	else if (argc == 2) {
		CONNECTION_PORT = atoi(argv[1]);
	}
	// Else, leave port and remoteTracker as the default values

	// explicitly open the connection
	connection = vrpn_create_server_connection(CONNECTION_PORT);

	// Create the text generator
	txtSender = new vrpn_Text_Sender(TRACKER_NAME, connection);

	printf("%s Ready\n", TRACKER_NAME);
	/* 
	 * main interactive loop
	 */
	while ( 1 ) {

		sprintf(msg, "%ld", counter++);
		txtSender->send_message(msg);
		printf("Send [%s]\n", msg);

		// Let the tracker server, client and connection do their things
		txtSender->mainloop();
		connection->mainloop();

		// Sleep a while so we don't eat the CPU
		vrpn_SleepMsecs(16);
	}

	return 0;

}   /* main */


