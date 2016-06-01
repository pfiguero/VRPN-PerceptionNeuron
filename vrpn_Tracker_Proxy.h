#ifndef VRPN_TRACKER_PROXY_H
#define VRPN_TRACKER_PROXY_H

#include "vrpn_Configure.h"

#include <map>
#include <vector>

#include "vrpn_Shared.h"
#include "vrpn_Tracker.h"


class VRPN_API vrpn_Tracker_Proxy : public vrpn_Tracker_Server {

public:

  vrpn_Tracker_Proxy(const char *name, 
                          vrpn_Connection *c,
                          const char* remoteService, 
							int numSensors);


  ~vrpn_Tracker_Proxy();

  // Tries to open or close the connection to the proxy
  bool enable(bool newState);

  virtual void mainloop();

  static int VRPN_CALLBACK handle_update_rate_request(void *userdata, vrpn_HANDLERPARAM p);  

private:
	vrpn_Tracker_Remote *tkr;
};
#endif
