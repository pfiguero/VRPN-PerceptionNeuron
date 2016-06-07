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
	vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, const char *consoleDeviceTxt, const char *nameHeadsTrk, int nH, const char *nameBodiesTrk, int nB, 
		const char *nameCars, const char *expDirectory);

	vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, const char *consoleDeviceTxt, const char *nameHeadsTrk, int nH, const char *nameBodiesTrk, int nB, 
		const char *nameCars, const char *expDirectory, const char *headsDeviceTrk, const char *bodiesDeviceTrk, const char *carsDevice);

	~vrpn_WWA_Server();

	void addDevices(vrpn_MainloopContainer *devs);

	virtual void mainloop();

	const std::string& getExpDir() { return expDir; }

	void loadFile(const char* filename);

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
	vrpn_Tracker_Server* bodiesTracker;
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

	bool hasRealTrackers;

	std::thread* fileMngrThread;
	std::string	fileToLoad;
	std::string	headDevName;
	std::string	bodyDevName;
	std::string	carsDevName;
	std::string msgDevName;

	enum ThreadStates { notInit = -1,ready = 0, startReadingFiles, readingFiles, filesReady, endThread };
	std::atomic<ThreadStates> threadState;

	void createFileThread();
	void mainFileMngrThread();
};


/*****************************************************************************
*
Callback handlers
*
*****************************************************************************/

void VRPN_CALLBACK handle_console_commands(void *userdata, const vrpn_TEXTCB t);
void VRPN_CALLBACK handle_heads_pos_quat(void *userdata, const vrpn_TRACKERCB t);
void VRPN_CALLBACK handle_body_pos_quat(void *userdata, const vrpn_TRACKERCB t);
void VRPN_CALLBACK handle_cars(void *userdata, const vrpn_TRACKERCB t);
void VRPN_CALLBACK handle_file_heads_pos_quat(void *userdata, const vrpn_TRACKERCB t);
void VRPN_CALLBACK handle_file_body_pos_quat(void *userdata, const vrpn_TRACKERCB t);
void VRPN_CALLBACK handle_file_cars(void *userdata, const vrpn_TRACKERCB t);
void VRPN_CALLBACK handle_file_msgs(void *userdata, const vrpn_TEXTCB t);


#endif
