#include "vrpn_Tracker_Proxy.h"

// temporal. These should be in the header, and not static
static vrpn_Tracker_Remote *tkr = NULL;
static	int	count = 0;

/*****************************************************************************
*
Callback handlers
*
*****************************************************************************/

void	VRPN_CALLBACK handle_pos(void *ptr, const vrpn_TRACKERCB t)
{

	vrpn_Tracker_Proxy* obj = static_cast<vrpn_Tracker_Proxy*>(ptr);

	obj->report_pose(t.sensor, t.msg_time, t.pos, t.quat);

	/*
	fprintf(stderr, "%d.", t.sensor);
	if ((++count % 20) == 0) {
		fprintf(stderr, "\n");
		if (count > 300) {
			printf("Pos, sensor %d = %f, %f, %f\n", t.sensor,
				t.pos[0], t.pos[1], t.pos[2]);
			count = 0;
		}
	}
	*/
}

void	VRPN_CALLBACK handle_vel(void *, const vrpn_TRACKERVELCB t)
{
	//static	int	count = 0;

	fprintf(stderr, "%d/", t.sensor);
}

void	VRPN_CALLBACK handle_acc(void *, const vrpn_TRACKERACCCB t)
{
	//static	int	count = 0;

	fprintf(stderr, "%d~", t.sensor);
}

vrpn_Tracker_Proxy::vrpn_Tracker_Proxy(const char *name, vrpn_Connection *c, const char* remoteService, int numSensors)
  : vrpn_Tracker_Server(name,c,numSensors)
{
  if(d_connection) {
	tkr = new vrpn_Tracker_Remote(remoteService);
	tkr->register_change_handler(this, handle_pos);
	tkr->register_change_handler(this, handle_vel);
	tkr->register_change_handler(this, handle_acc);
  }
  // ...
}

//
vrpn_Tracker_Proxy::~vrpn_Tracker_Proxy()
{
#ifdef DEBUG
  printf("%s\n", __PRETTY_FUNCTION__);
#endif

}


// This function should be called each time through the main loop
// of the server code. It polls for data from the OWL server and
// sends them if available.
void vrpn_Tracker_Proxy::mainloop()
{
	tkr->mainloop();

  // Call the generic server mainloop, since we are a server
  server_mainloop();
  return;
}

bool vrpn_Tracker_Proxy::enable(bool newState)
{
	if( tkr != NULL )
		return true;
	return false;
}
