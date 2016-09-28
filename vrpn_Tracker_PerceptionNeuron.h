#ifndef VRPN_TRACKER_PERCEPTION_NEURON_H
#define VRPN_TRACKER_PERCEPTION_NEURON_H

#include "vrpn_Configure.h"   // IWYU pragma: keep

#ifdef  VRPN_INCLUDE_PERCEPTION_NEURON
#include <map>
#include <vector>

#include "vrpn_Shared.h"
#include "vrpn_Tracker.h"

#include "NeuronDataReader.h"


class VRPN_API vrpn_Tracker_PerceptionNeuron : public vrpn_Tracker {
public:
	vrpn_Tracker_PerceptionNeuron(const char *name,
		vrpn_Connection *c,
		const char* device,
		const char* protocol,
		int port);

	~vrpn_Tracker_PerceptionNeuron();

	virtual void mainloop();

	void handleData(BvhDataHeader* header, float* data);
	bool enableTracker(bool enable);
	void send_report(void);

	/// Handlers
	static int VRPN_CALLBACK
		handle_first_connection_message(void *userdata, vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK
		handle_got_connection_message(void *userdata, vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK
		handle_connection_dropped_message(void *userdata, vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK
		handle_dropped_last_connection_message(void *userdata, vrpn_HANDLERPARAM p);

private:
	const int BVHBoneCount = 59;
	int tcpPort;
	// UDP doesn't work yet... 
	int udpPort;

	/// File-static constant of max line size. From vrpn_Generic_server_object.h
	static const int LINESIZE = 512;

	char ipNeuron[LINESIZE]; // BRConnectTo asks for char*, not const char*... :(
};


/*






#include "vrpn_Shared.h"
#include "vrpn_Tracker.h"



bool addMarker(int sensor,int led_id);
bool addRigidMarker(int sensor, int led_id, float x, float y, float z);
bool startNewRigidBody(int sensor);
bool enableTracker(bool enable);
void setFrequency(float freq);

static int VRPN_CALLBACK handle_update_rate_request(void *userdata, vrpn_HANDLERPARAM p);

protected:

int numRigids;
int numMarkers;
bool owlRunning;
float frequency;
bool readMostRecent;
bool slave;
int frame;

typedef std::map<int, vrpn_int32> RigidToSensorMap;
RigidToSensorMap r2s_map;

protected:
int read_frame(void);

virtual int get_report(void);
virtual void send_report(void);

*/

#endif

#endif
