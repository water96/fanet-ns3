#ifndef ADHOC_H
#define ADHOC_H

#include <stdint.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "utils/tracers.h"

class RoutingHelper : public ns3::Object
{
public:
  static ns3::TypeId GetTypeId (void);

  static const double TOTAL_SIM_TIME;
  static const uint16_t PORT;

  RoutingHelper ();
  virtual ~RoutingHelper ();
  void Install (ns3::NodeContainer & c,
                ns3::NetDeviceContainer & d,
                ns3::Ipv4InterfaceContainer & i,
                double totalTime,
                std::string protocol);

  StatsCollector & GetStatsCollector ();
  void ConfigureTracing (double start_time);

private:
  void SetupRoutingProtocol (ns3::NodeContainer & c);
  void AssignIpAddresses (ns3::NetDeviceContainer & d,
                          ns3::Ipv4InterfaceContainer & adhocTxInterfaces);

  std::vector<ns3::Ptr<Ipv4L3ProtocolTracer> > m_ipv4_tracers;
  IPv4AllStatsTracer m_ip_lev_tracer;
  ns3::Ptr<NetworkAdjTracer> m_net_adj_tracer;

  ns3::NodeContainer m_nodes;
  ns3::NetDeviceContainer m_devs;
  ns3::Ipv4InterfaceContainer m_ifs;

  double m_TotalSimTime;        ///< seconds
  std::string m_protocol;       ///< routing protocol; 0=NONE, 1=OLSR, 2=AODV, 3=DSDV, 4=DSR
  StatsCollector m_stats_collector; ///< routing statistics
};



#endif // ADHOC_H
