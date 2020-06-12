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
    m_txp (40),   //1
    m_traffic_model(""),  //1
    m_mob_scenario(FanetMobilityCreator::Inst().GetDefaultModel()),   //1
    // AODV
    m_rout_prot("AODV"),
    //m_lossModelName ("ns3::RangePropagationLossModel"),
    m_lossModelName ("ns3::FriisPropagationLossModel"),
    m_phyMode ("OfdmRate27MbpsBW10MHz"),
    m_nNodes (2),
    m_trName ("fanet-routing-compare"),
    m_nodeSpeed (200.0),
    m_asciiTrace (0),
    m_pcap (0),
    m_log (0),
    m_streamIndex (0),
    m_adhocTxNodes (),
    m_prop_model_ptr(nullptr)
{
  TracerBase::ResetAllTraceResults();
}

TypeId FanetRoutingExperiment::GetTypeId (void)
{
  static TypeId tid = TypeId ("FanetRoutingExperiment")
    .SetParent<ExperimentApp> ()
    .AddConstructor<FanetRoutingExperiment> ()
    .AddAttribute ("nodes", "Number of nodes in simulation",
                   UintegerValue (2),
                   MakeUintegerAccessor (&FanetRoutingExperiment::m_nNodes),
                   MakeUintegerChecker<uint32_t> (2, 100))
    .AddAttribute ("time", "Total simulation time",
                   DoubleValue (),
                   MakeDoubleAccessor (&FanetRoutingExperiment::m_total_sim_time),
                   MakeDoubleChecker<double>(0.0, 1000.0))
    .AddAttribute ("power", "Tx power",
                   DoubleValue (40.0),
                   MakeDoubleAccessor (&FanetRoutingExperiment::m_txp),
                   MakeDoubleChecker<double>(0.0, 100.0))
    .AddAttribute ("traffic", "Traffic model in the network",
                   StringValue (""),
                   MakeStringAccessor (&FanetRoutingExperiment::m_traffic_model),
                   MakeStringChecker())
    .AddAttribute ("mobility", "Mobility model in the network",
                   StringValue (FanetMobilityCreator::Inst().GetDefaultModel()),
                   MakeStringAccessor (&FanetRoutingExperiment::m_mob_scenario),
                   MakeStringChecker())
    .AddAttribute ("routing", "Routing protocol in the network",
                   StringValue ("AODV"),
                   MakeStringAccessor (&FanetRoutingExperiment::m_rout_prot),
                   MakeStringChecker())
    .AddAttribute ("speed", "Nodes speed",
                   DoubleValue (200.0),
                   MakeDoubleAccessor (&FanetRoutingExperiment::m_nodeSpeed),
                   MakeDoubleChecker<double>(0.0, 400.0))
  ;
  return tid;
}


FanetRoutingExperiment::~FanetRoutingExperiment()
{
  FanetMobilityCreator::Inst().DestroyMobilityModel();
  Names::Clear();
}

void
FanetRoutingExperiment::SetDefaultAttributeValues ()
{
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (m_phyMode));
}

void
FanetRoutingExperiment::ParseCommandLineArguments (int argc, char **argv)
{

}

