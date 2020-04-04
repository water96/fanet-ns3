#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/csma-module.h"
#include "ns3/csma-star-helper.h"
#include "ns3/bridge-module.h"
#include "ns3/wifi-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/v4ping.h"
#include "ns3/v4ping-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/rectangle.h"

using namespace ns3;

void rtt_ping_cb(ns3::Ptr<ns3::OutputStreamWrapper> stream, ns3::Time rtt)
{
  *(stream->GetStream ()) << ns3::Simulator::Now ().GetSeconds () << "\t" << rtt.GetMilliSeconds () << std::endl;
}

void bulk_tx_cb(ns3::Ptr<ns3::OutputStreamWrapper> stream, Ptr< const Packet > packet)
{
  *(stream->GetStream ()) << ns3::Simulator::Now ().GetSeconds () << "\t" << packet->GetSize () << std::endl;
}

class TracerBase
{
protected:
  std::ofstream m_out;
public:
  TracerBase(){}
  ~TracerBase()
  {
    if(m_out.is_open ())
      m_out.close ();
  }

  void CreateOutput(const std::string& name)
  {
    m_out.open (name);
  }
};

class PacketStatCollector
{
private:
  std::ofstream m_out;
  ns3::Address m_addr_to_collect;
  uint32_t m_ip_addr;
  uint64_t m_total;
public:
  PacketStatCollector(){}
  ~PacketStatCollector()
  {
    if(m_out.is_open ())
      m_out.close ();
  }

  void CreateOutput(const std::string& name)
  {
    m_out.open (name);
  }

  void CollectFrom(const ns3::Address& addr)
  {
    addr.CopyTo (reinterpret_cast<uint8_t*>(&m_ip_addr));
  }

  void Trace(const Ptr< const Packet > packet, const Address &srcAddress, const Address &destAddress)
  {
    uint8_t* buf = new uint8_t[srcAddress.GetLength ()];
    srcAddress.CopyTo (buf);
    if(std::memcmp (&m_ip_addr, buf, 4) == 0)
      m_out << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize () << std::endl;
  }
};

class QueCalcer : public TracerBase
{
private:
  std::map<Ptr<const Packet>, double > que;

public:
  void TraceTxStart(Ptr< const Packet > packet)
  {
    que.insert (std::pair<Ptr<const Packet>, double>(packet, Simulator::Now ().GetSeconds ()));
    //m_out << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize () << std::endl;
  }

  void TraceTxEnd(Ptr< const Packet > packet)
  {
    double now = Simulator::Now ().GetSeconds ();
    double diff = now - que[packet];
    m_out << now << "\t" << diff << std::endl;
  }
};

