#include "adhoc.h"

#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/gpsr-module.h"

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
  std::vector<std::pair<std::string, ns3::AttributeValue*> > attr_val_list;

  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  if(m_protocol == "AODV")
  {
    routing = new AodvHelper;
    attr_val_list.push_back(std::make_pair("EnableHello", new BooleanValue(true)));
  }
  else if(m_protocol == "OLSR")
  {
    routing = new OlsrHelper;
  }
  else if(m_protocol == "GPSR")
  {
    routing = new GpsrHelper;
  }
  else
  {

  }

  if(routing)
  {
    list.Add(*routing, prior);
    internet.SetRoutingHelper (list);
    delete routing;
  }

  internet.Install (c);

  for(auto it = c.Begin(); it != c.End(); it++)
  {
    Ptr<Ipv4RoutingProtocol> routing = (*it)->GetObject<Ipv4RoutingProtocol>();
    Ptr<aodv::RoutingProtocol> r = DynamicCast<aodv::RoutingProtocol>(routing);
    if(routing)
    {
      for(auto vars : attr_val_list)
      {
        routing->SetAttribute(vars.first, *vars.second);
      }
    }
  }

  std::for_each(attr_val_list.begin(),
                attr_val_list.end(),
                [](std::pair<std::string, ns3::AttributeValue*>& p) {
                                                                      delete p.second;
                                                                    });

  //internet.EnablePcapIpv4All("ip-pcap");
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
    AsciiTraceHelper ascii;

  if(m_protocol == "AODV")
  {
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
    m_ip_lev_tracer.AddCollectingStatsFrom(ip, n_name);
  }

  m_net_adj_tracer = CreateObject<NetworkAdjTracer>();
  m_net_adj_tracer->CreateOutput("network-real.csv");
  m_net_adj_tracer->SetNodeIfces(m_ifs);
  m_net_adj_tracer->SetDumpInterval(1.0);
}


