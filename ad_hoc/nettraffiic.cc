#include "nettraffiic.h"

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
#include "ns3/arp-l3-protocol.h"
#include "ns3/arp-header.h"
#include "ns3/arp-queue-disc-item.h"

#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NetTrafficCreator& NetTrafficCreator::Inst()
{
  static NetTrafficCreator inst;
  return inst;
}

NetTrafficCreator::NetTrafficCreator() : m_inst(nullptr)
{
  m_models.insert(std::make_pair("UDP_CBR", new UdpCbrTraffic));
  m_models.insert(std::make_pair("Ping", new PingTraffic));
  m_models.insert(std::make_pair("L3ND", new L3NodesDiscoverTraffic));

  m_default = m_models.begin()->first;
}

NetTrafficCreator::~NetTrafficCreator()
{
  for(auto& it : m_models)
  {
    delete it.second;
  }

  DestroyNetTrafficModel();
}

std::string NetTrafficCreator::GetDefaultModel()
{
  return m_default;
}

std::string NetTrafficCreator::GetModelsList()
{
  std::string ret;
  for(auto& it : m_models)
  {
    ret += it.first + ",";
  }

  return ret.substr(0, ret.length() - 1);
}

int NetTrafficCreator::CreateNetTrafficModel(const std::string& model, uint64_t stream_index, double total_sim_time)
{
  if(m_inst)
  {
    return 1;
  }

  auto find_it = m_models.find(model);
  if(find_it != m_models.end())
  {
    m_inst = find_it->second->Clone();
    m_inst->m_total_time = total_sim_time;
    m_inst->m_index = stream_index;
    return 0;
  }

  return 1;
}

NetTraffic& NetTrafficCreator::GetNetTrafficModel()
{
  NS_ASSERT(m_inst != nullptr);
  return *m_inst;
}

void NetTrafficCreator::DestroyNetTrafficModel()
{
  if(m_inst)
  {
    delete m_inst;
    m_inst = nullptr;
  }
}

//==================================================

NetTraffic::NetTraffic()
{

}

NetTraffic::~NetTraffic()
{

}
uint64_t NetTraffic::GetStreamIndex() const
{
  return m_index;
}

double NetTraffic::GetTotalSimTime() const
{
  return m_total_time;
}

//Ping
PingTraffic::PingTraffic()
{

}

NetTraffic* PingTraffic::Clone() const
{
  return new PingTraffic();
}

PingTraffic::~PingTraffic(){}

int PingTraffic::ConfigreTracing()
{
  return 1;
}

int PingTraffic::Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c)
{
  if(nc.GetN() != ip_c.GetN())
  {
    return -1;
  }

  Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
  var->SetStream(NetTraffic::GetStreamIndex());

  auto src = ip_c.Get(0);
  auto node = src.first->GetNetDevice(src.second)->GetNode();
  std::string s = Names::FindName(node);

  for(uint32_t i = 1; i < ip_c.GetN(); i++)
  {
    V4PingHelper hlp(ip_c.GetAddress(i));
    hlp.SetAttribute("Verbose", BooleanValue(true));
    hlp.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    ApplicationContainer a = hlp.Install(node);
    a.Start(Seconds(var->GetValue(GetTotalSimTime()*0.1, GetTotalSimTime()*0.2)));
    a.Stop(Seconds(GetTotalSimTime()));

    auto tmp_node = nc.Get(i);
    std::string s2 = Names::FindName(tmp_node);
    Ptr<PingTracer> tmp = CreateObject<PingTracer>();
    tmp->CreateOutput("ping-" + s + "-to-" + s2 + ".csv");
    a.Get(0)->TraceConnectWithoutContext("Rtt", MakeCallback(&PingTracer::RttPingCb, tmp));
    m_ping_trace.push_back(tmp);
  }
  return 0;
}

//================================

//UdpCbrTraffic
UdpCbrTraffic::UdpCbrTraffic() : m_interval(1.0), m_pckt_size(64), m_pckt_tracer(nullptr)
{

}
UdpCbrTraffic::~UdpCbrTraffic()
{

}

NetTraffic* UdpCbrTraffic::Clone() const
{
  return new UdpCbrTraffic();
}

int UdpCbrTraffic::Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c)
{
  if(nc.GetN() != ip_c.GetN())
  {
    return -1;
  }

  Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
  var->SetStream(NetTraffic::GetStreamIndex());

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Node> src_node = nc.Get (0);
  std::string src_name_node = Names::FindName(src_node);
  Ipv4Address src_addr = ip_c.GetAddress(0);

  for(auto it = ip_c.Begin() + 1; it != ip_c.End(); it++)
  {
    Ptr<Node> n = it->first->GetNetDevice(it->second)->GetNode();
    std::string n_name = Names::FindName(n);
    Ipv4Address rem = it->first->GetAddress(it->second, 0).GetLocal();

    //Setup rx
    Ptr<Socket> recvSink = Socket::CreateSocket (n, tid);
    InetSocketAddress local = InetSocketAddress (rem, 9);
    recvSink->Bind (local);
    recvSink->SetRecvCallback(MakeCallback(&UdpCbrTraffic::RxCb, this));

    //Setup send from node 0
    InetSocketAddress remote = InetSocketAddress (rem, 9);
    Ptr<Socket> source_socket = Socket::CreateSocket (src_node, tid);
    source_socket->Connect(remote);

    Simulator::ScheduleWithContext (src_node->GetId (),
                                    Seconds (var->GetValue(0.1, 0.3)), &UdpCbrTraffic::GenerateTraffic,
                                    this, source_socket);
  }

  return 0;
}

