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
#include "utils/script.h"

using namespace ns3;

class ConnectivityTracer : public TracerBase
{
private:

public:
  void CreateOutput(const std::string& name)
  {
    TracerBase::CreateOutput(name);

    TracerBase::m_out << "nodes" << m_delimeter
                      << "con" << m_delimeter
                      << std::endl;
  }

  void DumbConnectivity(uint32_t n, double val)
  {
    TracerBase::m_out << n << m_delimeter
                      << val <<m_delimeter
                      << std::endl;
  }
};


int main(int argc, char **argv)
{
  //=====================================
  //LogComponentEnableAll (LogLevel::LOG_ALL);
  //=====================================

  std::cout << "test!\n";

  //=====================================

  std::string out_dir = Script::GetEnv("NS3_FANET_OUTPUT_DIR");
  if(out_dir.length())
  {
    Script::MkDir(out_dir);
  }

  Script::ChDir(out_dir);

  ConnectivityTracer tr;
  tr.CreateOutput("real_connectivity.csv");
  for(uint32_t n = 2; n <= 20; n+=2)
  {
    Script::ChDir(out_dir);
    Ptr<FanetRoutingExperiment> exp = CreateObject<FanetRoutingExperiment>();
    exp->SetAttribute("Nodes", UintegerValue(n));
    exp->SetAttribute("Stream", UintegerValue(0));
    exp->Simulate(argc, argv);
    ExpResults r = exp->GetSimulationResults();
    double d = std::stod(r["RealConnectivity"]);
    tr.DumbConnectivity(n, d);
  }

  return 0;
}
