#include <iostream>

#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/packet.h"
#include "ns3/string.h"
#include "ns3/simulator.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/seq-ts-header.h"
#include "ns3/wave-net-device.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/wave-helper.h"

#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/aodv-helper.h"

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

using namespace std;
using namespace ns3;


struct node_id
{
  ns3::Ptr<ns3::Node> n;
  ns3::Ptr<ns3::NetDevice> d;
  std::pair<Ptr<Ipv4>, uint32_t> l3;
};

void ReceivePacket (ns3::Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
      Simulator::Stop(pktInterval);
    }
}

int main()
{
  //LogComponentEnableAll (LogLevel::LOG_ALL);
  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponent::ComponentList *components = LogComponent::GetComponentList ();
  std::string pattern = "Ipv4";
  for(auto it = components->begin(); it != components->end(); it++)
  {
    if(it->first.find(pattern) != std::string::npos)
    {
      LogComponentEnable(it->first.c_str(), LogLevel::LOG_ALL);
    }
  }

//  LogComponentEnable("Ipv4StaticRouting", LogLevel::LOG_ALL);
//  LogComponentEnable("Ipv4", LogLevel::LOG_ALL);

  ns3::NodeContainer nodes;
  nodes.Create(2);

  ns3::YansWavePhyHelper wifi_phy = ns3::YansWavePhyHelper::Default();
  ns3::YansWifiChannelHelper wifi_ch = ns3::YansWifiChannelHelper::Default();
  wifi_phy.SetChannel(wifi_ch.Create());
  wifi_phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  ns3::NqosWaveMacHelper wifi_mac = ns3::NqosWaveMacHelper::Default();
  ns3::Wifi80211pHelper wifi_helper = ns3::Wifi80211pHelper::Default();

  wifi_helper.EnableLogComponents();

  wifi_helper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue ("OfdmRate6MbpsBW10MHz"),
                                      "ControlMode", StringValue ("OfdmRate6MbpsBW10MHz"));

  ns3::NetDeviceContainer infs = wifi_helper.Install(wifi_phy, wifi_mac, nodes);

  wifi_phy.EnablePcapAll("80211p-");
  wifi_phy.EnableAsciiAll("80211p-tr");

  //Mobility
  ns3::MobilityHelper mobility;
  ns3::Ptr<ns3::ListPositionAllocator> positionAlloc = ns3::CreateObject<ns3::ListPositionAllocator> ();
  positionAlloc->Add (ns3::Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (ns3::Vector (100.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);


  ns3::InternetStackHelper internet;
  ns3::AodvHelper routing;
  internet.SetRoutingHelper(routing);
  internet.EnablePcapIpv4All("ip-");
  internet.SetIpv6StackInstall(false);
  internet.Install(nodes);

  uint8_t cnter = 1;
  for(auto it = nodes.Begin(); it != nodes.End(); it++)
  {
    char buf[30];
    ns3::Ptr<ns3::Node> n = *it;
    sprintf(buf, "00:00:00:00:00:%0*x", 2, cnter);
    ns3::Mac48Address addr(buf);
    auto wdev = n->GetDevice(0);
    wdev->SetAddress(addr);
    sprintf(buf, "%x", cnter);
    ns3::Names::Add("node", std::string(buf), n);

    cnter++;
  }

  ns3::Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.0.0.0", "255.255.255.0");
  ns3::Ipv4InterfaceContainer ifs = ipv4.Assign(infs);

  node_id snder {nodes.Get(0), infs.Get(0), ifs.Get(0)};
  node_id rcver {nodes.Get(1), infs.Get(1), ifs.Get(1)};

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (rcver.n, tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (snder.n, tid);

  Ipv4Address addr;
  auto a = rcver.l3.first->GetAddress(rcver.l3.second, 0);
  addr = a.GetLocal();
  InetSocketAddress remote = InetSocketAddress (addr, 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  ns3::AsciiTraceHelper tr;
  ns3::Ptr<ns3::OutputStreamWrapper> wr = tr.CreateFileStream(Names::FindName(snder.n) + "-node.rt");
  routing.PrintRoutingTableAt(MilliSeconds(1), snder.n, wr);

  const ns3::Time period = ns3::MilliSeconds(1000);
  const uint32_t pckts = 100;
  const uint32_t pckt_size = 700;

  Simulator::ScheduleWithContext (snder.n->GetId (),
                                  Seconds (1.0), &GenerateTraffic,
                                  source, pckt_size, pckts, period);


  ns3::Simulator::Run();

  ns3::Simulator::Destroy();
  return 0;
}


class FANETL3Level;


class FL3Header : public ns3::Header
{
public:
  ns3::Address m_addr;
  uint8_t m_ttl;

public:

  FL3Header (){}
  virtual ~FL3Header (){}

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("FL3Header")
      .SetParent<ns3::Header> ()
      .AddConstructor<FL3Header> ()
    ;
    return tid;
  }
  virtual ns3::TypeId GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }
  virtual void Print (std::ostream &os) const
  {
    // This method is invoked by the packet printing
    // routines to print the content of my header.
    //os << "data=" << m_data << std::endl;
    char* buf = new char [m_addr.GetLength() + 1];
    buf[m_addr.GetLength()] = '\0';
    os << "addr=" << std::string(buf) <<"\nttl=" << m_ttl;
    delete[] buf;
  }
  virtual void Serialize (ns3::Buffer::Iterator start) const
  {
    // we can serialize two bytes at the start of the buffer.
    // we write them in network byte order.
    uint8_t* buf = new uint8_t[m_addr.GetSerializedSize()];
    ns3::TagBuffer tb(buf, buf + m_addr.GetSerializedSize());
    m_addr.Serialize(tb);
    start.Write(buf, m_addr.GetSerializedSize());
    start.WriteU8 (m_ttl);
    delete[] buf;
  }
  virtual uint32_t Deserialize (ns3::Buffer::Iterator start)
  {
    uint8_t* buf = new uint8_t[m_addr.GetSerializedSize()];
    start.Read(buf, m_addr.GetSerializedSize());
    ns3::TagBuffer tb(buf, buf + m_addr.GetSerializedSize());
    m_addr.Deserialize(tb);
    m_ttl = start.ReadU8();
    delete[] buf;
  }
  virtual uint32_t GetSerializedSize (void) const
  {
    return m_addr.GetSerializedSize() + 1;
  }
};

class FANETL3Level : public ns3::Object
{
private:
  ns3::Mac48Address m_addr;
  ns3::Ptr<ns3::Node> m_node;
  ns3::Ptr<ns3::NetDevice> m_dev;
public:

  static const uint16_t FL3ID = 152;

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("FANETL3Level")
      .SetParent<Object> ()
      .SetGroupName ("L3")
      .AddConstructor<FANETL3Level> ();
    return tid;
  }

  FANETL3Level () : m_node(nullptr), m_dev(nullptr)
  {

  }
  virtual ~FANETL3Level (){}

  static ns3::Ptr<FANETL3Level> Install(ns3::Ptr<ns3::Node> n)
  {
    if(n->GetNDevices() == 0)
    {
      return nullptr;
    }

    ns3::ObjectFactory f;
    f.SetTypeId(GetTypeId());
    ns3::Ptr<FANETL3Level> lev = f.Create<FANETL3Level>();
    n->AggregateObject(lev);

    lev->m_node = n;
    lev->m_dev = n->GetDevice(0);
    lev->m_addr = ns3::Mac48Address::ConvertFrom(lev->m_dev->GetAddress());

    n->RegisterProtocolHandler(ns3::MakeCallback(&FANETL3Level::ReceiveFromL2, lev),
                               FANETL3Level::FL3ID,
                               lev->m_dev,
                               true);

    return lev;
  }

  ns3::Ptr<ns3::Node> GetNode() const
  {
    return m_node;
  }

  void ReceiveFromL2(ns3::Ptr<ns3::NetDevice> dev,
                            ns3::Ptr<const ns3::Packet> p,
                            uint16_t prot,
                            const ns3::Address & from,
                            const ns3::Address & to,
                            ns3::NetDevice::PacketType p_type)
  {
    NS_LOG_DEBUG ("NodeProtocolHandler");

    FL3Header hdr;
    p->PeekHeader(hdr);

    switch (p_type) {
      case ns3::NetDevice::PacketType::PACKET_HOST:
      break;
      case ns3::NetDevice::PacketType::PACKET_BROADCAST:
      break;
      case ns3::NetDevice::PacketType::PACKET_MULTICAST:
      break;
      case ns3::NetDevice::PacketType::PACKET_OTHERHOST:
      break;
    }
  }

  int8_t SendPacket(ns3::Ptr<ns3::Packet> p, ns3::Mac48Address to)
  {
    FL3Header hdr;
    hdr.m_ttl = 4;
    hdr.m_addr = to;
    ns3::Ptr<ns3::Packet> pckt = p->Copy();
    pckt->AddHeader(hdr);
    return m_dev->Send(pckt, to, FL3ID);
  }



};

static void test_send_data(ns3::Ptr<FANETL3Level> l3, ns3::Mac48Address to)
{
  const uint32_t data_size = 100;
  const uint32_t pckt_size = 700;
  const ns3::Time period = ns3::MilliSeconds(3);
  static uint32_t cnter = data_size;
  ns3::Ptr<ns3::Packet> p = ns3::Create<ns3::Packet>(pckt_size);
  l3->SendPacket(p, to);
  cnter--;
  if(cnter)
  {
    ns3::Simulator::ScheduleWithContext(l3->GetNode()->GetId(),
                                        period,
                                        test_send_data,
                                        l3,
                                        to);
  }
  else
  {
    ns3::Simulator::Stop(period);
  }
}
