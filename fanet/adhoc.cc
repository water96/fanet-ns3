#include "adhoc.h"

#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/gpsr-module.h"
#include "ns3/pagpsr-helper.h"
#include "ns3/mmgpsr-helper.h"

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
  //const uint16_t prior = 100;
  std::vector<std::pair<std::string, ns3::AttributeValue*> > attr_val_list;

  //Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  if(m_protocol == "AODV")
  {
    AodvHelper routing;
    internet.SetRoutingHelper (routing);
    internet.Install (c);

    attr_val_list.push_back(std::make_pair("EnableHello", new BooleanValue(false)));
  }
  else if(m_protocol == "OLSR")
  {
    OlsrHelper routing;
    internet.SetRoutingHelper (routing);
    internet.Install (c);

    attr_val_list.push_back(std::make_pair("HelloInterval", new TimeValue(Seconds (0.5))));
  }
  else if(m_protocol == "GPSR")
  {
    GpsrHelper routing;
    internet.SetRoutingHelper (routing);
    internet.Install (c);
    routing.Install(c);

    attr_val_list.push_back(std::make_pair("HelloInterval", new TimeValue(Seconds (1.0))));
  }
  else if(m_protocol == "PAGPSR")
  {
    PAGpsrHelper routing;
    internet.SetRoutingHelper (routing);
    internet.Install (c);
    routing.Install(c);

    attr_val_list.push_back(std::make_pair("HelloInterval", new TimeValue(Seconds (1.0))));
  }
  else if(m_protocol == "MMGPSR")
  {
    MMGpsrHelper routing;
    internet.SetRoutingHelper (routing);
    internet.Install (c);
    routing.Install(c);

    attr_val_list.push_back(std::make_pair("HelloInterval", new TimeValue(Seconds (1.0))));
  }
  else
  {

  }

  for(auto it = c.Begin(); it != c.End(); it++)
  {
    Ptr<Ipv4RoutingProtocol> routing = (*it)->GetObject<Ipv4RoutingProtocol>();
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

}

void
RoutingHelper::AssignIpAddresses (NetDeviceContainer & d,
                                  Ipv4InterfaceContainer & adhocTxInterfaces)
{
  NS_LOG_INFO ("Assigning IP addresses");
  uint16_t cntr = 1;
  for (auto it = d.Begin(); it != d.End(); it++)
  {
    std::string n = Names::FindName((*it)->GetNode());
    if(n.empty() == false)
    {
      std::size_t s = n.find_last_of('n');
      if(s != std::string::npos)
      {
        n = n.substr(s + 1);
      }
    }
    //std::string a = "10.0.1." + n;
    std::string a = "0.0.0." + n;

    Ipv4AddressHelper addressAdhoc;
    addressAdhoc.SetBase("10.0.1.0", "255.255.255.0", a.c_str());
    adhocTxInterfaces.Add(addressAdhoc.Assign(NetDeviceContainer(*it)));

    cntr++;
  }
}

StatsCollector &
RoutingHelper::GetStatsCollector ()
{
  return m_stats_collector;
}

void
RoutingHelper::ConfigureTracing(double start_time)
{
  AsciiTraceHelper ascii;

  m_ip_lev_tracer.SetDumpInterval(1.0, start_time);
  m_ip_lev_tracer.CreateOutput("ipv4.csv");

  //Through ip interaces
  for(auto it = m_ifs.Begin(); it != m_ifs.End(); it++)
  {
    Ptr<Ipv4> ip = it->first;
    std::string n_name = Names::FindName(ip->GetNetDevice(it->second)->GetNode());
    m_ip_lev_tracer.AddCollectingStatsFrom(ip, n_name);
  }

//  m_net_adj_tracer = CreateObject<NetworkAdjTracer>();
//  m_net_adj_tracer->CreateOutput("network-real.csv");
//  m_net_adj_tracer->SetNodeIfces(m_ifs);
//  m_net_adj_tracer->SetDumpInterval(1.0);
}


