#include "vrpn_WWA_Server.h"
#include <vrpn_FileConnection.h>

#include <chrono>

FILE* debugFile = NULL;

vrpn_WWA_Server::vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, int tGoMsgInSecs,
	const char *consoleDeviceTxt, const char *p1DeviceTxt, const char *p2DeviceTxt,
	const char *nameHeadsTrk, int nH, int h11, int h12, int h21, int h22,
	const char *nameBodiesTrk, int nB, int b11, int b12, int b21, int b22,
	const char *nameCars, const char *expDirectory): vrpn_Text_Sender(nameTxt, c)
{
#ifdef DEBUG
	printf("%s\nheadTrackerReader, __PRETTY_FUNCTION__);
#endif

	if (debugFile == NULL)
	{
		debugFile = fopen("C:\\Users\\pfigueroa\\Desktop\\GIT\\vrpn-PerceptionNeuron\\pc_win32\\server_src\\vrpn_server\\Debug\\debugFile.txt", "a");
	}

	headDevName = nameHeadsTrk;
	bodyDevName = nameBodiesTrk;
	carsDevName = nameCars;
	msgDevName = nameTxt;
	hasRealTrackers = false;
	nHeadSensors = nH;
	nBodySensors = nB;
	timeoutGoMsgInSecs = tGoMsgInSecs;

	h1_1 = h11; h1_2 = h12, h2_1 = h21, h2_2 = h22;
	b1_1 = b11; b1_2 = b12, b2_1 = b21, b2_2 = b22;

	u1FromFile = false;
	u2FromFile = false;

	// Create internal devices and clients
	console = new vrpn_Text_Receiver(consoleDeviceTxt);
	console->register_message_handler(this, handle_console_commands);

	if (strncmp(p1DeviceTxt, "NoDevice", 8) != 0)
	{
		p1 = new vrpn_Text_Receiver(consoleDeviceTxt);
		p1->register_message_handler(this, handle_console_commands);
	}
	if (strncmp(p2DeviceTxt, "NoDevice", 8) != 0)
	{
		p2 = new vrpn_Text_Receiver(consoleDeviceTxt);
		p2->register_message_handler(this, handle_console_commands);
	}

	headTracker = new vrpn_Tracker_Server(nameHeadsTrk,c,nH);
	bodiesTracker = new vrpn_Tracker_Server(nameBodiesTrk, c, nB);
	carsServer = new vrpn_MxRTrafficGenerator_ProxyServer(nameCars);

	expDir = "";
	expDir += expDirectory;
	fileToLoad = "";

	headTrackerReader = NULL;
	bodyTrackerReader = NULL;
	carReader = NULL;
	fileHeadTrackerReader = NULL;
	fileBodyTrackerReader = NULL;
	fileCarReader = NULL;
	fileMsgReader = NULL;

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

	threadState = notInit;
	fileMngrThread = NULL;

	serverState = idle;
	okTrialP1 = okTrialP2 = okTrialCS = false;
	endTrialP1 = endTrialP2 = endTrialCS = false;
}

vrpn_WWA_Server::vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, int tGoMsgInSecs,
	const char *consoleDeviceTxt, const char *p1DeviceTxt, const char *p2DeviceTxt,
	const char *nameHeadsTrk, int nH, int h1_1, int h1_2, int h2_1, int h2_2, 
	const char *nameBodiesTrk, int nB, int b1_1, int b1_2, int b2_1, int b2_2,
	const char *nameCars, const char *expDirectory, const char *headsDeviceTrk, const char *bodiesDeviceTrk, const char *carsDevice): 
	vrpn_WWA_Server(c, nameTxt, tGoMsgInSecs, consoleDeviceTxt, p1DeviceTxt, p2DeviceTxt, nameHeadsTrk, nH, h1_1, h1_2, h2_1, h2_2,
		nameBodiesTrk, nB, b1_1, b1_2, b2_1, b2_2, nameCars, expDirectory)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif

	hasRealTrackers = true;

	// Create extra internal devices and clients. Check if NoDevice
	if (strncmp(headsDeviceTrk, "NoDevice", 8) != 0)
	{
		headTrackerReader = new vrpn_Tracker_Remote(headsDeviceTrk);
		headTrackerReader->register_change_handler(this, handle_heads_pos_quat);
	} // assuming the variable is already NULL if the condition is not met
	if (strncmp(bodiesDeviceTrk, "NoDevice", 8) != 0)
	{
		bodyTrackerReader = new vrpn_Tracker_Remote(bodiesDeviceTrk);
		bodyTrackerReader->register_change_handler(this, handle_body_pos_quat);
	}
	if (strncmp(carsDevice, "NoDevice", 8) != 0)
	{
		carReader = new vrpn_MxRTrafficGenerator_Remote(carsDevice);
		carReader->register_trafficupdate_handler(this, handle_cars);
	}

	printf("WWAServer. Creating WWA_Server with parameters: %s %d %s %s %s %s %d %d %d %d %d %s %d %d %d %d %d %s %s %s %s %s\n", 
		nameTxt, tGoMsgInSecs, consoleDeviceTxt, p1DeviceTxt, p2DeviceTxt, nameHeadsTrk, nH, h1_1, h1_2, h2_1, h2_2,
		nameBodiesTrk, nB, b1_1, b1_2, b2_1, b2_2, nameCars, expDirectory, headsDeviceTrk, bodiesDeviceTrk, carsDevice );
}

