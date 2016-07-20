#include "VRPNUnity.h"

VRPNUnity* VRPNUnity::instance = nullptr;
#define MAIN_LOOP_THREAD_TIMEOUT 1


VRPNUnity* VRPNUnity::getInstance()
{
	if (VRPNUnity::instance == nullptr)
	{
		instance = new VRPNUnity();
	}
	return instance;
}

VRPNTracker* VRPNUnity::initializeTracker(const char* serverName)
{
	std::string str = serverName;
	VRPNTracker* tracker;
	VRPNHandle handle;
	auto it = trackersByName.find(str);

	if (it == trackersByName.end())
	{
		handle = newHandle();
		tracker = new VRPNTracker(serverName, handle);
		tracker->tracker->register_change_handler(tracker, trackerCallback);
		trackersByName[serverName] = tracker;
		trackersByHandle[handle] = tracker;
	}
	else
	{
		tracker = it->second;
	}
	
	return tracker;
}

void VRPNUnity::update()
{
	// running the thread for the first time
	if (trackerUpdateThread == nullptr)
	{
		terminateTrackerUpdateThread = false;
		trackerUpdateThread = new std::thread(staticThreadMainLoop, &p);
		f = p.get_future();
	}
}

int VRPNUnity::threadMainLoop(std::promise<bool>* p)
{
	try
	{
		while (!terminateTrackerUpdateThread)
		{
			for (auto iter = trackersByName.begin(); iter != trackersByName.end(); iter++)
				iter->second->mainloop();
			//updateStrings();
			//VRPNTextSender::getInstance()->update();

			std::this_thread::sleep_for(std::chrono::milliseconds(MAIN_LOOP_THREAD_TIMEOUT));
		}
	}
	catch (std::exception& e)
	{
		p->set_exception(std::current_exception());
	}
	p->set_value(true);
	return 1;
}


VRPNUnity::VRPNUnity()
{
	nextHandle = 0;
	trackerUpdateThread = nullptr;
	terminateTrackerUpdateThread = false;
}


VRPNUnity::~VRPNUnity()
{
	if (trackerUpdateThread)
	{
		terminateTrackerUpdateThread = true;
		trackerUpdateThread->join();
		delete trackerUpdateThread;
		trackerUpdateThread = nullptr;
	}
	for (auto iter = trackersByName.begin(); iter != trackersByName.end(); iter++)
		delete iter->second;

	instance = nullptr;
}


extern "C"
{

	VRPN_API void vrpnInit(void)
	{

	}

	VRPN_API void vrpnDestroy(void)
	{
		VRPNUnity::destroy();
	}

	VRPN_API VRPNHandle vrpnInitializeTracker(const char* serverName, int sensorNumber,  int numRepetitions)
	{
		VRPNTracker* tr = VRPNUnity::getInstance()->initializeTracker(serverName);
		tr->addSensorData(sensorNumber, numRepetitions);
		return tr->getHandle();
	}

	VRPN_API bool vrpnTrackerHasData(int handle, int sensorNumber)
	{
		VRPNTracker* tr = VRPNUnity::getInstance()->getTrackerByHandle(handle);
		if (tr != nullptr)
		{
			return tr->hasData(sensorNumber);
		}
		lastError = "Tracker or sensor not found";
		return false;
	}

	VRPN_API TrackerData* vrpnTrackerGetData(int handle, int sensorNumber)
	{
		static TrackerData tr;
		VRPNTracker* tr = VRPNUnity::getInstance()->getTrackerByHandle(handle);
		if (tr != nullptr)
		{
			return tr->copyData(sensorNumber, tr);
		}
		lastError = "Tracker or sensor not found";
		return &tr;
	}

	VRPN_API int vrpnTrackerGetOverflow(int handle, int sensorNumber)
	{
		VRPNTracker* tr = VRPNUnity::getInstance()->getTrackerByHandle(handle);
		if (tr != nullptr)
		{
			return tr->getOverflow(sensorNumber);
		}
		lastError = "Tracker or sensor not found";
		return -1;
	}

	VRPN_API void vrpnTrackerResetOverflow(int handle, int sensorNumber)
	{
		VRPNTracker* tr = VRPNUnity::getInstance()->getTrackerByHandle(handle);
		if (tr != nullptr)
		{
			return tr->resetOverflow(sensorNumber);
		}
		lastError = "Tracker or sensor not found";
	}


	VRPN_API void vrpnUpdate(void)
	{
		VRPNUnity::getInstance()->update();
	}

	void VRPN_CALLBACK trackerCallback(void *userdata, vrpn_TRACKERCB t)
	{
		VRPNTracker* tracker = static_cast<VRPNTracker*>(userdata);
		int sensorNumber = t.sensor;

		if (tracker->isValid(sensorNumber))
		{
			TrackerData tr;
			tr.position[0] = t.pos[0];
			tr.position[1] = t.pos[1];
			tr.position[2] = t.pos[2];
			tr.rotation[0] = t.quat[0];
			tr.rotation[1] = t.quat[1];
			tr.rotation[2] = t.quat[2];
			tr.rotation[3] = t.quat[3];
			tracker->pushData(sensorNumber, tr); // check if overflow?
		}
	}


}
