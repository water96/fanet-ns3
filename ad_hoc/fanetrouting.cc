#include "fanetrouting.h"
#include "ns3/names.h"

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/config-store-module.h"
#include "ns3/integer.h"
#include "ns3/wave-bsm-helper.h"
#include "ns3/wave-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/v4ping.h"

#include "utils/script.h"

#include <string>
#include <iostream>
#include <vector>
#include <map>
using namespace ns3;

FanetRoutingExperiment::FanetRoutingExperiment()
  : m_total_sim_time(200.0),  //1
    m_nsinks (1),
    m_txp (40),   //1
    m_traffic_apps(NetTrafficCreator::NetTrafficClasses::UDP_CBR),  //1
    m_mob_scenario(""),   //1
    // AODV
    m_rout_prot(RoutingHelper::ROUTING_PROTOCOL::AODV),

    m_lossModelName ("ns3::FriisPropagationLossModel"),
    m_phyMode ("OfdmRate27MbpsBW10MHz"),
    m_nNodes (2),
    m_trName ("fanet-routing-compare"),
    m_nodeSpeed (200),
    m_verbose (0),
    m_print_routingTables (true),
    m_asciiTrace (1),
    m_pcap (1),
    m_log (0),
    m_streamIndex (0),
    m_adhocTxNodes ()
{

  m_routingHelper = CreateObject<RoutingHelper> ();

}

TypeId FanetRoutingExperiment::GetTypeId (void)
{
  static TypeId tid = TypeId ("FanetRoutingExperiment")
    .SetParent<ExperimentApp> ()
    .AddConstructor<FanetRoutingExperiment> ()
    .AddAttribute ("Nodes", "Number of nodes in simulation",
                   UintegerValue (2),
                   MakeUintegerAccessor (&FanetRoutingExperiment::m_nNodes),
                   MakeUintegerChecker<uint32_t> (2, 100))
    .AddAttribute ("Stream", "Seed of random number generation",
                   UintegerValue (0),
                   MakeUintegerAccessor (&FanetRoutingExperiment::m_streamIndex),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}


FanetRoutingExperiment::~FanetRoutingExperiment()
{
  for(auto it : m_wifi_phy_tracers)
  {
    delete it;
  }

  for(auto it : m_wifi_state_tracers)
  {
    delete it;
  }

  for(auto it : m_ipv4_tracers)
  {
    delete it;
  }

  Names::Clear();
}

void
FanetRoutingExperiment::SetDefaultAttributeValues ()
{
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (m_phyMode));
  m_mob_scenario = m_nodes_mobility.GetDefaultModel();
}

void
FanetRoutingExperiment::ParseCommandLineArguments (int argc, char **argv)
{
  CommandLine cmd;

  cmd.Parse (argc, argv);

  std::string out_dir = ".";

  std::string script_name = m_mob_scenario + "-" +
                            std::to_string(static_cast<uint32_t>(m_rout_prot)) + "-" +
                            std::to_string(m_nNodes) + "n-" +
                            std::to_string(m_total_sim_time) + "s";
  Script::Instance().SetScriptName(script_name);
  Script::Instance().CreateOutputDir(out_dir);
  Script::ChDir(Script::Instance().GetOutputDir());
}

void FanetRoutingExperiment::ConfigureNodes ()
{
  m_adhocTxNodes.Create (m_nNodes);

  uint16_t cntr = 1;
  for(auto it = m_adhocTxNodes.Begin(); it != m_adhocTxNodes.End(); it++)
  {
    ns3::Ptr<Node> n = *it;
    std::stringstream ss;
    ss << "n-" << cntr;
    Names::Add(ss.str(), n);
    cntr++;
  }
}

void FanetRoutingExperiment::ConfigureChannels ()
{

}

void FanetRoutingExperiment::ConfigureDevices ()
{
  // frequency
  // 802.11p 5.9 GHz
  double freq = 5.9e9;

  // Setup propagation models
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss (m_lossModelName, "Frequency", DoubleValue (freq));

  // the channel
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();

  // The below set of helpers will help us to put together the wifi NICs we want
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (channel);
  // ns-3 supports generate a pcap trace
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);

   // Setup WAVE PHY and MAC
  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();

  WifiHelper wifi;

  // Setup 802.11p stuff
  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue (m_phyMode),
                                      "ControlMode", StringValue (m_phyMode));

  // Set Tx Power
  wifiPhy.Set ("TxPowerStart",DoubleValue (m_txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (m_txp));

  // Setup net devices
  m_adhocTxDevices = wifi80211p.Install (wifiPhy, wifi80211pMac, m_adhocTxNodes);

  uint16_t cnter = 1;
  for(auto it = m_adhocTxDevices.Begin(); it != m_adhocTxDevices.End(); it++)
  {
    std::string n = Names::FindName((*it)->GetNode());
    uint16_t num = cnter;
    if(n.empty() == false)
    {
      std::size_t s = n.find_last_of('-');
      if(s != std::string::npos)
      {
        n = n.substr(s + 1, n.size() - s);
        num = std::stoi(n);
      }
    }

    char tmp[20];
    std::sprintf(tmp, "00:00:00:00:00:%0*x", 2, num);
    ns3::Mac48Address addr(tmp);
    (*it)->SetAddress(addr);
    cnter++;
  }

  if (m_asciiTrace != 0)
  {
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ( (m_trName + ".tr").c_str ());
    wifiPhy.EnableAsciiAll (osw);
  }
  if ( m_pcap != 0)
  {
    wifiPhy.EnablePcapAll ("wifi-pcap");
  }
}

