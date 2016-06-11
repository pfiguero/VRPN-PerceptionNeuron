#include "vrpn_WWA_Server.h"
#include <vrpn_FileConnection.h>

#include <chrono>

vrpn_WWA_Server::vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, const char *consoleDeviceTxt, 
	const char *nameHeadsTrk, int nH, int h11, int h12, int h21, int h22,
	const char *nameBodiesTrk, int nB, int b11, int b12, int b21, int b22,
	const char *nameCars, const char *expDirectory): vrpn_Text_Sender(nameTxt, c)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif

	headDevName = nameHeadsTrk;
	bodyDevName = nameBodiesTrk;
	carsDevName = nameCars;
	msgDevName = nameTxt;
	hasRealTrackers = false;
	nHeadSensors = nH;
	nBodySensors = nB;

	h1_1 = h11; h1_2 = h12, h2_1 = h21, h2_2 = h22;
	b1_1 = b11; b1_2 = b12, b2_1 = b21, b2_2 = b22;

	u1FromFile = false;
	u2FromFile = false;

	// Create internal devices and clients
	console = new vrpn_Text_Receiver(consoleDeviceTxt);
	console->register_message_handler(this, handle_console_commands);

	headTracker = new vrpn_Tracker_Server(nameHeadsTrk,c,nH);
	bodiesTracker = new vrpn_Tracker_Server(nameBodiesTrk, c, nB);
	carsServer = new vrpn_Tracker_Server(nameCars,c,1);
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

}

vrpn_WWA_Server::vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, const char *consoleDeviceTxt, 
	const char *nameHeadsTrk, int nH, int h1_1, int h1_2, int h2_1, int h2_2, 
	const char *nameBodiesTrk, int nB, int b1_1, int b1_2, int b2_1, int b2_2,
	const char *nameCars, const char *expDirectory, const char *headsDeviceTrk, const char *bodiesDeviceTrk, const char *carsDevice): 
	vrpn_WWA_Server(c, nameTxt, consoleDeviceTxt, nameHeadsTrk, nH, h1_1, h1_2, h2_1, h2_2, 
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
		carReader = new vrpn_Tracker_Remote(carsDevice);
		carReader->register_change_handler(this, handle_cars);
	}

	printf("WWAServer. Creating WWA_Server with parameters: %s %s %s %d %d %d %d %d %s %d %d %d %d %d %s %s %s %s %s\n", nameTxt,
		consoleDeviceTxt, nameHeadsTrk, nH, h1_1, h1_2, h2_1, h2_2, 
		nameBodiesTrk, nB, b1_1, b1_2, b2_1, b2_2, nameCars, expDirectory, headsDeviceTrk, bodiesDeviceTrk, carsDevice );
}

//
vrpn_WWA_Server::~vrpn_WWA_Server()
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
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

	// readers from file
	if (fileHeadTrackerReader != NULL)
		fileHeadTrackerReader->mainloop();
	if (fileBodyTrackerReader != NULL)
		fileBodyTrackerReader->mainloop();
	if (fileCarReader != NULL)
		fileCarReader->mainloop();
	if (fileMsgReader != NULL)
		fileMsgReader->mainloop();

	if (carsServer != NULL)
		carsServer->mainloop();

	vrpn_Text_Sender::mainloop(); // msgs
}

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
			fileHeadTrackerReader->register_change_handler(this, handle_file_heads_pos_quat);

			devName = bodyDevName;
			devName += "@file://";
			devName += fileToLoad;
			printf("WWAServer. Connection to [%s] requested\n", devName.c_str());
			fileBodyTrackerReader = new vrpn_Tracker_Remote(devName.c_str());
			fileBodyTrackerReader->register_change_handler(this, handle_file_body_pos_quat);

			devName = carsDevName;
			devName += "@file://";
			devName += fileToLoad;
			printf("WWAServer. Connection to [%s] requested\n", devName.c_str());
			fileCarReader = new vrpn_Tracker_Remote(devName.c_str());
			fileCarReader->register_change_handler(this, handle_file_cars);

			devName = msgDevName;
			devName += "@file://";
			devName += fileToLoad;
			printf("WWAServer. Connection to [%s] requested\n", devName.c_str());
			fileMsgReader = new vrpn_Text_Receiver(devName.c_str());
			fileMsgReader->register_message_handler(this, handle_file_msgs);

			con = fileHeadTrackerReader->connectionPtr();
			fcn_fileReader = con->get_File_Connection();
			if (fcn_fileReader == NULL)
			{
				fprintf(stderr, "WWAServer Error: Error establishing connection with [%s]\n", devName.c_str());
				threadState = ready;
				break;
			}
			fcn_fileReader->set_replay_rate(0.0); // pause the files
			fcn_fileReader->reset();

			threadState = filesReady;
			break;
		case readingFiles:
			fprintf(stderr, "WWAServer Error: Very weird state [readingFiles]. Check File Manager.\n");
			threadState = endThread;
			break;
		case filesReady:
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
	case filesReady:
		fprintf(stderr, "WWAServer Error: Can't process a new file load for [%s]. Still working on the previous one [%s]\n", filename, fileToLoad.c_str());
		break;
	}
}

/*****************************************************************************
*
Callback handlers
*
*****************************************************************************/

void VRPN_CALLBACK 
handle_console_commands(void *userdata, const vrpn_TEXTCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	const char* command = t.message;
	char p1[20], p2[20];
	p1[0] = '\0';
	p2[0] = '\0';

	fprintf(stderr,"WWAServer Receiving command: %s\n", command);

	if (strncmp(command, "LOAD ", 5) == 0)
	{
		command += 5; // now command sees the file name
		std::string filename = obj->getExpDir();
		filename += "\\";
		filename += command;
		printf("WWAServer. LOAD file: %s\n", filename.c_str());
		// start the process of connecting a file
		obj->loadFile(filename.c_str());
	}
	else if (strncmp(command, "PLAY ", 5) == 0)
	{
		command += 5; // now command sees parameters
		int params = sscanf(command, "%s %s", p1, p2);
		if (strncmp(p1, "U1", 2) == 0)
		{
			obj->u1FromFile = true;
		}
		if (strncmp(p1, "U2", 2) == 0)
		{
			obj->u2FromFile = true;
		}
		if (params == 2)
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
	}
	/*
	// Warnings and errors are printed by the system text printer.
	if (t.type == vrpn_TEXT_NORMAL) {
		printf("%s: Text message: %s\n", name, t.message);
	}
	*/
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
handle_cars(void *userdata, const vrpn_TRACKERCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	// Process information from car simulator

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
handle_file_cars(void *userdata, const vrpn_TRACKERCB t)
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

