#include "vrpn_WWA_Server.h"

vrpn_WWA_Server::vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, const char *consoleDeviceTxt, const char *nameHeadsTrk, int nH, const char *nameBodiesTrk, int nB,
	const char *nameCars, const char *expDirectory): vrpn_Text_Sender(nameTxt, c)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif

	hasRealTrackers = false;

	// Create internal devices and clients
	console = new vrpn_Text_Receiver(consoleDeviceTxt);
	console->register_message_handler(this, handle_console_commands);

	headTracker = new vrpn_Tracker_Server(nameHeadsTrk,c,nH);
	bodiesTracker = new vrpn_Tracker_Server(nameBodiesTrk, c, nB);
	carsServer = new vrpn_Tracker_Server(nameCars,c,1);
	expDir = "";
	expDir += expDirectory;

	headTrackerReader = NULL;
	bodyTrackerReader = NULL;
	carReader = NULL;

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

vrpn_WWA_Server::vrpn_WWA_Server(vrpn_Connection *c, const char *nameTxt, const char *consoleDeviceTxt, const char *nameHeadsTrk, int nH, const char *nameBodiesTrk, int nB,
	const char *nameCars, const char *expDirectory, const char *headsDeviceTrk, const char *bodiesDeviceTrk, const char *carsDevice): 
	vrpn_WWA_Server(c, nameTxt, consoleDeviceTxt, nameHeadsTrk, nH, nameBodiesTrk, nB, nameCars, expDirectory)
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif

	hasRealTrackers = true;

	// Create extra internal devices and clients
	headTrackerReader = new vrpn_Tracker_Remote(headsDeviceTrk);
	headTrackerReader->register_change_handler(this, handle_heads_pos_quat);
	bodyTrackerReader = new vrpn_Tracker_Remote(bodiesDeviceTrk);
	bodyTrackerReader->register_change_handler(this, handle_body_pos_quat);
	carReader = new vrpn_Tracker_Remote(carsDevice);

	printf("Creating WWA_Server with parameters: %s %s %s %d %s %d %s %s %s %s %s\n", nameTxt,
		consoleDeviceTxt, nameHeadsTrk, nH, nameBodiesTrk, nB, nameCars, expDirectory, headsDeviceTrk, bodiesDeviceTrk, carsDevice );
}

//
vrpn_WWA_Server::~vrpn_WWA_Server()
{
#ifdef DEBUG
	printf("%s\n", __PRETTY_FUNCTION__);
#endif
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
	if (console != NULL)
		console->mainloop();
	if (headTrackerReader != NULL)
		headTrackerReader->mainloop();
	if (bodyTrackerReader != NULL)
		bodyTrackerReader->mainloop();
	if (carReader != NULL)
		carReader->mainloop();

	if (headTracker != NULL)
		headTracker->mainloop();
	if (bodiesTracker != NULL)
		bodiesTracker->mainloop();
	if (carsServer != NULL)
		carsServer->mainloop();

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
	printf("Receiving command: %s\n", command);

	if (strncmp(command, "LOAD ", 5) == 0)
	{
		command += 5; // now commands sees the file name
		std::string filename = obj->getExpDir();
		filename += "\\";
		filename += command;
		printf("LOAD file: %s\n", filename.c_str());
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

	// Process information from the head's tracker

}

void VRPN_CALLBACK
handle_body_pos_quat(void *userdata, const vrpn_TRACKERCB t)
{
	vrpn_WWA_Server *obj = static_cast<vrpn_WWA_Server *>(userdata);

	// Process information from the body's tracker

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
