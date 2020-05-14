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

  enum class ROUTING_PROTOCOL
  {
    NONE = 0u,
    OLSR,
    AODV,
    DSDV,
    DSR
  };


  RoutingHelper ();
  virtual ~RoutingHelper ();
  void Install (ns3::NodeContainer & c,
                ns3::NetDeviceContainer & d,
                ns3::Ipv4InterfaceContainer & i,
                double totalTime,
                ROUTING_PROTOCOL protocol,
                uint32_t nSinks,
                int routingTables);
  void OnOffTrace (std::string context, ns3::Ptr<const ns3::Packet> packet);
  StatsCollector & GetStatsCollector ();
  void SetLogging (int log);

private:
  void SetupRoutingProtocol (ns3::NodeContainer & c);
  void AssignIpAddresses (ns3::NetDeviceContainer & d,
                          ns3::Ipv4InterfaceContainer & adhocTxInterfaces);
  void SetupRoutingMessages (ns3::NodeContainer & c,
                             ns3::Ipv4InterfaceContainer & adhocTxInterfaces);
  ns3::Ptr<ns3::Socket> SetupRoutingPacketReceive (ns3::Ipv4Address addr, ns3::Ptr<ns3::Node> node);
  void ReceiveRoutingPacket (ns3::Ptr<ns3::Socket> socket);

  double m_TotalSimTime;        ///< seconds
  ROUTING_PROTOCOL m_protocol;       ///< routing protocol; 0=NONE, 1=OLSR, 2=AODV, 3=DSDV, 4=DSR
  uint32_t m_port;           ///< port
  uint32_t m_nSinks;              ///< number of sink nodes (< all nodes)
  int m_routingTables;      ///< dump routing table (at t=5 sec).  0=No, 1=Yes
  StatsCollector m_stats_collector; ///< routing statistics
  std::string m_protocolName; ///< protocol name
  int m_log; ///< log
};



#endif // ADHOC_H