//
vrpn_WWA_Server::~vrpn_WWA_Server()
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif

	if (debugFile != NULL)
	{
		fflush(debugFile);
		fclose(debugFile);
	}

	fprintf(stderr, "WWAServer destroying...\n");

	threadState = endThread;
	/*
	if(fileMngrThread->joinable())
		fileMngrThread->join();
	if (console != NULL) delete console;
	if (headTracker != NULL) delete headTracker;
	if (bodiesTracker != NULL) delete bodiesTracker;
	if (carsServer != NULL) delete carsServer;
	if (headTrackerReader != NULL) delete headTrackerReader;
	if (bodyTrackerReader != NULL) delete bodyTrackerReader;
	if (fileHeadTrackerReader != NULL) delete fileHeadTrackerReader;
	if (fileBodyTrackerReader != NULL) delete fileBodyTrackerReader;
	if (fileCarReader != NULL) delete fileCarReader;
	if (fileMsgReader != NULL) delete fileMsgReader;
	if (carReader != NULL) delete carReader;
	*/
}

void vrpn_WWA_Server::addDevices(vrpn_MainloopContainer *devs)
{
	devs->add(this); // adding the text server
	if (headTracker != NULL)
		devs->add(headTracker); // adding the head tracker server
	if (bodiesTracker != NULL)
		devs->add(bodiesTracker); // adding the body tracker server
	if (carsServer != NULL)
		devs->add(carsServer); // adding the cars server
}

void vrpn_WWA_Server::mainloop()
{
	// readers from real devices and the console
	if (console != NULL)
		console->mainloop();

	if (headTrackerReader != NULL)
		headTrackerReader->mainloop();
	if (bodyTrackerReader != NULL)
		bodyTrackerReader->mainloop();

	// servers
	if (headTracker != NULL)
		headTracker->mainloop();
	if (bodiesTracker != NULL)
		bodiesTracker->mainloop();

	if (carReader != NULL)
		carReader->mainloop();

	// readers from file. Be sure they are ready to be read
	if (fileHeadTrackerReader != NULL && threadState == ready)
		fileHeadTrackerReader->mainloop();
	if (fileBodyTrackerReader != NULL && threadState == ready)
		fileBodyTrackerReader->mainloop();
	if (fileCarReader != NULL && threadState == ready)
		fileCarReader->mainloop();
	if (fileMsgReader != NULL && threadState == ready)
		fileMsgReader->mainloop();

	if (carsServer != NULL)
		carsServer->mainloop();

	vrpn_Text_Sender::mainloop(); // msgs

	// Extra processing for server's thread states
	switch (threadState)
	{
	case errorReadingFiles:
	case filesReady:
		threadState = ready;
		break;
	}

	// extra processing for other commands
	if (isReadyForCommands())
	{
		switch (serverState)
		{
		case idle:
			break;
		case waitingOkTrial: 
			if (okTrialP1 && okTrialP2 && okTrialCS)
			{
				send_message("UNBLACK SV");
				startGo = std::chrono::system_clock::now();
				okTrialP1 = okTrialP2 = okTrialCS = false;
				serverState = waitingGoTrial;
			}
			break;
		case waitingGoTrial:
			endGo = std::chrono::system_clock::now();
			auto delta = std::chrono::duration<float>(endGo - startGo);
			if (std::chrono::duration_cast<std::chrono::seconds>(delta).count() >= timeoutGoMsgInSecs)
			{
				send_message("GO SV");
				serverState = waitingEndTrial;
			}
			break;
		case waitingEndTrial:
			if (endTrialP1 && endTrialP2 && endTrialCS)
			{
				send_message("ENDTRIAL SV");
				endTrialP1 = endTrialP2 = endTrialCS = false;
				serverState = idle;
			}
			break;
		}
	}
}

