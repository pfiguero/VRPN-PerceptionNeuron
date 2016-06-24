#include <stdio.h>
#include <vector>

#include "vrpn_Connection.h"
#include "vrpn_MxRTrafficGenerator.h"

const vrpn_int32 MBUFFER_SIZE = 1000;

vrpn_MxRTrafficGenerator_Server *vrpn_MxRTrafficGenerator_Server::instance = NULL;



vrpn_MxRTrafficGenerator::vrpn_MxRTrafficGenerator(const char *name, vrpn_Connection *c)
	: vrpn_BaseClass(name, c)
{
	vrpn_BaseClass::init();
	//car_data.reserve(MAX_VEHICLES_PER_TRAFFIC_REPORT);


}

vrpn_int32 vrpn_MxRTrafficGenerator::register_types(void)
{
	if (d_connection == NULL) {
		return 0;
	}

	trafficreport_m_id = d_connection->register_message_type("vrpn_MxRTrafficGenerator_Report");

	if (trafficreport_m_id == -1)
	{
		fprintf(stderr, "vrpn_MxRTrafficGenerator: Can't register message type ID\n");
		d_connection = NULL;
	}

	return 0;
}

vrpn_int32 vrpn_MxRTrafficGenerator::encode_to(char * buf)
{
	char *bufptr = buf;
	vrpn_int32 buflen = MBUFFER_SIZE;

	// Message includes: vrpn_int32 cars_count
	//plus MAX_VEHICLES_PER_TRAFFIC_REPORT x (vrpn_int32 car_ID vrpn_float64 pos_z)

	vrpn_buffer(&bufptr, &buflen, active_car_count);
	for (vrpn_int32 i = 0; i < active_car_count; i++)
	{
		vrpn_buffer(&bufptr, &buflen, car_data[i].ID);
		vrpn_buffer(&bufptr, &buflen, car_data[i].pos_z);
	}

	return MBUFFER_SIZE - buflen;
}

