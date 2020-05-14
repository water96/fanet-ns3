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
    m_protocol (ROUTING_PROTOCOL::NONE),
    m_port (PORT),
    m_nSinks (0),
    m_routingTables (1),
    m_log (0)
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
                        ROUTING_PROTOCOL protocol,
                        uint32_t nSinks,
                        int routingTables)
{
  m_TotalSimTime = totalTime;
  m_protocol = protocol;
  m_nSinks = nSinks;
  m_routingTables = routingTables;

  SetupRoutingProtocol (c);
  AssignIpAddresses (d, i);
  SetupRoutingMessages (c, i);
}

Ptr<Socket>
RoutingHelper::SetupRoutingPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, m_port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingHelper::ReceiveRoutingPacket, this));

  return sink;
}

void
RoutingHelper::SetupRoutingProtocol (NodeContainer & c)
{
  Ipv4RoutingHelper* routing = nullptr;
  Ptr<Ipv4RoutingProtocol> prot;
  const uint16_t prior = 100;

  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  Time rtt = Time (5.0);
  AsciiTraceHelper ascii;

  switch (m_protocol)
    {
      case ROUTING_PROTOCOL::NONE:
        m_protocolName = "NONE";
      break;
      case ROUTING_PROTOCOL::OLSR:
        routing = new OlsrHelper;
        m_protocolName = "OLSR";
      break;
      case ROUTING_PROTOCOL::AODV:
        routing = new AodvHelper;
        //prot = routing->GetRouting<ns3::aodv::RoutingProtocol>(prot);
        //prot->SetAttribute("EnableHello", BooleanValue(false));
        m_protocolName = "AODV";
      break;
      case ROUTING_PROTOCOL::DSDV:
        routing = new DsdvHelper;
        m_protocolName = "DSDV";
      break;
      case ROUTING_PROTOCOL::DSR:
        DsrHelper dsr;
        DsrMainHelper dsrMain;
        dsrMain.Install (dsr, c);
        m_protocolName = "DSR";
      break;
    }

  if(routing)
  {
    if (m_routingTables != 0)
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

    }
    list.Add(*routing, prior);
    internet.SetRoutingHelper (list);
    delete routing;
  }

  internet.Install (c);

  internet.EnablePcapIpv4All("ip-pcap");
  internet.EnableAsciiIpv4All("ip-ascii");

  if (m_log != 0)
    {
      NS_LOG_UNCOND ("Routing Setup for " << m_protocolName);
    }
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
      std::size_t s = n.find_last_of('-');
      if(s != std::string::npos)
      {
        n = n.substr(s + 1, n.size() - s);
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

void
RoutingHelper::SetupRoutingMessages (NodeContainer & c,
                                     Ipv4InterfaceContainer & adhocTxInterfaces)
{

}

static inline std::string
PrintReceivedRoutingPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address srcAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (srcAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (srcAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
RoutingHelper::ReceiveRoutingPacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address srcAddress;
  while ((packet = socket->RecvFrom (srcAddress)))
    {
      // application data, for goodput
      uint32_t RxRoutingBytes = packet->GetSize ();
      GetStatsCollector ().IncRxBytes (RxRoutingBytes);
      GetStatsCollector ().IncRxPkts ();
      if (m_log != 0)
        {
          NS_LOG_UNCOND (m_protocolName + " " + PrintReceivedRoutingPacket (socket, packet, srcAddress));
        }
    }
}

void
RoutingHelper::OnOffTrace (std::string context, Ptr<const Packet> packet)
{
  uint32_t pktBytes = packet->GetSize ();
  m_stats_collector.IncTxBytes (pktBytes);
}

StatsCollector &
RoutingHelper::GetStatsCollector ()
{
  return m_stats_collector;
}

void
RoutingHelper::SetLogging (int log)
{
  m_log = log;
}


