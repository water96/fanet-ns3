#include "adhoc.h"

#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

/*
 * ====================================================
 */

const double RoutingHelper::TOTAL_SIM_TIME = 300.01;
const uint16_t RoutingHelper::PORT = 9;

TypeId
RoutingHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RoutingHelper")
    .SetParent<Object> ()
    .AddConstructor<RoutingHelper> ();
  return tid;
}

RoutingHelper::RoutingHelper ()
  : m_TotalSimTime (TOTAL_SIM_TIME),
    m_protocol ("")
{
}

RoutingHelper::~RoutingHelper ()
{
}

void
RoutingHelper::Install (NodeContainer & c,
                        NetDeviceContainer & d,
                        Ipv4InterfaceContainer & i,
                        double totalTime,
                        std::string protocol)
{
  m_TotalSimTime = totalTime;
  m_protocol = protocol;

  SetupRoutingProtocol (c);
  AssignIpAddresses (d, i);

  m_nodes = c;
  m_devs = d;
  m_ifs = i;
}

void
RoutingHelper::SetupRoutingProtocol (NodeContainer & c)
{
  Ipv4RoutingHelper* routing = nullptr;
  Ptr<Ipv4RoutingProtocol> prot;
  const uint16_t prior = 100;

  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  AsciiTraceHelper ascii;

  if(m_protocol == "AODV")
  {
    routing = new AodvHelper;
  }
  else if(m_protocol == "OLSR")
  {
    routing = new OlsrHelper;
  }
  else if(m_protocol == "GPSR")
  {

  }
  else
  {

  }

  if(routing)
  {
    for (auto it = c.Begin(); it != c.End(); it++)
    {
      std::string n_name = Names::FindName(*it);
      if(n_name.empty())
      {
        n_name = "node-" + std::to_string((*it)->GetId());
      }
      Ptr<OutputStreamWrapper> rtw = ascii.CreateFileStream (n_name + ".rt");
      routing->PrintRoutingTableEvery(Seconds(1.0), *it, rtw);
    }

    list.Add(*routing, prior);
    internet.SetRoutingHelper (list);
    delete routing;
  }

  internet.Install (c);

  //internet.EnablePcapIpv4All("ip-pcap");
  //internet.EnableAsciiIpv4All("ip-ascii");
}

void
RoutingHelper::AssignIpAddresses (NetDeviceContainer & d,
                                  Ipv4InterfaceContainer & adhocTxInterfaces)
{
  NS_LOG_INFO ("Assigning IP addresses");
  Ipv4AddressHelper addressAdhoc;
  // we may have a lot of nodes, and want them all
  // in same subnet, to support broadcast
  NS_LOG_INFO ("Calculate mask:");

  addressAdhoc.SetBase ("10.0.1.0", "255.255.255.0");

  adhocTxInterfaces = addressAdhoc.Assign (d);
  uint16_t cntr = 1;
  for(auto it = adhocTxInterfaces.Begin(); it != adhocTxInterfaces.End(); it++)
  {
    Ptr<Ipv4> ip = it->first;

    std::string n = Names::FindName(ip->GetNetDevice(it->second)->GetNode());
    if(n.empty() == false)
    {
      std::size_t s = n.find_last_of('n');
      if(s != std::string::npos)
      {
        n = n.substr(s + 1);
      }
    }
    else
    {
      n = std::to_string(cntr);
    }

    std::string a = "10.0.1." + n;
    Ipv4InterfaceAddress addr(Ipv4Address(a.c_str()), Ipv4Mask(0xFFFFFF00));
    ip->RemoveAddress(it->second, 0);
    ip->AddAddress(it->second, addr);

    cntr++;
  }
}

StatsCollector &
RoutingHelper::GetStatsCollector ()
{
  return m_stats_collector;
}

void
RoutingHelper::ConfigureTracing()
{
  if(m_protocol == "AODV")
  {
    for(auto it = m_nodes.Begin(); it != m_nodes.End(); it++)
    {
      Ptr<Ipv4RoutingProtocol> routing = (*it)->GetObject<Ipv4RoutingProtocol>();
      if(routing)
      {
        routing->SetAttribute("EnableHello", BooleanValue(false));
      }
    }
  }
  else if(m_protocol == "OLSR")
  {
  }
  else if(m_protocol == "GPSR")
  {

  }

  m_ip_lev_tracer.SetDumpInterval(1.0);
  m_ip_lev_tracer.CreateOutput("ipv4.csv");

  //Through ip interaces
  for(auto it = m_ifs.Begin(); it != m_ifs.End(); it++)
  {
    Ptr<Ipv4> ip = it->first;
    std::string n_name = Names::FindName(ip->GetNetDevice(it->second)->GetNode());
    ns3::Ptr<Ipv4L3ProtocolTracer> tmp = CreateObject<Ipv4L3ProtocolTracer>();
    tmp->CreateOutput("ipv4-" + n_name + ".csv");
    ip->TraceConnectWithoutContext("Tx", MakeCallback(&Ipv4L3ProtocolTracer::TxCb, tmp));
    ip->TraceConnectWithoutContext("Rx", MakeCallback(&Ipv4L3ProtocolTracer::RxCb, tmp));
    ip->TraceConnectWithoutContext("Drop", MakeCallback(&Ipv4L3ProtocolTracer::DropCb, tmp));
    ip->TraceConnectWithoutContext("UnicastForward", MakeCallback(&Ipv4L3ProtocolTracer::UnicastForwardCb, tmp));
    ip->TraceConnectWithoutContext("LocalDeliver", MakeCallback(&Ipv4L3ProtocolTracer::LocalDeliverCb, tmp));
    m_ipv4_tracers.push_back(tmp);
    m_ip_lev_tracer.AddCollectingStatsFrom(ip, n_name);
  }
}