int UdpCbrTraffic::ConfigreTracing()
{
  //Create trace object
  m_pckt_tracer = CreateObject<PDRAndThroughputMetr>();
  //

  m_pckt_tracer->CreateOutput("pdr-udp-cbr-traffic.csv");
  m_pckt_tracer->SetDumpInterval(m_interval, GetTotalSimTime());
  m_pckt_tracer->Start();
  return 0;
}

void UdpCbrTraffic::RxCb(ns3::Ptr<ns3::Socket> socket)
{
  Ptr<Packet> packet;
  Address srcAddress;
  while ((packet = socket->RecvFrom (srcAddress)))
  {
    // application data, for goodput
    this->m_pckt_tracer->RxCb(packet->Copy(), socket);
  }
}

void UdpCbrTraffic::GenerateTraffic(ns3::Ptr<ns3::Socket> socket)
{
  Ptr<Packet> p = Create<Packet>(m_pckt_size);
  socket->Send (p);
  this->m_pckt_tracer->TxCb(p->Copy(), socket);
  Simulator::Schedule (Seconds(m_interval), &UdpCbrTraffic::GenerateTraffic,
                       this, socket);
}

//================================

const uint16_t L3NodesDiscoverTraffic::PROT_NUMBER = 0x0f0f;

//L3NodesDiscoverTraffic
L3NodesDiscoverTraffic::L3NodesDiscoverTraffic() : m_interval(1.0), m_pckt_size(64)
{

}
L3NodesDiscoverTraffic::~L3NodesDiscoverTraffic()
{

}

NetTraffic* L3NodesDiscoverTraffic::Clone() const
{
  return new L3NodesDiscoverTraffic();
}

int L3NodesDiscoverTraffic::Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c)
{
  if(nc.GetN() != ip_c.GetN() || (devs.GetN() != nc.GetN()))
  {
    return -1;
  }
  m_devs = devs;
  //Random variable for start discovering
  Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
  var->SetStream(NetTraffic::GetStreamIndex());


  for(auto ip_it = ip_c.Begin(); ip_it != ip_c.End(); ip_it++)
  {
    ns3::Ptr<NetDevice> dev = ip_it->first->GetNetDevice(ip_it->second);
    Ptr<Node> n = dev->GetNode();
    Ptr<TrafficControlLayer> tc = n->GetObject<TrafficControlLayer> ();
    n->RegisterProtocolHandler(MakeCallback (&TrafficControlLayer::Receive, tc),
                               PROT_NUMBER, dev);
    tc->RegisterProtocolHandler(MakeCallback(&L3NodesDiscoverTraffic::ReceiveCb, this),
                                PROT_NUMBER, dev);
    ns3::Simulator::Schedule(ns3::Seconds(var->GetValue(0.1, 0.4)), &L3NodesDiscoverTraffic::TransmitCb, this, dev, tc, ip_it->first->GetAddress(ip_it->second, 0));
  }

  return 0;
}

void L3NodesDiscoverTraffic::TransmitCb(ns3::Ptr<ns3::NetDevice> dev, ns3::Ptr<ns3::TrafficControlLayer> tc, ns3::Ipv4InterfaceAddress ip_addr)
{
  ArpHeader hdr;
  hdr.SetRequest (dev->GetAddress (), ip_addr.GetLocal(), dev->GetBroadcast (), ip_addr.GetBroadcast());
  ns3::Ptr<Packet> p = Create<Packet>(m_pckt_size);
  Ptr<ArpQueueDiscItem> item = Create<ArpQueueDiscItem>(p, dev->GetBroadcast (), PROT_NUMBER, hdr);
  tc->Send(dev, item);

  ns3::Simulator::Schedule(ns3::Seconds(m_interval), &L3NodesDiscoverTraffic::TransmitCb, this, dev, tc, ip_addr);
}

void L3NodesDiscoverTraffic::ReceiveCb(ns3::Ptr<ns3::NetDevice> device, ns3::Ptr<const ns3::Packet> p, uint16_t protocol, const ns3::Address &from,
               const ns3::Address &to, ns3::NetDevice::PacketType packetType )
{
  if(m_adj_tracer)
  {
    m_adj_tracer->RxCb(device, from, p->Copy());
  }
}

int L3NodesDiscoverTraffic::ConfigreTracing()
{
  m_adj_tracer = CreateObject<AdjTracer>();
  m_adj_tracer->CreateOutput("potential.csv");
  m_adj_tracer->SetNodeDevices(m_devs);
  m_adj_tracer->SetDumpInterval(1.0);
  return 0;
}
