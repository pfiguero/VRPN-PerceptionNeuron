#include "quat.h"
#include "vrpn_Tracker_PerceptionNeuron.h"

#define SCALE_AXIS (0.01)


#ifdef VRPN_INCLUDE_PERCEPTION_NEURON
//#define DEBUG

void __stdcall bvhFrameDataReceived(void* customedObj, SOCKET_REF sender, BvhDataHeader* header, float* data);
void __stdcall CalcFrameDataReceive(void* customedObj, SOCKET_REF sender, CalcDataHeader* header, float* data);

class PerceptionNeuronHandler
{
public:
	static PerceptionNeuronHandler* getInstance()
	{
		if (singleton == NULL)
		{
			singleton = new PerceptionNeuronHandler();
		}
		return singleton;
	}
	bool enable(vrpn_Tracker_PerceptionNeuron* h, char* ipNeuron, int tcpPort, int udpPort)
	{
		bool resp = false;

		if (handler != NULL)
		{
			if (handler == h)
			{
				fprintf(stderr, "Perception Neuron: Registering the same handler at least twice!\n");
			}
			else
			{
				fprintf(stderr, "Perception Neuron: Registering more than one handler!\n");
			}
		}
		handler = h;
		if (tcpPort != -1 && sockTCPRef == NULL)
		{
			sockTCPRef = BRConnectTo(ipNeuron, tcpPort);
			if (sockTCPRef)
			{
				resp = true;
			}
		}
		else if (udpPort != -1 && sockUDPRef == NULL)
		{
			sockUDPRef = BRStartUDPServiceAt(udpPort);
			if (sockUDPRef)
			{
				resp = true;
			}
		}
		return resp;
	}
	bool disable()
	{
		bool resp = false;

		handler = NULL;
		if (sockTCPRef)
		{
			BRCloseSocket(sockTCPRef);
			sockTCPRef = NULL;
			resp = true;
		}
		else if (sockUDPRef)
		{
			BRCloseSocket(sockUDPRef);
			sockUDPRef = NULL;
			resp = true;
		}
		return resp;
	}
	void handleData(SOCKET_REF sender, BvhDataHeader* header, float* data)
	{
		if (handler != NULL && BRGetSocketStatus(sender) == CS_Running)
		{
			handler->handleData(header, data);
		}
	}
private:
	static PerceptionNeuronHandler* singleton;

	vrpn_Tracker_PerceptionNeuron* handler;
	SOCKET_REF sockTCPRef;
	SOCKET_REF sockUDPRef;

	PerceptionNeuronHandler()
	{
		handler = NULL;
		sockTCPRef = NULL;
		sockUDPRef = NULL;

		BRRegisterFrameDataCallback(this, bvhFrameDataReceived);
		BRRegisterCalculationDataCallback(this, CalcFrameDataReceive);

	};
};

PerceptionNeuronHandler* PerceptionNeuronHandler::singleton = NULL;

void __stdcall bvhFrameDataReceived(void* customedObj, SOCKET_REF sender, BvhDataHeader* header, float* data)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	PerceptionNeuronHandler* obj = static_cast<PerceptionNeuronHandler*>(customedObj);
	obj->handleData(sender, header, data);
}

void __stdcall CalcFrameDataReceive(void* customedObj, SOCKET_REF sender, CalcDataHeader* header, float* data)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	printf("CalcFrameDataReceive\n");
}

int vrpn_Tracker_PerceptionNeuron::handle_first_connection_message(void *userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	// Do nothing for the moment...
	return 0;
}

int vrpn_Tracker_PerceptionNeuron::handle_got_connection_message(void *userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	// Do nothing for the moment...
	return 0;
}

int vrpn_Tracker_PerceptionNeuron::handle_connection_dropped_message(void *userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	/*
	vrpn_Tracker_PerceptionNeuron *me = static_cast<vrpn_Tracker_PerceptionNeuron *>(userdata);
	// it still crashes sometimes... I guess more exceptions have to be handled, plus something related to the UDP connection...
	delete me;
	*/
	// Do nothing for the moment...
	return 0;
}

int vrpn_Tracker_PerceptionNeuron::handle_dropped_last_connection_message(void *userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	// Do nothing for the moment...
	return 0;
}