void FanetRoutingExperiment::ConfigureNodes ()
{
  m_adhocTxNodes.Create (m_nNodes);

  uint16_t cntr = 1;
  for(auto it = m_adhocTxNodes.Begin(); it != m_adhocTxNodes.End(); it++)
  {
    ns3::Ptr<Node> n = *it;
    std::stringstream ss;
    ss << "n" << cntr;
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
  wifiChannel.AddPropagationLoss (m_lossModelName);

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
  wifiPhy.Set ("TxPowerStart", DoubleValue (m_txp));
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
      std::size_t s = n.find_last_of('n');
      if(s != std::string::npos)
      {
        n = n.substr(s + 1);
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

  ns3::PointerValue ptr_loss;
  channel->GetAttribute("PropagationLossModel", ptr_loss);

  m_prop_model_ptr = ptr_loss.Get<PropagationLossModel>();
  if(m_lossModelName == "ns3::RangePropagationLossModel")
  {
    m_prop_model_ptr->SetAttribute("MaxRange", ns3::DoubleValue(1500.0));
  }
  else if(m_lossModelName == "ns3::FriisPropagationLossModel")
  {
    m_prop_model_ptr->SetAttribute("Frequency", ns3::DoubleValue(freq));
  }

}

void FanetRoutingExperiment::ConfigureMobility ()
{
  FanetMobilityCreator::Inst().CreateMobilityModel(m_mob_scenario);
  FanetMobility& mob = FanetMobilityCreator::Inst().GetMobilityModel();

  mob.SetSimulationTime(ns3::Seconds(m_total_sim_time));
  mob.SetMobilityAreaAndSpeed(ns3::Vector3D(20000.0, 20000.0, 2000.0), m_nodeSpeed);
  m_streamIndex += mob.Install(m_adhocTxNodes, m_streamIndex);
}

void FanetRoutingExperiment::ConfigureApplications ()
{

  m_routingHelper.Install(m_adhocTxNodes,
                           m_adhocTxDevices,
                           m_adhocTxInterfaces,
                           m_total_sim_time,
                           m_rout_prot);


  //apps
  std::vector<std::string> traffic_apps_id;
  utils::SplitString(m_traffic_model, ";", traffic_apps_id);

  if((m_traffic_model != "") && traffic_apps_id.empty())
  {
    traffic_apps_id.push_back(m_traffic_model);
  }

  for(auto it : traffic_apps_id)
  {
    ObjectFactory f(it);
    ns3::Ptr<NetTraffic> tr = f.Create<NetTraffic>();

    tr->SetSimulationTime(m_total_sim_time);
    m_streamIndex += tr->Install(m_adhocTxNodes, m_adhocTxDevices, m_adhocTxInterfaces, m_streamIndex);
    m_traffic_generators.push_back(tr);
  }
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

const ExpResults& FanetRoutingExperiment::GetSimulationResults() const
{
  return m_results;
}

void FanetRoutingExperiment::ConfigureTracing ()
{
  //=============================
  FanetMobilityCreator::Inst().GetMobilityModel().ConfigureMobilityTracing();
  //=============================

  //=============================
  std::for_each(m_traffic_generators.begin(),
                m_traffic_generators.end(),
                [](ns3::Ptr<NetTraffic> tr) {
                                              tr->ConfigreTracing();
                                            });
  //=============================

  //=============================
  m_routingHelper.ConfigureTracing();
  //=============================

  //=============================
//  m_adj_tx_tracer = CreateObject<AdjTxPowerTracer>();
//  m_adj_tx_tracer->SetDumpInterval(1.0);
//  m_adj_tx_tracer->CreateOutput("tx-power.csv");
//  m_adj_tx_tracer->SetNodesAndDelayModel(m_adhocTxDevices, m_prop_model_ptr);
  //=============================

  if(m_log)
  {
    EnableLogComponent();
  }

  if(m_asciiTrace)
  {
    AsciiTraceHelper ascii;
    MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (m_trName + ".mob"));
  }

  //Through nodes
  for(auto it = m_adhocTxNodes.Begin(); it != m_adhocTxNodes.End(); it++)
  {

  }

  //Through devices
  for(auto it = m_adhocTxDevices.Begin(); it != m_adhocTxDevices.End(); it++)
  {

  }
}

void FanetRoutingExperiment::RunSimulation ()
{
  NS_LOG_INFO ("Run Simulation.");

  Simulator::Stop (Seconds (m_total_sim_time));
  Simulator::Run ();
  Simulator::Destroy ();
}

void FanetRoutingExperiment::ProcessOutputs ()
{
  //GetResults from all tracers objects
  ExpResults r(TracerBase::GetAllTraceResults());
  m_results.insert(r.begin(), r.end());

}

