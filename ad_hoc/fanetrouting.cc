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

#include <string>
#include <iostream>
#include <vector>
#include <map>
using namespace ns3;

FanetRoutingExperiment::FanetRoutingExperiment()
  : m_total_sim_time(200.0),
    m_port (9),
    m_nsinks (1),
    m_txp (40),
    m_traceMobility (true),
    m_traffic_apps(NetTrafficCreator::NetTrafficClasses::UDP_CBR),
    // AODV
    m_protocol (2),
    m_rout_prot(RoutingHelper::ROUTING_PROTOCOL::AODV),

    m_lossModelName ("ns3::FriisPropagationLossModel"),
    m_phyMode ("OfdmRate6MbpsBW10MHz"),
    m_traceFile (""),
    m_nNodes (2),
    m_rate ("2048bps"),
    m_trName ("fanet-routing-compare"),
    m_nodeSpeed (200),
    m_nodePause (0),
    m_verbose (0),
    m_print_routingTables (true),
    m_asciiTrace (1),
    m_pcap (1),
    m_log (0),
    m_streamIndex (0),
    m_adhocTxNodes ()
{

  m_routingHelper = CreateObject<RoutingHelper> ();

  // set to non-zero value to enable
  // simply uncond logging during simulation run
  m_log = 1;
}

FanetRoutingExperiment::~FanetRoutingExperiment()
{
  for(auto it : m_dist_calc_trace)
  {
    delete it;
  }

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

}

void
FanetRoutingExperiment::SetDefaultAttributeValues ()
{
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (m_phyMode));
}

void
FanetRoutingExperiment::ParseCommandLineArguments (int argc, char **argv)
{
  CommandLine cmd;

  cmd.Parse (argc, argv);

  m_routingHelper->SetLogging (m_log);
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
  if (m_verbose)
  {
    wifi80211p.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
  }

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
  MobilityHelper mobilityAdhoc;

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomBoxPositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=20000.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=20000.0]"));
  pos.Set ("Z", StringValue ("ns3::UniformRandomVariable[Min=1800.0|Max=2200.0]"));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  m_streamIndex += taPositionAlloc->AssignStreams (m_streamIndex);

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=" << m_nodeSpeed * 0.9 << "|Max=" << m_nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << m_nodePause << "]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str ()),
                                  "Pause", StringValue (ssPause.str ()),
                                  "PositionAllocator", PointerValue (taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (m_adhocTxNodes);
  m_streamIndex += mobilityAdhoc.AssignStreams (m_adhocTxNodes, m_streamIndex);

  ObjectFactory init_pos;
  init_pos.SetTypeId("ns3::UniformDiscPositionAllocator");
  init_pos.Set("X", ns3::DoubleValue(10000.0));
  init_pos.Set("Y", ns3::DoubleValue(10000.0));
  init_pos.Set("Z", ns3::DoubleValue(2000.0));
  init_pos.Set("rho", ns3::DoubleValue(100.0));

  Ptr<PositionAllocator> initial_pos = init_pos.Create ()->GetObject<PositionAllocator> ();
  m_streamIndex += initial_pos->AssignStreams (m_streamIndex);

  AsciiTraceHelper ascii;
  for(auto it = m_adhocTxNodes.Begin(); it != m_adhocTxNodes.End(); it++)
  {
    Ptr<MobilityModel> node_mob = (*it)->GetObject<MobilityModel>();
    if(node_mob)
    {
      node_mob->SetPosition(initial_pos->GetNext());
    }
  }
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

void FanetRoutingExperiment::ConfigureTracing ()
{
  EnableLogComponent();

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

    //Mobility
    Ptr<MobilityModel> node_mob = (n)->GetObject<MobilityModel>();
    if(node_mob)
    {
      DistanceCalculatorAndTracer* mob_tmp = new DistanceCalculatorAndTracer;
      mob_tmp->CreateOutput(n_name + "-mob.csv");
      mob_tmp->SetNodeMobilityModel(node_mob);
      ns3::Simulator::Schedule(Time(0.0), &DistanceCalculatorAndTracer::DumperCb, mob_tmp, 1.0);
      m_dist_calc_trace.push_back(mob_tmp);

      //add node to all mob trace
      std::size_t s = n_name.find('-');
      m_all_nodes_mobility_trace.AddNodeMobilityModel(node_mob, n_name.substr(s+1));
    }
    //=======================

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

  m_all_nodes_mobility_trace.CreateOutput("all-nodes-mobs.csv");
  ns3::Simulator::Schedule(Time(0.0), &AllNodesMobilityTracer::DumperCb, &m_all_nodes_mobility_trace, 1.0);

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
  double averageRoutingGoodputKbps = 0.0;
  uint32_t totalBytesTotal = m_routingHelper->GetStatsCollector ().GetCumulativeRxBytes ();
  averageRoutingGoodputKbps = (((double) totalBytesTotal * 8.0) / m_total_sim_time) / 1000.0;

  if (m_log != 0)
  {

  }
}

