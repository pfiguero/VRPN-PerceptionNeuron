//New VRPN device to contain traffic generator written by Yuan

#ifndef VRPN_MXRTRAFFICGENERATOR_H
#define VRPN_MXRTRAFFICGENERATOR_H

const int MAX_VEHICLES_PER_TRAFFIC_REPORT = 30;

#include <stddef.h>         //for NULL
#include <vector>

#include "vrpn_BaseClass.h"
#include "vrpn_Configure.h" // for VRPN_API, VRPN_CALLBACK
#include "vrpn_Shared.h"    // for timeval
#include "vrpn_Types.h"     // for vrpn_float64, vrpn_int32


class VRPN_API vrpn_Connection;
struct vrpn_HANDLERPARAM;


typedef struct _MXRTRAFFIC_CARDATA {

	vrpn_int32 ID;
	vrpn_float64 pos_z;

} MXRTRAFFIC_CARDATA;

class TrafficGenerator {

	//gloable variables that will be modified in init function to fit different traffic
	vrpn_float32 carLength = 2;
	vrpn_float32 gaps[3] = { 1,3,5 };

	typedef std::vector<MXRTRAFFIC_CARDATA> car_vec_simple;


	//used for inside calculation. 
	struct car {
		vrpn_int32 ID;//sequencially go up from 10000
		vrpn_float32 length = 2;
		vrpn_float64 posZ;//position along z, calc by car spawnInitPosZ+direction*distance
		vrpn_float32 gapSizeBehindMe;
		bool soonDelete = false;//soonbedeletecar(just used in this logic, happens fast, should not used outside to control unity cars)

								//debug
		vrpn_float64 creationTime = 0;
	};
	//typedef car car_vec[MAX_VEHICLES_PER_TRAFFIC_REPORT];//vecoter type containing all the active cars

	typedef std::vector<car> car_vec;//vecoter type containing all the active cars

public:

	void startTrial();
	void finishTrial();
	void init(vrpn_float32 trafficBoundSamallZ, vrpn_float32 trafficBoundLargeZ, bool startingTrafficDirectionPositiveZ, vrpn_float32 trafficSpeed, vrpn_float32 carLen, vrpn_float32 gapS, vrpn_float32 gapM, vrpn_float32 gapL);
	void Update();//center logic controlling the traffic, have to be called every iteration like a while(1)
	car_vec_simple activeCars_simple; //cars are the same ones as activeCars, just in simpler format for sending messages.

private:

	bool trafficON = false;//set to true to start a trial, set to false to end a trial 
	car_vec activeCars; //used in insideCalc, keep the active cars, if an obj not in this list, just delete that car

	bool trafficOnRem = false;
	vrpn_int32 lastCarIdRem = 9999;
	vrpn_float32 trafficSpawnPoint;//for just the current trial
	vrpn_float32 trafficDeletePoint;
	vrpn_float32 speed;

	vrpn_int32 trafficDirectionCur;

	void spawnACar(vrpn_float32 posZ, vrpn_float32 gapsize); //add a car into the carVec
	void refreshCarVec();//refreshe all the pos of the cars in vec based on current clock time
	void deleteFlaggedCars();//delete a car at the front the carVec
	void spawnCarsForNewTrial();//generate a list of cars when trial starts

	vrpn_float32 pickAGap();//pick a random gap size out of the const gaps list, return a float (second)
	vrpn_float64 timeElapsSinceLastNewCar(); //in clock ticks, devided by CLOCKS_PER_SEC to get seconds
	clock_t lastNewCarTimeRem;
	vrpn_float64 timeElapsSinceLastRefresh();
	clock_t lastRefreshTimeRem;
};


class VRPN_API vrpn_MxRTrafficGenerator : public vrpn_BaseClass {

public:
	
	vrpn_MxRTrafficGenerator(const char *name, vrpn_Connection *c = NULL);

protected:

	struct timeval timestamp;
	vrpn_int32 trafficreport_m_id; //Traffic report message id

	vrpn_int32 active_car_count;
	MXRTRAFFIC_CARDATA car_data[MAX_VEHICLES_PER_TRAFFIC_REPORT];
	

	virtual int register_types(void);
	virtual vrpn_int32 encode_to(char *buf);  //Encodes traffic report
	virtual void send_report(void);         // send report

};

class VRPN_API vrpn_MxRTrafficGenerator_Server : public vrpn_MxRTrafficGenerator {
public:
	vrpn_MxRTrafficGenerator_Server(const char *name, vrpn_Connection *c,
		vrpn_float32 trafficBoundSamallZ, vrpn_float32 trafficBoundLargeZ, 
		bool startingTrafficDirectionPositiveZ, vrpn_float32 trafficSpeed,
		vrpn_float32 carLen, vrpn_float32 gapS, vrpn_float32 gapM, vrpn_float32 gapL);
	virtual void mainloop();
	virtual void generate_report();
	void startTrial();
	void finishTrial();

	static vrpn_MxRTrafficGenerator_Server* getInstance()
	{
		return instance;
	}

private:

	vrpn_float64 _update_rate; //Update rate for traffic generation (reports/sec)
	TrafficGenerator _traffic;

	static vrpn_MxRTrafficGenerator_Server* instance;
};



//----------------------------------------------------------
//************** Users deal with the following *************

// User routine to handle updates from traffic generator.  This is called when
// new traffic report from its counterpart / across the connetion arrives).

typedef struct _vrpn_MXRTRAFFIC_CALLBACK {
	struct timeval msg_time;                                   // Traffic report timestamp
	vrpn_int32 car_count;                                        // Number of active cars
	MXRTRAFFIC_CARDATA cars[MAX_VEHICLES_PER_TRAFFIC_REPORT];
} vrpn_MXRTRAFFIC_CALLBACK;


typedef void(VRPN_CALLBACK *vrpn_MXRTRAFFICREPORTHANDLER)(void *userdata,
	const vrpn_MXRTRAFFIC_CALLBACK info);

// Opena traffic generator device that is on the other end of a connection
// and handle updates from it.  This is the type of device
// that user code will deal with.

class VRPN_API vrpn_MxRTrafficGenerator_Remote : public vrpn_MxRTrafficGenerator {

public:
	// The name of the device to connect to.
	// Optional argument to be used when the Remote MUST listen on
	// a connection that is already open.
	vrpn_MxRTrafficGenerator_Remote(const char *name, vrpn_Connection *c = NULL);
	~vrpn_MxRTrafficGenerator_Remote();

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop();

	// (un)Register a callback handler to handle traffic updates
	virtual int register_trafficupdate_handler(void *userdata,
		vrpn_MXRTRAFFICREPORTHANDLER handler)
	{
		return d_callback_list.register_handler(userdata, handler);
	};
	virtual int unregister_trafficupdate_handler(void *userdata,
		vrpn_MXRTRAFFICREPORTHANDLER handler)
	{
		return d_callback_list.unregister_handler(userdata, handler);
	}
protected:

	vrpn_Callback_List<vrpn_MXRTRAFFIC_CALLBACK> d_callback_list;

	static int VRPN_CALLBACK 
	handle_trafficreport_message(void *userdata, vrpn_HANDLERPARAM p);


};




#endif