vrpn_Tracker_PerceptionNeuron::vrpn_Tracker_PerceptionNeuron(const char *name, vrpn_Connection *c, const char* device, const char* protocol, int port)
: vrpn_Tracker(name, c)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif

	udpPort = tcpPort = -1;
	if (strcmp(protocol, "udp") == 0)
		udpPort = port;
	else
		tcpPort = port;

	strcpy(ipNeuron, device);

	// Register the handler for all messages
	register_autodeleted_handler( 
		c->register_message_type(vrpn_got_first_connection),
		handle_first_connection_message, this);
	register_autodeleted_handler(
		c->register_message_type(vrpn_got_connection),
		handle_got_connection_message, this);
	register_autodeleted_handler(
		c->register_message_type(vrpn_dropped_connection),
		handle_connection_dropped_message, this);
	register_autodeleted_handler(
		c->register_message_type(vrpn_dropped_last_connection),
		handle_dropped_last_connection_message, this);

}

//
vrpn_Tracker_PerceptionNeuron::~vrpn_Tracker_PerceptionNeuron()
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	enableTracker(false);
}

bool vrpn_Tracker_PerceptionNeuron::enableTracker(bool enable)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	bool resp = false;
	if (enable)
	{
		resp = PerceptionNeuronHandler::getInstance()->enable(this, ipNeuron, tcpPort, udpPort);
	}
	else
	{
		resp = PerceptionNeuronHandler::getInstance()->disable();
	}
	return resp;
}

