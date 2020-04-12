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
#include "ns3/animation-interface.h"
#include "ns3/point-to-point-helper.h"

using namespace ns3;

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
  uint64_t m_total_send;
public:
  QueCalcer() : m_total_send(0){}
  void TraceTxStart(Ptr< const Packet > packet)
  {
    que.insert (std::pair<Ptr<const Packet>, double>(packet, Simulator::Now ().GetSeconds ()));
  }

  void TraceTxEnd(Ptr< const Packet > packet)
  {
    double now = Simulator::Now ().GetSeconds ();
    double diff = now - que[packet];
    m_total_send += packet->GetSize ();
    m_out << now << "\t" << diff << "\t" << m_total_send << std::endl;
  }
};

class MobTracer : public TracerBase
{
private:
  Ptr<MobilityModel> ap_mobility;
public:
  MobTracer() : ap_mobility(nullptr){}
  void SetApMobilityModel(Ptr<MobilityModel> mob)
  {
    ap_mobility = mob;
  }
  void CourseChangeCb(Ptr< const MobilityModel > model)
  {
    TracerBase::m_out << Simulator::Now ().GetSeconds () << "\t" << model->GetPosition ().x << "\t" << model->GetPosition ().y << "\t" << model->GetVelocity ().GetLength () << "\t" << model->GetDistanceFrom (ap_mobility) << "\t" << ap_mobility->GetPosition ().x << "\t" << ap_mobility->GetPosition ().y << std::endl;
  }
};

class PingTracer : public TracerBase
{
private:
public:
  PingTracer(){}
  void RttPingCb(ns3::Time rtt)
  {
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << "\t" << rtt.GetMilliSeconds () << std::endl;
  }
};


