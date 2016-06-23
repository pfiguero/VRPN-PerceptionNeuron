
#include <iostream>

#include "vrpn_Connection.h"
#include "vrpn_MxRTrafficGenerator.h"

int CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;

vrpn_Connection *connection;

vrpn_MxRTrafficGenerator_Server *trafficGenerator;
vrpn_MxRTrafficGenerator_Remote *trafficRemote;


void VRPN_CALLBACK handle_traffic_update(void*, const vrpn_MXRTRAFFIC_CALLBACK t)
{
	std::cout << "Received traffic update with N= " << t.car_count << "vehicles \n";
	for (int i = 0; i < t.car_count; i++)
	{
		std::cout<< "Car " << t.cars[i].ID << " in pos " << t.cars[i].pos_z <<"\n";
	}
}

void create_and_link_traffic_remote(void) {

	std::cout << "Starting traffic server remote... \n";
	trafficRemote = new vrpn_MxRTrafficGenerator_Remote("MyTraffic", connection);
	trafficRemote->register_trafficupdate_handler(NULL, handle_traffic_update);
}

int main(int argc, char * argv[])
{

	//open connection 
	std::cout << "Opening connection... \n";
	connection = vrpn_create_server_connection(CONNECTION_PORT);

	if (connection->doing_okay())
	{
		std::cout << "Starting traffic server... \n";
		trafficGenerator = new vrpn_MxRTrafficGenerator_Server("MyTraffic", connection, 0, 500, 1, 25, 2, 3.0, 4.0, 5.5);

	}
	create_and_link_traffic_remote();

	int test_frames_count = 20;

	trafficGenerator->startTrial();
	while (test_frames_count > 0)
	{
		if (test_frames_count < 3)
			trafficGenerator->finishTrial();
		trafficGenerator->mainloop();
		trafficRemote->mainloop();
	}

	delete trafficRemote;
	delete trafficGenerator;
	connection->removeReference();
}