void vrpn_Tracker_PerceptionNeuron::handleData(BvhDataHeader* header, float* data)
{
	int dataIndex = 0;
	int nr = 59;
	q_type destQuat;

	// From the NeuronDataReader Runtime API Documentation_D16.pdf
	// and Cdemo_MFCDlg::showBvhBoneInfo
	if (!d_connection)
		return;

	// it assumes the server is configured to send a reference and displacement
	// calculate data index for selected bone
	for (int curSel = 0; curSel < nr; curSel++)
	{
		if (header->WithDisp)
		{
			dataIndex = curSel * 6;
			if (header->WithReference)
			{
				dataIndex += 6;
			}

			// Displacement

			pos[0] = data[dataIndex + 0] * SCALE_AXIS;
			pos[1] = data[dataIndex + 1] * SCALE_AXIS;
			pos[2] = data[dataIndex + 2] * SCALE_AXIS;


			/*
			// Euler Angles
			d_quat[0] = -data[dataIndex + 5];
			d_quat[1] = -data[dataIndex + 3];
			d_quat[2] = data[dataIndex + 4];
			d_quat[3] = 1;
			*/

			// from axis come yrot, xrot, zrot
			//q_from_euler(destQuat, Q_DEG_TO_RAD(data[dataIndex + 4]), -Q_DEG_TO_RAD(data[dataIndex + 3]), -Q_DEG_TO_RAD(data[dataIndex + 5]));
			//q_from_euler(destQuat, -Q_DEG_TO_RAD(data[dataIndex + 5]), -Q_DEG_TO_RAD(data[dataIndex + 3]), Q_DEG_TO_RAD(data[dataIndex + 4]));
			// The same order as the Euler angles are sent below
			q_from_euler(destQuat, Q_DEG_TO_RAD(data[dataIndex + 3]), Q_DEG_TO_RAD(data[dataIndex + 4]), -Q_DEG_TO_RAD(data[dataIndex + 5]));
			//q_from_euler(destQuat, -Q_DEG_TO_RAD(data[dataIndex + 5]), 0,0);

			// After some checking on how Quaternion works in both Unity and VRPN
			d_quat[Q_X] = destQuat[Q_Z];
			d_quat[Q_Y] = destQuat[Q_Y];
			d_quat[Q_Z] = destQuat[Q_X];
			d_quat[Q_W] = destQuat[Q_W];

			if (header->AvatarIndex == 0)
			{
				d_sensor = curSel;
			}
			else
			{
				d_sensor = curSel + 60;
			}
		}
		else // ! (header->WithDisp)
		{
			if (curSel == 0)
			{
				dataIndex = 3;
				if (header->WithReference)
				{
					dataIndex += 6;
				}

				// Displacement. Send the Euler Angles, adjusted to the signs changed at the VRPNTracker.cs
				pos[0] = data[dataIndex + 0];
				pos[1] = data[dataIndex + 1];
				pos[2] = -data[dataIndex + 2];

				/*
				// Euler Angles
				d_quat[0] = -data[dataIndex + 5];
				d_quat[1] = -data[dataIndex + 3];
				d_quat[2] = data[dataIndex + 4];
				d_quat[3] = 1;
				*/

				// from axis come yrot, xrot, zrot
				//q_from_euler(destQuat, Q_DEG_TO_RAD(data[dataIndex + 1]), -Q_DEG_TO_RAD(data[dataIndex + 0]), -Q_DEG_TO_RAD(data[dataIndex + 2]));
				// not really necessary
				q_from_euler(destQuat, -Q_DEG_TO_RAD(data[dataIndex + 2]), -Q_DEG_TO_RAD(data[dataIndex + 0]), Q_DEG_TO_RAD(data[dataIndex + 1]));
				//q_from_euler(destQuat, -Q_DEG_TO_RAD(data[dataIndex + 2]), 0, 0);

				d_quat[Q_X] = destQuat[Q_X];
				d_quat[Q_Y] = destQuat[Q_Y];
				d_quat[Q_Z] = destQuat[Q_Z];
				d_quat[Q_W] = destQuat[Q_W];

				if (header->AvatarIndex == 0)
				{
					d_sensor = curSel;
				}
				else
				{
					d_sensor = curSel + 60;
				}
			}
			else
			{
				//dataIndex = curSel * 3;
				dataIndex = 3 + curSel * 3;
				if (header->WithReference)
				{
					dataIndex += 6;
				}

				// Displacement. Send the Euler Angles, adjusted to the signs changed at the VRPNTracker.cs
				pos[0] = data[dataIndex + 0];
				pos[1] = data[dataIndex + 1];
				pos[2] = -data[dataIndex + 2];

				/*
				// Euler Angles
				d_quat[0] = -data[dataIndex + 2];
				d_quat[1] = -data[dataIndex + 0];
				d_quat[2] = data[dataIndex + 1];
				d_quat[3] = 1;
				*/

				// from axis come yrot, xrot, zrot
				//q_from_euler(destQuat, Q_DEG_TO_RAD(data[dataIndex + 1]), -Q_DEG_TO_RAD(data[dataIndex + 0]), -Q_DEG_TO_RAD(data[dataIndex + 2]));
				// not really necessary
				q_from_euler(destQuat, -Q_DEG_TO_RAD(data[dataIndex + 2]), -Q_DEG_TO_RAD(data[dataIndex + 0]), Q_DEG_TO_RAD(data[dataIndex + 1]));
				//q_from_euler(destQuat, -Q_DEG_TO_RAD(data[dataIndex + 2]), 0, 0);

				d_quat[Q_X] = destQuat[Q_X];
				d_quat[Q_Y] = destQuat[Q_Y];
				d_quat[Q_Z] = destQuat[Q_Z];
				d_quat[Q_W] = destQuat[Q_W];

				if (header->AvatarIndex == 0)
				{
					d_sensor = curSel;
				}
				else
				{
					d_sensor = curSel + 60;
				}
			}
		}

		// send time out in Neuron's time? Not implemented yet...
		// if (frequency) frame_to_time(frame, frequency, timestamp);
		// else memset(&timestamp, 0, sizeof(timestamp));
		vrpn_gettimeofday(&timestamp, NULL);

		//send the report
		send_report();
	}

	// Add a tracker with the hips position
	dataIndex = 0;
	if (header->WithReference)
	{
		dataIndex += 6;
	}
	// Hips' Displacement
	pos[0] = -data[dataIndex + 0] * SCALE_AXIS;
	pos[1] = data[dataIndex + 1] * SCALE_AXIS;
	pos[2] = -data[dataIndex + 2] * SCALE_AXIS;
	//raw positions have no rotation
	d_quat[0] = 0;
	d_quat[1] = 0;
	d_quat[2] = 0;
	d_quat[3] = 1;

	if (header->AvatarIndex == 0)
	{
		d_sensor = nr;
	}
	else
	{
		d_sensor = nr+60;
	}
	// send time out in Neuron's time? Not implemented yet...
	vrpn_gettimeofday(&timestamp, NULL);
	//send the report
	send_report();

}


void vrpn_Tracker_PerceptionNeuron::send_report(void)
{
	const int VRPN_PERCEPTION_NEURON_MSGBUFSIZE = 1000;

	if (d_connection)
	{
		char	msgbuf[VRPN_PERCEPTION_NEURON_MSGBUFSIZE];
		int	len = encode_to(msgbuf);
		if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr, "Perception Neuron: cannot write message: tossing\n");
		}
	}
}


void vrpn_Tracker_PerceptionNeuron::mainloop()
{
	return;
}


#endif