void FanetRoutingExperiment::ConfigureMobility ()
{
  m_nodes_mobility.Create(m_mob_scenario);
  FanetMobility& mob = m_nodes_mobility.Inst();

  mob.SetStreamIndex(m_streamIndex);
  mob.SetSimulationTime(ns3::Seconds(m_total_sim_time));
  mob.SetMobilityAreaAndSpeed(ns3::Vector3D(20000.0, 20000.0, 2000.0), m_nodeSpeed);
  mob.Install(m_adhocTxNodes);
}

void FanetRoutingExperiment::ConfigureApplications ()
{

  m_routingHelper->Install(m_adhocTxNodes,
                           m_adhocTxDevices,
                           m_adhocTxInterfaces,
                           m_total_sim_time,
                           m_rout_prot,
                           m_nsinks,
                           m_print_routingTables);

  switch (m_rout_prot)
    {
      case RoutingHelper::ROUTING_PROTOCOL::NONE:

      break;
      case RoutingHelper::ROUTING_PROTOCOL::OLSR:

      break;
      case RoutingHelper::ROUTING_PROTOCOL::AODV:

        for(auto it = m_adhocTxNodes.Begin(); it != m_adhocTxNodes.End(); it++)
        {
          Ptr<Ipv4RoutingProtocol> routing = (*it)->GetObject<Ipv4RoutingProtocol>();
          if(routing)
          {
            routing->SetAttribute("EnableHello", BooleanValue(false));
          }
        }

      break;
      case RoutingHelper::ROUTING_PROTOCOL::DSDV:

      break;
      case RoutingHelper::ROUTING_PROTOCOL::DSR:

      break;
    }

  m_traffic_creator.Create(m_traffic_apps, m_streamIndex, m_total_sim_time);
  m_traffic_creator.Inst().Install(m_adhocTxNodes, m_adhocTxInterfaces);

//  Simulator::Schedule(Seconds(20.0), &Ipv4::SetDown, m_adhocTxInterfaces.Get(1).first, m_adhocTxInterfaces.Get(1).second);
//  Simulator::Schedule(Seconds(27.0), &Ipv4::SetUp, m_adhocTxInterfaces.Get(1).first, m_adhocTxInterfaces.Get(1).second);

}

void FanetRoutingExperiment::EnableLogComponent()
{
  const std::pair<std::string, ns3::LogLevel> log_component_list_to_enable[] =
  {
    std::make_pair("Ipv4*", ns3::LogLevel::LOG_ALL),
    std::make_pair("Wifi*", ns3::LogLevel::LOG_ALL),
    std::make_pair("Mobility*", ns3::LogLevel::LOG_ALL),
    std::make_pair("Routing*", ns3::LogLevel::LOG_ALL)
  };

  const LogComponent::ComponentList* all = LogComponent::GetComponentList();

  for(auto it = std::begin(log_component_list_to_enable); it != std::end(log_component_list_to_enable); it++)
  {
    std::size_t pos = it->first.find('*');
    if( pos != std::string::npos)
    {
      std::string s = it->first.substr(0, pos);
      for(auto c = all->begin(); c != all->end(); c++)
      {
        if(c->first.find(s) != std::string::npos)
        {
          LogComponentEnable(c->first.c_str(), it->second);
        }
      }
    }
    else
    {
      LogComponentEnable(it->first.c_str(), it->second);
    }
  }

  Packet::EnablePrinting ();
}

ExpResults FanetRoutingExperiment::GetSimulationResults() const
{
  return m_results;
}

