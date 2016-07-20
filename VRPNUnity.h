#pragma once

// VRPN Unity
// Multithreaded wrapper in Unity for VRPN
// Pablo Figueroa, PhD
// Universidad de los Andes, Colombia
// email: pfiguero@uniandes.edu.co
//

#include "vrpn_Configure.h"
#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"

#include <string>
#include <map>
#include <thread>
#include <future>
#include <mutex>
#include <chrono>


typedef int VRPNHandle;
const int DEF_REPETITIONS = 5;
class VRPNUnity; // cross reference.

struct TrackerData
{
	double position[3];
	double rotation[4];

	TrackerData()
	{
		reset();
	}
	~TrackerData()
	{
	}
	TrackerData(const TrackerData& t)
	{
		copy(t);
	}
	void reset()
	{
		position[0] = 0.0;
		position[1] = 0.0;
		position[2] = 0.0;
		rotation[0] = 0.0;
		rotation[1] = 0.0;
		rotation[2] = 0.0;
		rotation[3] = 1.0;
	}
	TrackerData& operator = (const TrackerData& t)
	{
		copy(t);
		return *this;
	}
private:
	void copy(const TrackerData& t)
	{
		position[0] = t.position[0];
		position[1] = t.position[1];
		position[2] = t.position[2];
		rotation[0] = t.rotation[0];
		rotation[1] = t.rotation[1];
		rotation[2] = t.rotation[2];
		rotation[3] = t.rotation[3];
	}
};

template<typename T>
class Repetitions
{
public:
	Repetitions(int n)
	{
		reps = new std::pair<bool, T*>[n]; // T*[n];
		num = n;
		curr = 0;
		for (int i = 0; i < num; i++)
		{
			reps[i].second = new T(); // new std::pair<bool, T*>(false, new T());
		}
		numOverflow = 0;
	}
	~Repetitions()
	{
		for (int i = 0; i < num; i++)
		{
			delete reps[i].second;
		}
		delete[] reps;
	}
	bool hasData()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return reps[curr].first;
	}
	bool pushData(T& data)
	{
		std::lock_guard<std::mutex> lock(mtx);
		bool isOverflow = false;
		if (reps[curr].first == true)
			isOverflow = true;
		reps[curr].first = true;
		*(reps[curr].second) = data; // requires operator=()
		curr++;
		if (curr >= num)
			curr = 0;
		if (isOverflow)
			numOverflow++;
		return isOverflow;
	}
	int getOverflow()
	{
		std::lock_guard<std::mutex> lock(mtx);
		return numOverflow;
	}
	void resetOverflow()
	{
		std::lock_guard<std::mutex> lock(mtx);
		numOverflow = 0;
	}
	void copyData(T& data)
	{
		std::lock_guard<std::mutex> lock(mtx);
		T* resp = nullptr;
		if (hasData())
		{
			resp = reps[curr].second;
			reps[curr].first = false;
			curr++;
			if (curr >= num)
				curr = 0;
			data = *resp;
		}
	}
	
private:
	std::pair<bool, T*>* reps; // has data or not, and the data
	int num;
	int curr;
	int numOverflow;

	std::mutex mtx;
};

class VRPNTracker
{
public:
	VRPNTracker(const char* name, VRPNHandle h)
	{
		tracker = new vrpn_Tracker_Remote(name);
		serverName = name;
		handle = h;
	}
	~VRPNTracker()
	{
		delete tracker;
		for (auto iter = data.begin(); iter != data.end(); iter++)
			delete iter->second;
	}
	void addSensorData(int sensorNumber, int numRepetitions)
	{
		Repetitions<TrackerData>* rep;
		auto it = data.find(sensorNumber);
		if (it == data.end())
		{
			Repetitions<TrackerData>* rep = new Repetitions<TrackerData>(numRepetitions);
			data[sensorNumber] = rep;
		}
		// It may happen that it was created with a different number of repetitions...
	}
	bool isValid(int sensorNumber)
	{
		return data.find(sensorNumber) != data.end();
	}
	bool hasData(int sensorNumber)
	{
		return data[sensorNumber]->hasData();
	}
	bool pushData(int sensorNumber, TrackerData& td)
	{
		return data[sensorNumber]->pushData(td);
	}
	int getOverflow(int sensorNumber)
	{
		data[sensorNumber]->getOverflow();
	}
	void resetOverflow(int sensorNumber)
	{
		data[sensorNumber]->resetOverflow();
	}
	void copyData(int sensorNumber, TrackerData& td)
	{
		data[sensorNumber]->copyData(td);
	}
	const char* getServerName()
	{
		return serverName.c_str();
	}
	VRPNHandle getHandle()
	{
		return handle;
	}
	void mainloop()
	{
		tracker->mainloop();
	}
	friend VRPNUnity;
private:
	vrpn_Tracker_Remote* tracker;
	std::map<int, Repetitions<TrackerData>* > data; // indexed by sensor number
	std::string serverName;
	VRPNHandle handle;
};



class VRPNUnity
{
public:
	static VRPNUnity* getInstance();
	VRPNTracker* initializeTracker(const char* serverName);
	void update();
	VRPNTracker* getTrackerByHandle(VRPNHandle h)
	{
		if (trackersByHandle.find(h) != trackersByHandle.end())
		{
			return trackersByHandle[h];
		}
		return nullptr;
	}

private:
	VRPNUnity();
	~VRPNUnity();
	VRPNHandle newHandle()
	{
		return ++nextHandle;
	}

	int threadMainLoop(std::promise<bool>* p);

	static int staticThreadMainLoop(std::promise<bool>* p)
	{
		return VRPNUnity::getInstance()->threadMainLoop(p);
	}

	static void destroy()
	{
		if (instance != nullptr)
		{
			delete instance;
			instance = nullptr;
		}
	}

	std::map<std::string, VRPNTracker*> trackersByName;
	std::map<VRPNHandle, VRPNTracker*> trackersByHandle;

	int nextHandle;
	static VRPNUnity* instance;

	std::thread* trackerUpdateThread;
	std::promise<bool> p;
	std::future<bool> f;
	bool terminateTrackerUpdateThread;
};

extern "C"
{
	const char* lastError = "";
	VRPN_API void getLastErrorAndClear(char* str, int n)
	{
		strncpy(str, lastError, n - 1);
		lastError = "";
	}

	VRPN_API void vrpnInit(void);
	VRPN_API void vrpnDestroy(void);
	VRPN_API VRPNHandle vrpnInitializeTracker(const char* serverName, int sensorNumber, int numRepetitions);

	VRPN_API bool vrpnTrackerHasData(int handle, int sensorNumber);
	VRPN_API TrackerData* vrpnTrackerGetData(int handle, int sensorNumber);

	VRPN_API int vrpnTrackerGetOverflow(int handle, int sensorNumber);
	VRPN_API void vrpnTrackerResetOverflow(int handle, int sensorNumber);

	VRPN_API void vrpnUpdate(void);

	void VRPN_CALLBACK trackerCallback(void *userdata, vrpn_TRACKERCB t);
}

