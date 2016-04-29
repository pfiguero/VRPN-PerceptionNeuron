// client_and_server.C
//	This is a VRPN example program that has a client and server for a
// tracker both within the same thread.
//	The basic idea is to instantiate both a vrpn_Tracker_NULL and
// a vrpn_Tracker_Remote using the same connection for each. Then, the
// local call handlers on the connection will send the information from
// the server to the client callbacks.

// Pablo Figueroa
// Changing the code so I can try the forwarding behavior

#include <stdio.h>                      // for fprintf, stderr, printf, etc
#include <stdlib.h> // for atoi, exit

#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs
#include "vrpn_Tracker.h"               // for vrpn_TRACKERCB, etc
#include "vrpn_Types.h"                 // for vrpn_float64

char	*TRACKER_NAME = "Tracker0";
int	CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;	// Port for connection to listen on
char* remoteTracker = "Tracker0@localhost";

vrpn_Tracker_Server	*stkr;
vrpn_Tracker_Remote	*tkr;
vrpn_Connection		*connection;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void	VRPN_CALLBACK handle_pos (void *, const vrpn_TRACKERCB t)
{
	static	int	count = 0;

	// Register the message...
	stkr->report_pose(t.sensor, t.msg_time, t.pos, t.quat);

	fprintf(stderr, "%d.", t.sensor);
	if ((++count % 20) == 0) {
		fprintf(stderr, "\n");
		if (count > 300) {
			printf("Pos, sensor %d = %f, %f, %f\n", t.sensor,
				t.pos[0], t.pos[1], t.pos[2]);
			count = 0;
		}
	}
}

void	VRPN_CALLBACK handle_vel (void *, const vrpn_TRACKERVELCB t)
{
	//static	int	count = 0;

	fprintf(stderr, "%d/", t.sensor);
}

void	VRPN_CALLBACK handle_acc (void *, const vrpn_TRACKERACCCB t)
{
	//static	int	count = 0;

	fprintf(stderr, "%d~", t.sensor);
}

int main (int argc, char * argv [])
{
	int port;
	if (argc > 4) {
		fprintf(stderr, "Usage: %s [port remoteTracker localTrackerName]\n", argv[0]);
		return -1;
	}
	else if (argc == 4) {
		CONNECTION_PORT = atoi(argv[1]);
		remoteTracker = argv[2];
		TRACKER_NAME = argv[3];
	}
	// Else, leave port and remoteTracker as the default values

	// explicitly open the connection
	connection = vrpn_create_server_connection(CONNECTION_PORT);

	// Open the tracker server, using this connection, 60 sensors, update 60 times/sec
	// This is the one accepting local connections
	// Testing for the Neuron
	stkr = new vrpn_Tracker_Server(TRACKER_NAME, connection, 60);

	// Open the tracker remote
	fprintf(stderr, "Tracker's name is %s.\n", remoteTracker);
	tkr = new vrpn_Tracker_Remote(remoteTracker);

	// Set up the tracker callback handlers
	printf("Tracker update: '.' = pos, '/' = vel, '~' = acc\n");
	tkr->register_change_handler(NULL, handle_pos);
	tkr->register_change_handler(NULL, handle_vel);
	tkr->register_change_handler(NULL, handle_acc);

	/* 
	 * main interactive loop
	 */
	while ( 1 ) {
		// Let the tracker server, client and connection do their things
		tkr->mainloop();
		stkr->mainloop();
		connection->mainloop();

		// Sleep for 1ms so we don't eat the CPU
		vrpn_SleepMsecs(1);
	}

	return 0;

}   /* main */