/*
// Create a call object that prints characters that it receives
// to the console.
call<wchar_t> print_character([](wchar_t c) {
wcout << c;
});

// Create a timer object that sends the period (.) character to
// the call object every 100 milliseconds.
timer<wchar_t> progress_timer(100u, L'.', &print_character, true);
*/

void vrpn_WWA_Server::createFileThread()
{
	threadState = ready;
	fileMngrThread = new std::thread([=] { mainFileMngrThread(); });
}

void vrpn_WWA_Server::mainFileMngrThread()
{
	std::string devName;
	vrpn_Connection *con;

	while (threadState != endThread)
	{
		switch (threadState)
		{
		case notInit:
			fprintf(stderr,"WWAServer Error: Very weird state [notInit]. Check File Manager.\n");
			threadState = endThread;
			break;
		case ready:
			break;
		case startReadingFiles:
			threadState = readingFiles;
			if (fileHeadTrackerReader != NULL)
			{
				delete fileHeadTrackerReader;
			}
			if (fileBodyTrackerReader != NULL)
			{
				delete fileBodyTrackerReader;
			}
			if (fileCarReader != NULL)
			{
				delete fileCarReader;
			}
			devName = headDevName;
			devName += "@file://";
			devName += fileToLoad;
			printf("WWAServer. Connection to [%s] requested\n", devName.c_str());
			fileHeadTrackerReader = new vrpn_Tracker_Remote(devName.c_str());
			if (fileHeadTrackerReader == NULL)
			{
				// assuming error while creating the file...
				fprintf(stderr, "WWAServer Error: can't create this client [%s]\n", devName.c_str());
				threadState = errorReadingFiles;
				break;
			}
			fileHeadTrackerReader->register_change_handler(this, handle_file_heads_pos_quat);

			devName = bodyDevName;
			devName += "@file://";
			devName += fileToLoad;
			printf("WWAServer. Connection to [%s] requested\n", devName.c_str());
			fileBodyTrackerReader = new vrpn_Tracker_Remote(devName.c_str());
			if (fileBodyTrackerReader == NULL)
			{
				// assuming error while creating the file...
				fprintf(stderr, "WWAServer Error: can't create this client [%s]\n", devName.c_str());
				threadState = errorReadingFiles;
				break;
			}
			fileBodyTrackerReader->register_change_handler(this, handle_file_body_pos_quat);

			devName = carsDevName;
			devName += "@file://";
			devName += fileToLoad;
			printf("WWAServer. Connection to [%s] requested\n", devName.c_str());
			fileCarReader = new vrpn_MxRTrafficGenerator_Remote(devName.c_str());
			if (fileCarReader == NULL)
			{
				// assuming error while creating the file...
				fprintf(stderr, "WWAServer Error: can't create this client [%s]\n", devName.c_str());
				threadState = errorReadingFiles;
				break;
			}
			fileCarReader->register_trafficupdate_handler(this, handle_file_cars);

			devName = msgDevName;
			devName += "@file://";
			devName += fileToLoad;
			printf("WWAServer. Connection to [%s] requested\n", devName.c_str());
			fileMsgReader = new vrpn_Text_Receiver(devName.c_str());
			if (fileMsgReader == NULL)
			{
				// assuming error while creating the file...
				fprintf(stderr, "WWAServer Error: can't create this client [%s]\n", devName.c_str());
				threadState = errorReadingFiles;
				break;
			}
			fileMsgReader->register_message_handler(this, handle_file_msgs);

			con = fileBodyTrackerReader->connectionPtr();
			fcn_fileReader = con->get_File_Connection();
			if (fcn_fileReader == NULL)
			{
				fprintf(stderr, "WWAServer Error: Error establishing connection with [%s]\n", devName.c_str());
				threadState = errorReadingFiles;
				break;
			}
			fcn_fileReader->set_replay_rate(1.0); // start playing
			fcn_fileReader->reset();

			threadState = filesReady;
			break;
		case readingFiles:
			fprintf(stderr, "WWAServer Error: Very weird state [readingFiles]. Check File Manager.\n");
			threadState = endThread;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void vrpn_WWA_Server::loadFile(const char* filename)
{
	switch (threadState)
	{
	case notInit:
		createFileThread();
		// continue with the ready behavior
	case ready:
		fileToLoad = filename;
		threadState = startReadingFiles;
		break;
	case startReadingFiles:
		fprintf(stderr, "WWAServer Error: Can't process a new file load for [%s]. Still working on the previous one [%s]\n", filename, fileToLoad.c_str());
		break;
	case readingFiles:
		fprintf(stderr,"WWAServer Error: Can't process a new file load for [%s]. Still working on the previous one [%s]\n", filename, fileToLoad.c_str());
		break;
	}
}

void vrpn_WWA_Server::replayFile()
{
	if (fcn_fileReader != NULL)
	{
		fcn_fileReader->set_replay_rate(1.0); // start playing
		fcn_fileReader->reset();
	}
	else
	{
		fprintf(stderr, "WWAServer Error: Can't replay. File handler not available\n" );
	}
}

void vrpn_WWA_Server::pauseFile()
{
	if (fcn_fileReader != NULL)
	{
		fcn_fileReader->set_replay_rate(0.0); // pause
	}
	else
	{
		fprintf(stderr, "WWAServer Error: Can't pause. File handler not available\n");
	}
}

void vrpn_WWA_Server::resumeFile()
{
	if (fcn_fileReader != NULL)
	{
		fcn_fileReader->set_replay_rate(1.0); // resume
	}
	else
	{
		fprintf(stderr, "WWAServer Error: Can't continue. File handler not available\n");
	}
}

void vrpn_WWA_Server::realData()
{
	u1FromFile = false;
	u2FromFile = false;
}

void vrpn_WWA_Server::startTrial(const char* origin, int trialId)
{
	// To Do: seek to a particular place!
	serverState = waitingOkTrial;
	fprintf(stderr, "WWAServer start trial processed: %s %d\n", origin, trialId);
	if (debugFile != NULL)
		fprintf(debugFile, "%s threadState [%d] serverState [%d] %s %s %s\n", "startTrial", threadState, serverState, okTrialP1 == true ? "true" : "false", okTrialP2 == true ? "true" : "false", okTrialCS == true ? "true" : "false");

	if (vrpn_MxRTrafficGenerator_Server::getInstance() != NULL)
	{
		vrpn_MxRTrafficGenerator_Server::getInstance()->startTrial();
	}
}

void vrpn_WWA_Server::okTrial(const char* origin, int trialId)
{
	// assuming trial id is correct
	if (strncmp(origin, "P1", 2) == 0)
	{
		okTrialP1 = true;
	}
	else if (strncmp(origin, "P2", 2) == 0)
	{
		okTrialP2 = true;
	}
	else if (strncmp(origin, "CS", 2) == 0)
	{
		okTrialCS = true;
	}

	if (debugFile != NULL)
		fprintf(debugFile, "%s threadState [%d] serverState [%d] %s %s %s\n", "okTrial", threadState, serverState, okTrialP1 == true ? "true" : "false", okTrialP2 == true ? "true" : "false", okTrialCS == true ? "true" : "false");
}

void vrpn_WWA_Server::endTrial(const char* origin, int trialId)
{
	// assuming trial id is correct
	if (strncmp(origin, "P1", 2) == 0)
	{
		endTrialP1 = true;
	}
	else if (strncmp(origin, "P2", 2) == 0)
	{
		endTrialP2 = true;
	}
	else if (strncmp(origin, "CS", 2) == 0)
	{
		endTrialCS = true;
	}

	if (vrpn_MxRTrafficGenerator_Server::getInstance() != NULL && endTrialP1 && endTrialP2 && endTrialCS)
	{
		vrpn_MxRTrafficGenerator_Server::getInstance()->finishTrial();
	}

}


/*****************************************************************************
*
Callback handlers
*
*****************************************************************************/


/* General Commands:
	LOAD filename <U1 | U2 | U1 U2>
	REPLAY
	PAUSE
	CONTINUE
	REAL_DATA

	Received from clients, due to protocol (it may generate others):
	BLACKSCR <P1 | P2>
	TRIAL CS id
	OKTRIAL <CS | P1 | P2> id (it generates UNBLACK SV and GO SV after a while)
	ENDTRIAL <P1 | P2> id
*/

void VRPN_CALLBACK 
handle_console_commands(void *userdata, const vrpn_TEXTCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	const char* command = t.message;
	char fname[100];
	char p1[20], p2[20], p3[20];
	int var1=-1;
	fname[0] = '\0';
	p1[0] = '\0';
	p2[0] = '\0';

	fprintf(stderr,"WWAServer Receiving command: %s\n", command);

	if (!obj->isReadyForCommands())
	{
		fprintf(stderr, "WWAServer is processing another command. Wait a while\n" );
		return;
	}

	// Message handling
	// Most messages in the protocol are broadcasted, unless they have as origin SV... to be safg
	if (strncmp(command, "LOAD", 4) == 0)
	{
		// reset
		obj->realData();

		// LOAD filename U1 | U2
		command += 5; // now command sees parameters

		int params = sscanf(command, "%s %s %s", fname, p1, p2);

		if (strncmp(p1, "U1", 2) == 0)
		{
			obj->u1FromFile = true;
		}
		if (strncmp(p1, "U2", 2) == 0)
		{
			obj->u2FromFile = true;
		}
		if (params == 3)
		{
			if (strncmp(p2, "U1", 2) == 0)
			{
				obj->u1FromFile = true;
			}
			if (strncmp(p2, "U2", 2) == 0)
			{
				obj->u2FromFile = true;
			}
		}

		// everything ready to start reading from the file
		std::string filename = obj->getExpDir();
		filename += "\\";
		filename += fname;
		printf("WWAServer. LOAD file: %s\n", filename.c_str());
		// start the process of connecting a file
		obj->loadFile(filename.c_str());
	}
	else if (strncmp(command, "REPLAY", 6) == 0)
	{
		obj->replayFile();
	}
	else if (strncmp(command, "PAUSE", 5) == 0)
	{
		obj->pauseFile();
	}
	else if (strncmp(command, "CONTINUE", 7) == 0)
	{
		obj->resumeFile();
	}
	else if (strncmp(command, "REAL_DATA", 9) == 0)
	{
		obj->realData();
	}
	else if (strncmp(command, "BLACKSCR", 8) == 0)
	{
		command += 9;
		int params = sscanf(command, "%s", p1);

		// Assuming it is well formed and not originated at SV
		if (strncmp(p1, "SV", 2) != 0)
		{
			command -= 9;
			obj->send_message(command);
		}
	}
	else if (strncmp(command, "TRIAL", 5) == 0)
	{
		command += 6;
		int params = sscanf(command, "%s %d", p1, &var1);
		obj->startTrial(p1, var1);

		// Assuming it is well formed and not originated at SV
		if (strncmp(p1, "SV", 2) != 0)
		{
			command -= 6;
			obj->send_message(command);
		}
	}
	else if (strncmp(command, "OKTRIAL", 7) == 0)
	{
		command += 8;
		int params = sscanf(command, "%s %d", p1, &var1);
		obj->okTrial(p1, var1);

		// Assuming it is well formed and not originated at SV
		if (strncmp(p1, "SV", 2) != 0)
		{
			command -= 8;
			obj->send_message(command);
		}
	}
	else if (strncmp(command, "ENDTRIAL", 8) == 0)
	{
		command += 9;
		int params = sscanf(command, "%s %d", p1, &var1);
		obj->endTrial(p1, var1);

		// Assuming it is well formed and not originated at SV
		if (strncmp(p1, "SV", 2) != 0)
		{
			command -= 9;
			obj->send_message(command);
		}
	}
	// ignore other messages... specially the "No response" message that repeats itself so much... :)
}


void VRPN_CALLBACK
handle_heads_pos_quat(void *userdata, const vrpn_TRACKERCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	// For user 1
	if (!obj->u1FromFile && obj->headTracker != NULL && obj->nHeadSensors > t.sensor && (t.sensor == obj->h1_1 || t.sensor == obj->h1_2))
	{
		obj->headTracker->report_pose(t.sensor, t.msg_time, t.pos, t.quat);
	}
	// For user 2
	if (!obj->u2FromFile && obj->headTracker != NULL && obj->nHeadSensors > t.sensor && (t.sensor == obj->h2_1 || t.sensor == obj->h2_2))
	{
		obj->headTracker->report_pose(t.sensor, t.msg_time, t.pos, t.quat);
	}
}

void VRPN_CALLBACK
handle_body_pos_quat(void *userdata, const vrpn_TRACKERCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	// For user 1
	if (!obj->u1FromFile && obj->bodiesTracker != NULL && obj->nBodySensors > t.sensor && (t.sensor >= obj->b1_1 && t.sensor <= obj->b1_2))
	{
		obj->bodiesTracker->report_pose(t.sensor, t.msg_time, t.pos, t.quat);
	}
	// For user 2
	if (!obj->u2FromFile && obj->bodiesTracker != NULL && obj->nBodySensors > t.sensor && (t.sensor >= obj->b2_1 && t.sensor <= obj->b2_2))
	{
		obj->bodiesTracker->report_pose(t.sensor, t.msg_time, t.pos, t.quat);
	}
}

void VRPN_CALLBACK
handle_cars(void *userdata, const vrpn_MXRTRAFFIC_CALLBACK t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	// Just send real data, for the moment...
	obj->carsServer->report_traffic_data(t.msg_time, t.car_count, t.cars);

}

void VRPN_CALLBACK
handle_file_heads_pos_quat(void *userdata, const vrpn_TRACKERCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	// Process information from the head's tracker
	// For user 1
	if (obj->u1FromFile && obj->headTracker != NULL && obj->nHeadSensors > t.sensor && (t.sensor == obj->h1_1 || t.sensor == obj->h1_2))
	{
		obj->headTracker->report_pose(t.sensor, t.msg_time, t.pos, t.quat);
	}
	// For user 2
	if (obj->u2FromFile && obj->headTracker != NULL && obj->nHeadSensors > t.sensor && (t.sensor == obj->h2_1 || t.sensor == obj->h2_2))
	{
		obj->headTracker->report_pose(t.sensor, t.msg_time, t.pos, t.quat);
	}
}

void VRPN_CALLBACK
handle_file_body_pos_quat(void *userdata, const vrpn_TRACKERCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	// Process information from the body's tracker
	// For user 1
	if (obj->u1FromFile && obj->bodiesTracker != NULL && obj->nBodySensors > t.sensor && (t.sensor >= obj->b1_1 && t.sensor <= obj->b1_2))
	{
		obj->bodiesTracker->report_pose(t.sensor, t.msg_time, t.pos, t.quat);
	}
	// For user 2
	if (obj->u2FromFile && obj->bodiesTracker != NULL && obj->nBodySensors > t.sensor && (t.sensor >= obj->b2_1 && t.sensor <= obj->b2_2))
	{
		obj->bodiesTracker->report_pose(t.sensor, t.msg_time, t.pos, t.quat);
	}

}

void VRPN_CALLBACK
handle_file_cars(void *userdata, const vrpn_MXRTRAFFIC_CALLBACK t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	// Process information from car simulator

}

void VRPN_CALLBACK
handle_file_msgs(void *userdata, const vrpn_TEXTCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

}

int vrpn_WWA_Server::handle_first_connection_message(void *userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	// Do nothing for the moment...
	return 0;
}

int vrpn_WWA_Server::handle_got_connection_message(void *userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	// Do nothing for the moment...
	return 0;
}

int vrpn_WWA_Server::handle_connection_dropped_message(void *userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	/*
	vrpn_WWA_Server *me = static_cast<vrpn_WWA_Server *>(userdata);
	// it still crashes sometimes... I guess more exceptions have to be handled, plus something related to the UDP connection...
	delete me;
	*/
	// Do nothing for the moment...
	return 0;
}

int vrpn_WWA_Server::handle_dropped_last_connection_message(void *userdata, vrpn_HANDLERPARAM)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
	// Do nothing for the moment...
	return 0;
}

