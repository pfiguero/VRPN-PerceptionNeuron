// text_server_test.C
// A test for text generation
// It generates numbers as strings every deltaTime
// Created by Pablo Figueroa, PhD
// Universidad de los Andes, Colombia

#define _USE_MATH_DEFINES // for C
#include <math.h>
#include <stdio.h>                      // for fprintf, stderr, printf, etc
#include <stdlib.h> // for atoi, exit

#include "quat.h"

#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs
#include "vrpn_Text.h"
#include "vrpn_Tracker.h"
#include "vrpn_Types.h"                 // for vrpn_float64

#define NUM_SENSORS 6

char	*TEXT_NAME = "StrGenerator";
char	*TRACKER_NAME = "Tracker0";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on

vrpn_Connection		*connection;
vrpn_Text_Sender	*txtSender;
vrpn_Tracker_Server	*trackerServer;

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

	timeval t;
	vrpn_float64 position[NUM_SENSORS][3];
	vrpn_float64 quaternion[NUM_SENSORS][4];

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
	txtSender = new vrpn_Text_Sender(TEXT_NAME, connection);

	// Create the tracker server, using this connection, 60 sensors, update 60 times/sec
	trackerServer = new vrpn_Tracker_Server(TRACKER_NAME, connection, NUM_SENSORS);

	srand(NUM_SENSORS);
	for (int i = 0; i < NUM_SENSORS; i++)
	{
		position[i][0] = (rand()*10.0) / RAND_MAX;
		position[i][1] = (rand()*10.0) / RAND_MAX;
		position[i][2] = (rand()*10.0) / RAND_MAX;

		q_make(quaternion[i], (rand()*1.0) / RAND_MAX, (rand()*1.0) / RAND_MAX, (rand()*1.0) / RAND_MAX,
			(rand()*1.0) / RAND_MAX * 2.0 * M_PI - M_PI);
	}

	printf("%s Ready\n", TRACKER_NAME);
	/* 
	 * main interactive loop
	 */
	while ( 1 ) {
		vrpn_gettimeofday(&t, NULL);

		sprintf(msg, "%ld", counter++);
		txtSender->send_message(msg);
		printf("Send [%s]\n", msg);
		for (int i = 0; i < NUM_SENSORS; i++)
		{
			trackerServer->report_pose(i, t, position[i], quaternion[i]);
		}

		// Let all servers and connection do their things
		txtSender->mainloop();
		trackerServer->mainloop();

		connection->mainloop();

		// Sleep a while so we don't eat the CPU
		vrpn_SleepMsecs(16);
	}

	return 0;

}   /* main */