void vrpn_MxRTrafficGenerator::send_report(void)
{

	char msgbuf[MBUFFER_SIZE];
	vrpn_int32 i;
	vrpn_int32 len;

	if (d_connection) {
		
		len = encode_to(msgbuf);
		if (d_connection->pack_message(len, timestamp, trafficreport_m_id,
			d_sender_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
				fprintf(stderr, "vrpn_MxRTrafficGenerator: can't write message: tossing\n");
		}
	}
}

vrpn_MxRTrafficGenerator_Server::vrpn_MxRTrafficGenerator_Server(const char *name,
	vrpn_Connection *c,vrpn_float32 trafficBoundSamallZ, vrpn_float32 trafficBoundLargeZ,
	bool startingTrafficDirectionPositiveZ, vrpn_float32 trafficSpeed,
	vrpn_float32 carLen, vrpn_float32 gapS, vrpn_float32 gapM, vrpn_float32 gapL)
	: vrpn_MxRTrafficGenerator(name,c)
{
	_traffic.init(trafficBoundSamallZ, trafficBoundLargeZ,
		startingTrafficDirectionPositiveZ, trafficSpeed, carLen, gapS, gapM, gapL);
	_update_rate = 60;

	printf("vrpn_MxRTrafficGenerator_Server. Creating server with parameters: %s %f %f %s %f %f %f %f %f\n",
		name, trafficBoundSamallZ, trafficBoundLargeZ,
		startingTrafficDirectionPositiveZ ? "true" : "false",
		trafficSpeed, carLen, gapS, gapM, gapL);

	if (instance == NULL)
		instance = this;
}

void vrpn_MxRTrafficGenerator_Server::mainloop() {
	

	//printf("Server mainloop \n");
	struct timeval current_time;

	//Call generic server mainloop
	server_mainloop();

	vrpn_gettimeofday(&current_time, NULL);
	if (vrpn_TimevalDuration(current_time, timestamp) >=
		1000000.0 / _update_rate) {

		// Update the time
		timestamp.tv_sec = current_time.tv_sec;
		timestamp.tv_usec = current_time.tv_usec;

		//Generate new report
		_traffic.Update();
		

		//Send report
		generate_report();

	}

}

//Copy traffic data into message structure and send report
void vrpn_MxRTrafficGenerator_Server::generate_report() {
	active_car_count = _traffic.activeCars_simple.size();
	for (int i = 0; i < active_car_count; i++)
	{
		car_data[i] = _traffic.activeCars_simple[i];
	}

	send_report();
}

void vrpn_MxRTrafficGenerator_Server::startTrial() {
	_traffic.startTrial();
}


void vrpn_MxRTrafficGenerator_Server::finishTrial() {
	_traffic.finishTrial();
}

vrpn_MxRTrafficGenerator_Remote::vrpn_MxRTrafficGenerator_Remote(const char *name, vrpn_Connection *c)
	: vrpn_MxRTrafficGenerator(name, c)
{
	if (d_connection) {
		if (register_autodeleted_handler(trafficreport_m_id, handle_trafficreport_message, this, d_sender_id)) {
			fprintf(stderr, "vrpn_MxRTrafficGenerator_Remote: can't register handler\n");
			d_connection = NULL;
		}
	}
	vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_MxRTrafficGenerator_Remote::~vrpn_MxRTrafficGenerator_Remote()
{}

void vrpn_MxRTrafficGenerator_Remote::mainloop()
{
	client_mainloop();
	if (d_connection) {
		d_connection->mainloop();
	}
}

vrpn_int32 VRPN_CALLBACK vrpn_MxRTrafficGenerator_Remote::handle_trafficreport_message(void *userdata, vrpn_HANDLERPARAM p)
{
	vrpn_MxRTrafficGenerator_Remote *me = (vrpn_MxRTrafficGenerator_Remote *)userdata;
	vrpn_MXRTRAFFIC_CALLBACK cp;
	const char *bufptr = p.buffer;

	cp.msg_time = p.msg_time;
	vrpn_unbuffer(&bufptr, &cp.car_count);
	for (vrpn_int32 i = 0; i < MAX_VEHICLES_PER_TRAFFIC_REPORT; i++)
	{
		vrpn_unbuffer(&bufptr, &cp.cars[i].ID);
		vrpn_unbuffer(&bufptr, &cp.cars[i].pos_z);
		
	}
	
	// Go down the list of callbacks that have been registered.
	// Fill in the parameter and call each.
	me->d_callback_list.call_handlers(cp);

	return 0;

}

vrpn_MxRTrafficGenerator_ProxyServer:: vrpn_MxRTrafficGenerator_ProxyServer(const char * name, vrpn_Connection *c) : vrpn_MxRTrafficGenerator(name, c)
{}


void vrpn_MxRTrafficGenerator_ProxyServer::mainloop()
{
	server_mainloop();
}

void vrpn_MxRTrafficGenerator_ProxyServer::report_traffic_data(const struct timeval t, int car_count, const MXRTRAFFIC_CARDATA* vehicle_data)
{
	timestamp = t;
	active_car_count = car_count;
	memcpy(car_data, vehicle_data, sizeof(car_data));
	send_report();
}


void TrafficGenerator::init(vrpn_float32 trafficBoundSmallZ, vrpn_float32 trafficBoundLargeZ, bool startingTrafficDirectionPositiveZ, vrpn_float32 trafficSpeed, vrpn_float32 carLen, vrpn_float32 gapS, vrpn_float32 gapM, vrpn_float32 gapL) {
	//set random seed 
	time_t seed;
	time(&seed);
	srand((vrpn_uint32)seed);
	if (startingTrafficDirectionPositiveZ) {
		trafficSpawnPoint = trafficBoundSmallZ;
		trafficDeletePoint = trafficBoundLargeZ;
		trafficDirectionCur = 1;
	}
	else if (!startingTrafficDirectionPositiveZ) {
		trafficSpawnPoint = trafficBoundLargeZ;
		trafficDeletePoint = trafficBoundSmallZ;
		trafficDirectionCur = -1;
	}
	speed = trafficSpeed;
	//trafficON = true;
	trafficOnRem = false;
	//lastRefreshTimeRem = clock();
	lastCarIdRem = 9999;
	carLength = carLen;
	gaps[0] = gapS;
	gaps[1] = gapM;
	gaps[2] = gapL;
	activeCars.clear();
}

void TrafficGenerator::startTrial()
{
	if (!trafficON) {
		trafficON = true;
		lastRefreshTimeRem = clock();
	}
	
}

void TrafficGenerator::finishTrial()
{
	if (trafficON)
		trafficON = false;
}

void TrafficGenerator::Update() {
	refreshCarVec();
	if (trafficON && !trafficOnRem) {//if changed from off traffic to on traffic, new trial, generate whole bouch of cars fills up the road.
		spawnCarsForNewTrial();

		trafficOnRem = true;
	}
	else if (!trafficON && trafficOnRem) { //if change from on traffic to off traffic, end trial, clear all the cars
		activeCars.clear();
		activeCars_simple.clear();
		lastNewCarTimeRem = 0;
		trafficDirectionCur = -trafficDirectionCur;//flip the traffic direction
		vrpn_float32 temppos = trafficDeletePoint;//flip the traffic starting/ending position
		trafficDeletePoint = trafficSpawnPoint;
		trafficSpawnPoint = temppos;
		trafficOnRem = false;
	}
	else if (trafficON && trafficOnRem) { //normal traffic
		if ((!activeCars.empty() && timeElapsSinceLastNewCar() >= (activeCars.back().gapSizeBehindMe + carLength / speed)) || activeCars.empty()) { //have to use distance, not timeelaps, because the trial start cars are generated at time 0 but locates in middle of the road, half of lead carlen + gapsize*speed + half of rear carlen
			vrpn_float32 newgap = pickAGap();
			spawnACar(trafficSpawnPoint, newgap);
		}
		//refreshCarVec();
	}
	else {}
	//if receive "stop trial" command set trafficON to false, if receive "next trial" command set trafficON to true

	/*if (latPrintRem > 60000 && activeCars.size()>0){
	for (int i = 0; i < activeCars.size(); i++)
	cout << "   ID=" << activeCars[i].ID << " pos=" << activeCars[i].posZ << endl;
	cout << "**************" << endl<<endl;
	latPrintRem = 0;
	}
	latPrintRem++;*/
}

//generate a list of cars fill up the whole traffic delete point to spawn point
void TrafficGenerator::spawnCarsForNewTrial() {

	vrpn_float32 coveredDistance = 0;//the distanse that covered by cars & gaps starting from delete point,until the center of the last car
	while (coveredDistance < abs(trafficDeletePoint - trafficSpawnPoint)) {
		spawnACar(trafficDeletePoint - trafficDirectionCur*(coveredDistance + carLength / 2), pickAGap());
		coveredDistance = coveredDistance + carLength + activeCars.back().gapSizeBehindMe*speed;
	}
}
void TrafficGenerator::spawnACar(vrpn_float32 posZ, vrpn_float32 gapsize) {
	car newcar;
	newcar.ID = lastCarIdRem + 1; //each new car get a sequential id from 10000
	lastCarIdRem = newcar.ID;
	newcar.gapSizeBehindMe = gapsize;
	newcar.posZ = posZ;

	if (trafficON && !trafficOnRem)//generating for a new trial
	{
		lastNewCarTimeRem = (((vrpn_float64)clock()) / CLOCKS_PER_SEC - abs(posZ - trafficSpawnPoint) / speed)*CLOCKS_PER_SEC;
		newcar.creationTime = ((vrpn_float64)clock()) / CLOCKS_PER_SEC - abs(posZ - trafficSpawnPoint) / speed;
	}
	else if (trafficOnRem)//normal traffic
	{
		lastNewCarTimeRem = clock();
		newcar.creationTime = ((vrpn_float64)clock()) / CLOCKS_PER_SEC;
	}
	activeCars.push_back(newcar);
	/*if (activeCars.size() > 1){
	cout << newcar.ID << " " << newcar.posZ << "    " << activeCars[activeCars.size() - 2].ID << " " << activeCars[activeCars.size() - 2].posZ<<endl;
	cout <<" clock="<< newcar.creationTime<<" ID=" << newcar.ID << " gap to front=" << (abs(newcar.posZ - activeCars[activeCars.size() - 2].posZ)-carLength) / speed << " gap behind=" << newcar.gapSizeBehindMe << endl;
	}*/
}
void TrafficGenerator::deleteFlaggedCars() {
	car_vec temp;
	for (vrpn_int32 i = 0; i < activeCars.size(); i++) {
		if (!activeCars[i].soonDelete)
			temp.push_back(activeCars[i]);
	}
	activeCars.clear();
	activeCars = temp;
}
//all active cars move forward, flag the ones that should be deleted
void TrafficGenerator::refreshCarVec() {
	for (vrpn_int32 i = 0; i < activeCars.size(); i++) {
		activeCars[i].posZ = activeCars[i].posZ + trafficDirectionCur*timeElapsSinceLastRefresh()*speed;
		if ((trafficDirectionCur == 1 && activeCars[i].posZ >= trafficDeletePoint) || (trafficDirectionCur == -1 && activeCars[i].posZ <= trafficDeletePoint))
			activeCars[i].soonDelete = true;
	}
	deleteFlaggedCars();

	//update the activeCars_simple vector for message out
	activeCars_simple.clear();
	for (vrpn_int32 i = 0; i<activeCars.size(); i++) {
		MXRTRAFFIC_CARDATA newcar;
		newcar.ID = activeCars[i].ID;
		newcar.pos_z = activeCars[i].posZ;
		//newcar.gapSizeBehindMe = activeCars[i].gapSizeBehindMe;
		activeCars_simple.push_back(newcar);
	}

	lastRefreshTimeRem = clock();
}
double TrafficGenerator::timeElapsSinceLastNewCar() {
	return ((vrpn_float64)clock() - lastNewCarTimeRem) / CLOCKS_PER_SEC;
}
double TrafficGenerator::timeElapsSinceLastRefresh() {
	return ((vrpn_float64)clock() - lastRefreshTimeRem) / CLOCKS_PER_SEC;
}
//pick a random gap size out of the const gaps list
vrpn_float32 TrafficGenerator::pickAGap() {
	const vrpn_int32 randlow = 0;
	const vrpn_int32 randhigh = sizeof(gaps) / sizeof(*gaps) - 1; //generate random index for gaps to random select a gap 
	return gaps[rand() % (randhigh - randlow + 1) + randlow];
};


