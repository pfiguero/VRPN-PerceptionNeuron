
#include <iostream>

#include "vrpn_Connection.h"
#include "vrpn_MxRTrafficGenerator.h"

int CONNECTION_PORT = vrpn_DEFAULT_LISTEN_PORT_NO;

vrpn_Connection *connection;

vrpn_MxRTrafficGenerator_Remote *trafficRemote;


void VRPN_CALLBACK handle_traffic_update(void*, const vrpn_MXRTRAFFIC_CALLBACK t)
{
	std::cout << "Received traffic update with N= " << t.car_count << " vehicles \n";
	for (int i = 0; i < t.car_count; i++) 
	{
		std::cout<< "Car " << t.cars[i].ID << " in pos " << t.cars[i].pos_z <<"\n";
	}
}


int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <devName>\n", argv[0]);
		exit(-1);
	}
	char* devName = argv[1];
	//open connection 
	std::cout << "Opening connection... \n";
	trafficRemote = new vrpn_MxRTrafficGenerator_Remote(devName);
	trafficRemote->register_trafficupdate_handler(NULL, handle_traffic_update);

	while (1)
	{
		trafficRemote->mainloop();
	}

	delete trafficRemote;
}