int main (int argc, char *argv[])
{
  ns3::CommandLine cmd;
  cmd.Parse (argc, argv);

  //All nodes
  ns3::Ptr<ns3::Node> rt = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> remote = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> sw_ap = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> win = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> lin = ns3::CreateObject<ns3::Node>();
  ns3::Ptr<ns3::Node> nas = ns3::CreateObject<ns3::Node>();
  //wifi
  ns3::Ptr<ns3::Node> andr = ns3::CreateObject<ns3::Node>();

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  //Wifi
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
  apDevices = wifi.Install (phy, mac, sw_ap);

  //Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (0, 50, 0, 50)));
  mobility.Install (andr);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (sw_ap);

  Ptr<MobilityModel> andr_mobility = andr->GetObject<MobilityModel>();
  Ptr<MobilityModel> ap_mobility = sw_ap->GetObject<MobilityModel>();

  //Csma
  ns3::CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", ns3::StringValue("10Mbps"));
  csma.SetChannelAttribute ("Delay", ns3::TimeValue(ns3::MicroSeconds (2000)));

  csma.Install (ns3::NodeContainer(win, sw_ap));
  csma.Install (ns3::NodeContainer(lin, sw_ap));
  csma.Install (ns3::NodeContainer(nas, sw_ap));
  csma.Install (ns3::NodeContainer(sw_ap, rt));

  //sw ports
  ns3::NetDeviceContainer sw_ports;
  for(auto i = 0u; i < sw_ap->GetNDevices (); i++)
  {
    auto dev = sw_ap->GetDevice (i);
    NS_ASSERT(!dev);
    sw_ports.Add (dev);
  }

  ns3::BridgeHelper sw_bridge;
  sw_bridge.Install (sw_ap, sw_ports);

  PointToPointHelper ppphelper;
  ppphelper.SetChannelAttribute ("Delay", TimeValue(MilliSeconds (10)));
  ppphelper.SetDeviceAttribute ("DataRate", ns3::StringValue("100Mbps"));
  NetDeviceContainer ppp_devs = ppphelper.Install (rt, remote);


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

  ns3::NetDeviceContainer ip_ext_net_devs;
  ip_ext_net_devs.Add (rt->GetDevice (1));
  ip_ext_net_devs.Add (remote->GetDevice (0));

  ip_nodes.Add (remote);

  //Ip
  ns3::InternetStackHelper ip_stack;
  ip_stack.Install (ip_nodes);

  ns3::Ipv4AddressHelper ip_helper;
  ip_helper.SetBase ("10.0.1.0", "255.255.255.0");
  ns3::Ipv4InterfaceContainer if_ip = ip_helper.Assign (ip_devs);

  ip_helper.SetBase ("20.0.1.0", "255.255.255.0");
  ns3::Ipv4InterfaceContainer if_ip_ext = ip_helper.Assign (ip_ext_net_devs);

  ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  ns3::Ipv4GlobalRoutingHelper g;
  for(auto n = ip_nodes.Begin (); n != ip_nodes.End (); n++)
  {
    Ptr<Node> node = *n;
    std::string f_name = node->GetTypeId ().GetName () + std::to_string (node->GetId ()) + ".routes";
    ns3::Ptr<ns3::OutputStreamWrapper> out_stream = ns3::Create<ns3::OutputStreamWrapper> (f_name, std::ios::out);
    g.PrintRoutingTableAt (ns3::Seconds (10.0), node, out_stream);
  }

  //Apps
  ns3::V4PingHelper ping_app(if_ip.GetAddress (0));
  ns3::ApplicationContainer ping_apps = ping_app.Install (NodeContainer(win, andr));
  ping_apps.Start (ns3::Seconds (0.0));

  ns3::InetSocketAddress sink_rcv(ns3::Ipv4Address::GetAny (), 3303);
  ns3::PacketSinkHelper sink("ns3::TcpSocketFactory", sink_rcv);
  ns3::ApplicationContainer lin_apps = sink.Install (lin);
  lin_apps.Start (ns3::Seconds (0.0));

  ns3::InetSocketAddress bulk_snd(if_ip.GetAddress (2), 3303);
  ns3::BulkSendHelper bulk("ns3::TcpSocketFactory", bulk_snd);
  bulk.SetAttribute ("MaxBytes", ns3::UintegerValue (40000000));
  ns3::ApplicationContainer nas_apps = bulk.Install (remote);
  nas_apps.Start (ns3::Seconds (0.0));

  //Traces
  PingTracer win_ping_tracer, andr_ping_tracer;
  win_ping_tracer.CreateOutput ("win_rtt.dat");
  andr_ping_tracer.CreateOutput ("andr_rtt.dat");
  ping_apps.Get (0)->TraceConnectWithoutContext ("Rtt", ns3::MakeCallback(&PingTracer::RttPingCb, &win_ping_tracer));
  ping_apps.Get (1)->TraceConnectWithoutContext ("Rtt", ns3::MakeCallback(&PingTracer::RttPingCb, &andr_ping_tracer));

  QueCalcer que_c;
  que_c.CreateOutput ("que_delay.dat");
  remote->GetDevice (0)->TraceConnectWithoutContext ("MacTx", ns3::MakeCallback (&QueCalcer::TraceTxStart, &que_c));
  remote->GetDevice (0)->TraceConnectWithoutContext ("PhyTxEnd", ns3::MakeCallback (&QueCalcer::TraceTxEnd, &que_c));

  PacketStatCollector cl;
  cl.CreateOutput ("lin_rx.dat");
  cl.CollectFrom (if_ip_ext.GetAddress (1).operator Address ());
  lin_apps.Get (0)->TraceConnectWithoutContext ("RxWithAddresses", ns3::MakeCallback (&PacketStatCollector::Trace, &cl));

  MobTracer mb;
  mb.CreateOutput ("traj.dat");
  mb.SetApMobilityModel (ap_mobility);
  andr_mobility->TraceConnectWithoutContext ("CourseChange", ns3::MakeCallback(&MobTracer::CourseChangeCb, &mb));

  ip_stack.EnablePcapIpv4 ("ip-devs-pcap", ip_nodes);
  ip_stack.EnableAsciiIpv4 ("andr_log", if_ip.Get (4).first, 0);
  ip_stack.EnableAsciiIpv4 ("andr_log", if_ip.Get (4).first, 1);
  csma.EnablePcap("sw_aps", sw_ports);

  phy.EnablePcap ("wifi_devs", NodeContainer(andr));

  AnimationInterface anim ("animation.xml");  // where "animation.xml" is any arbitrary filenames

  anim.SetConstantPosition (win, 10, 10);
  anim.SetConstantPosition (lin, 10, 15);
  anim.SetConstantPosition (nas, 15, 15);
  anim.SetConstantPosition (sw_ap, 20, 20);
  anim.SetConstantPosition (rt, 25, 30);
  anim.SetConstantPosition (remote, 80, 60);

  Simulator::Stop (Seconds (20.0));

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}

