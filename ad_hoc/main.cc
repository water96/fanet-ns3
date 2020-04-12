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

using namespace ns3;

const std::string delim_str = "\n====================================\n";

void PosPrinter(ns3::NodeContainer& all_nodes)
{
  for(auto n = all_nodes.Begin (); n != all_nodes.End (); n++)
  {
    std::cout << "Node ID: " << (*n)->GetId () << std::endl;
    std::cout << "Mobility model: " << (*n)->GetObject<ns3::MobilityModel>()->GetTypeId ().GetName () << std::endl;
    std::cout << "Initila position: " << (*n)->GetObject<ns3::MobilityModel>()->GetPosition () << std::endl;
    std::cout << delim_str;
  }
}

int main()
{
  //=====================================
  LogComponentEnableAll (LogLevel::LOG_ALL);
  //=====================================
  uint8_t n_nodes = 4;
  double x_lim = 100;
  double y_lim = 100;
  double speed = 10;
  double sim_time = 10.0f;
  //=====================================

  //=====================================
  ns3::NodeContainer all_nodes;
  all_nodes.Create (n_nodes);
  //=====================================

  //=====================================
  ns3::MobilityHelper mob_hlpr;
  mob_hlpr.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mob_hlpr.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string (x_lim) + "]"),
                                 "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string (y_lim) + "]"));
  mob_hlpr.Install (all_nodes);
  for(auto n = all_nodes.Begin (); n != all_nodes.End (); n++)
  {
    std::cout << "Node ID: " << (*n)->GetId () << std::endl;
    std::cout << "Mobility model: " << (*n)->GetObject<ns3::MobilityModel>()->GetInstanceTypeId ().GetName () << std::endl;
    std::cout << "Initila position: " << (*n)->GetObject<ns3::MobilityModel>()->GetPosition () << std::endl;
    std::cout << delim_str;
  }
  //=====================================

  //=====================================
  ns3::AnimationInterface anim("ad_hoc.xml");
  //=====================================

  //=====================================
  //ns3::EventId p_id = Simulator::Schedule (Seconds (0.0),&PosPrinter, all_nodes);
  Simulator::Run ();
  Simulator::Destroy ();
  //=====================================
  return 0;
}
