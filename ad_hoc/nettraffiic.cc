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

#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NetTrafficCreator::NetTrafficCreator() : m_inst(nullptr)
{

}

NetTrafficCreator::~NetTrafficCreator()
{
  if(m_inst)
  {
    delete m_inst;
  }
}

int NetTrafficCreator::Create(NetTrafficClasses c, uint64_t stream_index, double total_sim_time)
{

  switch (c) {
    case NetTrafficCreator::NetTrafficClasses::PING_TRAFFIC:
      m_inst = new PingTraffic();
    break;

    case NetTrafficCreator::NetTrafficClasses::UDP_CBR:
      m_inst = new UdpCbrTraffic();
    break;

    default:
    break;
  }

  if(m_inst)
  {
    m_inst->m_total_time = total_sim_time;
    m_inst->m_index = stream_index;
  }

  return 0;
}

NetTraffic& NetTrafficCreator::Inst()
{
  NS_ASSERT(m_inst != nullptr);
  return *m_inst;
}

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

ExpResults& NetTraffic::GetResultsMap()
{
  return m_res;
}

//Ping
PingTraffic::PingTraffic()
{

}
PingTraffic::~PingTraffic()
{
  for(auto it : m_ping_trace)
  {
    delete it;
  }
}

int PingTraffic::Install(ns3::NodeContainer& nc, ns3::Ipv4InterfaceContainer& ip_c)
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
    PingTracer* tmp = new PingTracer();
    tmp->CreateOutput("ping-" + s + "-to-" + s2 + ".csv");
    a.Get(0)->TraceConnectWithoutContext("Rtt", MakeCallback(&PingTracer::RttPingCb, tmp));
    m_ping_trace.push_back(tmp);
  }
  return 0;
}

//================================

//UdpCbrTraffic
UdpCbrTraffic::UdpCbrTraffic() : m_interval(1.0), m_pckt_size(64)
{

}
UdpCbrTraffic::~UdpCbrTraffic()
{

}

int UdpCbrTraffic::Install(ns3::NodeContainer& nc, ns3::Ipv4InterfaceContainer& ip_c)
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

  m_pckt_tracer.CreateOutput("pdr-udp-cbr-traffic.csv");
  m_pckt_tracer.SetStatsCollector(&m_stats);
  m_pckt_tracer.SetDumpInterval(m_interval, GetTotalSimTime());
  m_pckt_tracer.Start();

  return 0;
}

ExpResults& UdpCbrTraffic::GetResultsMap()
{
  uint32_t cnter = m_pckt_tracer.GetConnectivityCnter();
  double conn = (cnter * m_interval) / GetTotalSimTime();
  m_res.insert(std::make_pair("RealConnectivity", std::to_string(conn)));
  return m_res;
}

void UdpCbrTraffic::RxCb(ns3::Ptr<ns3::Socket> socket)
{
  Ptr<Packet> packet;
  Address srcAddress;
  while ((packet = socket->RecvFrom (srcAddress)))
  {
    // application data, for goodput
    uint32_t RxBytes = packet->GetSize ();
    m_stats.IncRxBytes (RxBytes);
    m_stats.IncRxPkts ();
  }
}

void UdpCbrTraffic::GenerateTraffic(ns3::Ptr<ns3::Socket> socket)
{
  socket->Send (Create<Packet> (m_pckt_size));
  m_stats.IncTxPkts();
  m_stats.IncTxBytes(m_pckt_size);
  Simulator::Schedule (Seconds(m_interval), &UdpCbrTraffic::GenerateTraffic,
                       this, socket);
}

//================================
