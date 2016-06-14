#ifndef VRPN_WWA_SERVER_H
#define VRPN_WWA_SERVER_H

#include <string>
#include <thread>
#include <atomic>

#include "vrpn_Configure.h"   // IWYU pragma: keep

#include "vrpn_Shared.h"
#include "vrpn_Tracker.h"
#include <vrpn_Text.h> 

#include "vrpn_MainloopContainer.h" // for vrpn_MainloopContainer

class VRPN_API vrpn_WWA_Server : public vrpn_Text_Sender {
public:
	vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, const char *consoleDeviceTxt, const char *nameHeadsTrk, int nH, 
		int h1_1, int h1_2, int h2_1, int h2_2, const char *nameBodiesTrk, int nB, int b1_1, int b1_2, int b2_1, int b2_2,
		const char *nameCars, const char *expDirectory);

	vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, const char *consoleDeviceTxt, const char *nameHeadsTrk, int nH, 
		int h1_1, int h1_2, int h2_1, int h2_2, const char *nameBodiesTrk, int nB, int b1_1, int b1_2, int b2_1, int b2_2,
		const char *nameCars, const char *expDirectory, const char *headsDeviceTrk, const char *bodiesDeviceTrk, const char *carsDevice);

	~vrpn_WWA_Server();

	void addDevices(vrpn_MainloopContainer *devs);

	virtual void mainloop();

	const std::string& getExpDir() { return expDir; }

	void loadFile(const char* filename);
	void replayFile();
	void pauseFile();
	void resumeFile();
	void realData();

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
	vrpn_Text_Receiver* console;
	vrpn_Tracker_Server* headTracker;
	int nHeadSensors;
	vrpn_Tracker_Server* bodiesTracker;
	int nBodySensors;
	vrpn_Tracker_Server* carsServer;
	std::string	expDir;
	vrpn_Tracker_Remote* headTrackerReader;
	vrpn_Tracker_Remote* bodyTrackerReader;
	vrpn_Tracker_Remote* carReader;

	vrpn_Tracker_Remote* fileHeadTrackerReader;
	vrpn_Tracker_Remote* fileBodyTrackerReader;
	vrpn_Tracker_Remote* fileCarReader;
	vrpn_Text_Receiver* fileMsgReader;
	vrpn_File_Connection* fcn_fileReader;

	bool hasRealTrackers; // true if real trackers are connected

	bool u1FromFile;
	bool u2FromFile;

	std::thread* fileMngrThread;
	std::string	fileToLoad;
	std::string	headDevName;
	std::string	bodyDevName;
	std::string	carsDevName;
	std::string msgDevName;

	// sensor ids for data from the 1st and 2nd users
	int h1_1, h1_2, h2_1, h2_2;
	int b1_1, b1_2, b2_1, b2_2;

	enum ThreadStates { notInit = -1,ready = 0, startReadingFiles, readingFiles, filesReady, errorReadingFiles, endThread };
	std::atomic<ThreadStates> threadState;

	bool isReadyForCommands() { return (threadState==ready || threadState == notInit); }

	void createFileThread();
	void mainFileMngrThread();

	/*****************************************************************************
	*
	Callback handlers
	*
	*****************************************************************************/

	friend static void VRPN_CALLBACK handle_console_commands(void *userdata, const vrpn_TEXTCB t);
	friend static void VRPN_CALLBACK handle_heads_pos_quat(void *userdata, const vrpn_TRACKERCB t);
	friend static void VRPN_CALLBACK handle_body_pos_quat(void *userdata, const vrpn_TRACKERCB t);
	friend static void VRPN_CALLBACK handle_cars(void *userdata, const vrpn_TRACKERCB t);
	friend static void VRPN_CALLBACK handle_file_heads_pos_quat(void *userdata, const vrpn_TRACKERCB t);
	friend static void VRPN_CALLBACK handle_file_body_pos_quat(void *userdata, const vrpn_TRACKERCB t);
	friend static void VRPN_CALLBACK handle_file_cars(void *userdata, const vrpn_TRACKERCB t);
	friend static void VRPN_CALLBACK handle_file_msgs(void *userdata, const vrpn_TEXTCB t);


};



#endif