void FanetRoutingExperiment::ConfigureTracing ()
{
  //=============================
  m_nodes_mobility.Inst().ConfigureMobilityTracing();
  //=============================

  m_routingHelper->SetLogging (m_log);

  if(m_log)
  {
    EnableLogComponent();
  }

  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (m_trName + ".mob"));

  //AnimationInterface anim ("animation.xml");  // where "animation.xml" is any arbitrary filenames
  //anim.SetMobilityPollInterval(Seconds(1.0));
  //anim.EnableWifiMacCounters(Time(0), Seconds(m_total_sim_time));
  //anim.EnableWifiPhyCounters(Time(0), Seconds(m_total_sim_time));
  //anim.EnableIpv4RouteTracking("anim_rt_tables.rt", Time(0), Seconds(m_total_sim_time), Seconds(1.0));
  //anim.EnableIpv4L3ProtocolCounters(Time(0), Seconds(m_total_sim_time));


  PointerValue tmp_ptr_val;

  //Through nodes
  for(auto it = m_adhocTxNodes.Begin(); it != m_adhocTxNodes.End(); it++)
  {
    Ptr<Node> n = *it;
    std::string n_name = Names::FindName(n);

    //Wifi Phy
    Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice>(n->GetDevice(0));
    Ptr<WifiPhy> phy = dev->GetPhy();
    if(phy)
    {
      WifiPhyTracer* wifi_phy_tracer = new WifiPhyTracer();
      wifi_phy_tracer->CreateOutput(n_name + "-wifi-phy-drop.csv");
      phy->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&WifiPhyTracer::WifiPhyDropCb, wifi_phy_tracer));
      m_wifi_phy_tracers.push_back(wifi_phy_tracer);
    }
    //=======================

    //Wifi Phy state
    phy->GetAttribute("State", tmp_ptr_val);
    Ptr<WifiPhyStateHelper> state_hlp = DynamicCast<WifiPhyStateHelper>(tmp_ptr_val.Get<WifiPhyStateHelper>());
    if(state_hlp)
    {
      WifiPhyStateTracer* wifi_state_tracer = new WifiPhyStateTracer();
      wifi_state_tracer->CreateOutput("wifi-" + n_name + ".csv");
      state_hlp->TraceConnectWithoutContext("RxOk", MakeCallback(&WifiPhyStateTracer::RxOkCb, wifi_state_tracer));
      state_hlp->TraceConnectWithoutContext("RxError", MakeCallback(&WifiPhyStateTracer::RxErrorCb, wifi_state_tracer));
      state_hlp->TraceConnectWithoutContext("Tx", MakeCallback(&WifiPhyStateTracer::TxCb, wifi_state_tracer));
      m_wifi_state_tracers.push_back(wifi_state_tracer);
    }
    //=======================
  }

  //Through devices
  for(auto it = m_adhocTxDevices.Begin(); it != m_adhocTxDevices.End(); it++)
  {

  }

  //Through ip interaces
  for(auto it = m_adhocTxInterfaces.Begin(); it != m_adhocTxInterfaces.End(); it++)
  {
    Ptr<Ipv4> ip = it->first;
    std::string n_name = Names::FindName(ip->GetNetDevice(it->second)->GetNode());
    Ipv4L3ProtocolTracer* tmp = new Ipv4L3ProtocolTracer();
    tmp->CreateOutput("ipv4-" + n_name + ".csv");
    ip->TraceConnectWithoutContext("Tx", MakeCallback(&Ipv4L3ProtocolTracer::TxCb, tmp));
    ip->TraceConnectWithoutContext("Rx", MakeCallback(&Ipv4L3ProtocolTracer::RxCb, tmp));
    ip->TraceConnectWithoutContext("Drop", MakeCallback(&Ipv4L3ProtocolTracer::DropCb, tmp));
    ip->TraceConnectWithoutContext("UnicastForward", MakeCallback(&Ipv4L3ProtocolTracer::UnicastForwardCb, tmp));
    ip->TraceConnectWithoutContext("LocalDeliver", MakeCallback(&Ipv4L3ProtocolTracer::LocalDeliverCb, tmp));
    m_ipv4_tracers.push_back(tmp);
  }

  //GlobalTracers
  Ipv4L3ProtocolTracer::DumpStatsTo("all-network-stats-ipv4.csv");
  ns3::Simulator::Schedule(ns3::Seconds(0.0), &Ipv4L3ProtocolTracer::StatsDumper, 1.0);
  //=======================
}

void FanetRoutingExperiment::RunSimulation ()
{
  NS_LOG_INFO ("Run Simulation.");

  Simulator::Stop (Seconds (m_total_sim_time));
  Simulator::Run ();
  LogComponentDisableAll(LogLevel::LOG_ALL);
  Simulator::Destroy ();
}

void FanetRoutingExperiment::ProcessOutputs ()
{
  //Close ipv4stats
  Ipv4L3ProtocolTracer::Stop();

  //GetResults
  ExpResults& udp_res = m_traffic_creator.Inst().GetResultsMap();
  m_results.insert(udp_res.begin(), udp_res.end());

}

