#include <iostream>
#include <fstream>
#include <functional>

#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/csma-module.h"
#include "ns3/csma-star-helper.h"
#include "ns3/bridge-module.h"
#include "ns3/wifi-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/v4ping.h"
#include "ns3/v4ping-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/rectangle.h"
#include "ns3/animation-interface.h"
#include "ns3/point-to-point-helper.h"

#include "utils/tracers.h"
#include "fanetrouting.h"

using namespace ns3;

int main(int argc, char **argv)
{
  //=====================================
  //LogComponentEnableAll (LogLevel::LOG_ALL);
  //=====================================

  //Redirect all output to file
  std::streambuf* old = std::cout.rdbuf();
  std::ofstream f_cout("fanet-routing-сout.log");
  std::ofstream f_clog("fanet-routing-сlog.log");
  std::cout.rdbuf(f_cout.rdbuf());
  std::clog.rdbuf(f_clog.rdbuf());

  std::cout << "test!\n";

  //=====================================

  FanetRoutingExperiment exp;
  exp.Simulate(argc, argv);

  f_cout.close();
  f_clog.close();
  return 0;
}