int main (int argc, char *argv[])
{
  ns3::CommandLine cmd;
  cmd.Parse (argc, argv);

  ns3::Ptr<ns3::Node> rt = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> sw = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> win = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> lin = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> nas = ns3::CreateObject<ns3::Node>();
  //wifi
  ns3::Ptr<ns3::Node> ap = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> andr = ns3::CreateObject<ns3::Node>();

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("home-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer wifi_devs;
  wifi_devs = wifi.Install (phy, mac, andr);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, ap);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (andr);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ap);


  ns3::CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", ns3::StringValue("10Mbps"));
  csma.SetChannelAttribute ("Delay", ns3::TimeValue(ns3::MicroSeconds (2000)));

  csma.Install (ns3::NodeContainer(win, sw));
  csma.Install (ns3::NodeContainer(lin, sw));
  csma.Install (ns3::NodeContainer(nas, sw));
  csma.Install (ns3::NodeContainer(ap, sw));
  csma.Install (ns3::NodeContainer(sw, rt));

  //sw ports
  ns3::NetDeviceContainer sw_ports;
  for(auto i = 0u; i < sw->GetNDevices (); i++)
  {
    auto dev = sw->GetDevice (i);
    NS_ASSERT(!dev);
    sw_ports.Add (dev);
  }

  ns3::BridgeHelper sw_bridge;
  sw_bridge.Install (sw, sw_ports);

  ns3::NetDeviceContainer ip_devs;
  ip_devs.Add (rt->GetDevice (0));
  ip_devs.Add (win->GetDevice (0));
  ip_devs.Add (lin->GetDevice (0));
  ip_devs.Add (nas->GetDevice (0));
  ip_devs.Add (andr->GetDevice (0));

  ns3::NodeContainer ip_nodes;
  for(auto d = ip_devs.Begin (); d != ip_devs.End (); d++)
  {
    ip_nodes.Add ((*d)->GetNode ());
  }

  ns3::InternetStackHelper ip_stack;
  ip_stack.Install (ip_nodes);

  ns3::Ipv4AddressHelper ip_helper;
  ip_helper.SetBase ("192.168.1.0", "255.255.255.0");
  ns3::Ipv4InterfaceContainer if_ip = ip_helper.Assign (ip_devs);

  ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  ns3::Ptr<ns3::OutputStreamWrapper> win_routingStream =
    ns3::Create<ns3::OutputStreamWrapper> ("win.routes", std::ios::out);

  ns3::Ptr<ns3::OutputStreamWrapper> rt_routingStream =
    ns3::Create<ns3::OutputStreamWrapper> ("rt.routes", std::ios::out);

  ns3::Ipv4GlobalRoutingHelper g;

  g.PrintRoutingTableAt (ns3::Seconds (10.0), win, win_routingStream);
  g.PrintRoutingTableAt (ns3::Seconds (10.0), rt, rt_routingStream);

  ns3::V4PingHelper ping_app(if_ip.GetAddress (0));
  ns3::ApplicationContainer ping_apps = ping_app.Install (NodeContainer(win, andr));
  ping_apps.Start (ns3::Seconds (0.0));
  ping_apps.Stop (ns3::Seconds (20.0));

  ns3::InetSocketAddress sink_rcv(ns3::Ipv4Address::GetAny (), 3303);
  ns3::PacketSinkHelper sink("ns3::TcpSocketFactory", sink_rcv);
  ns3::ApplicationContainer lin_apps = sink.Install (lin);
  lin_apps.Start (ns3::Seconds (0.0));


  ns3::InetSocketAddress bulk_snd(if_ip.GetAddress (2), 3303);
  ns3::BulkSendHelper bulk("ns3::TcpSocketFactory", bulk_snd);
  bulk.SetAttribute ("MaxBytes", ns3::UintegerValue (40000000));
  ns3::ApplicationContainer nas_apps = bulk.Install (nas);
  nas_apps.Start (ns3::Seconds (0.0));

  ns3::AsciiTraceHelper asciiTraceHelper;
  ns3::Ptr<ns3::OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("rtt.dat");
  ping_apps.Get (0)->TraceConnectWithoutContext ("Rtt", ns3::MakeBoundCallback(&rtt_ping_cb, stream));

  ns3::Ptr<ns3::OutputStreamWrapper> tx_stream = asciiTraceHelper.CreateFileStream ("bulk_tx.dat");
  nas_apps.Get (0)->TraceConnectWithoutContext ("Tx", ns3::MakeBoundCallback(&bulk_tx_cb, tx_stream));

  QueCalcer que_c;
  que_c.CreateOutput ("que_delay.dat");
  nas->GetDevice (0)->TraceConnectWithoutContext ("MacTx", ns3::MakeCallback (&QueCalcer::TraceTxStart, &que_c));
  nas->GetDevice (0)->TraceConnectWithoutContext ("PhyTxEnd", ns3::MakeCallback (&QueCalcer::TraceTxEnd, &que_c));

  PacketStatCollector cl;
  cl.CreateOutput ("lin_rx.dat");
  cl.CollectFrom (if_ip.GetAddress (3).operator Address ());
  lin_apps.Get (0)->TraceConnectWithoutContext ("RxWithAddresses", ns3::MakeCallback (&PacketStatCollector::Trace, &cl));

  csma.EnablePcap ("ip-devs-pcap", ip_devs);

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}
