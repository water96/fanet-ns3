#ifndef TRACERS_H
#define TRACERS_H

#include <iostream>
#include <fstream>
#include <string>

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
#include "ns3/wifi-module.h"
#include "ns3/ip-l4-protocol.h"


typedef std::map<std::string, std::string> ExpResults;

class StatsCollector
{
public:
  StatsCollector ();

  uint32_t GetRxBytes ();
  uint32_t GetCumulativeRxBytes ();
  uint32_t GetRxPkts ();
  uint32_t GetCumulativeRxPkts ();

  void IncRxBytes (uint32_t rxBytes);
  void IncRxPkts ();
  void SetRxBytes (uint32_t rxBytes);
  void SetRxPkts (uint32_t rxPkts);

  uint32_t GetTxBytes ();
  uint32_t GetCumulativeTxBytes ();
  uint32_t GetTxPkts ();
  uint32_t GetCumulativeTxPkts ();

  void IncTxBytes (uint32_t txBytes);
  void IncTxPkts ();
  void SetTxBytes (uint32_t txBytes);
  void SetTxPkts (uint32_t txPkts);

private:
  uint32_t m_RxBytes; ///< reeive bytes
  uint32_t m_cumulativeRxBytes; ///< cumulative receive bytes
  uint32_t m_RxPkts; ///< receive packets
  uint32_t m_cumulativeRxPkts; ///< cumulative receive packets
  uint32_t m_TxBytes; ///< transmit bytes
  uint32_t m_cumulativeTxBytes; ///< cumulative transmit bytes
  uint32_t m_TxPkts; ///< transmit packets
  uint32_t m_cumulativeTxPkts; ///< cumulative transmit packets
};

//TODO: add for all tracers static GetTypeId method and change its usage
class TracerBase : public ns3::Object
{
protected:
  //std::ofstream m_out;
  std::map<std::string, std::ofstream> m_cb_out_map;
  std::map<std::string, std::string>   m_cb_name_to_hdr_map;

  //static
  const static std::string   m_delimeter;
  const static std::string   m_q;

  void _declare_files_and_headers() {}

public:

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("TracerBase")
      .SetParent<ns3::Object> ()
      .AddConstructor<TracerBase> ();
    return tid;
  }

  TracerBase() = default;
  ~TracerBase()
  {
    for(auto& it : m_cb_out_map)
    {
      it.second.close();
    }
  }

  void CreateOutput(const std::string& post_fix)
  {
    _declare_files_and_headers();

    for(auto& it : m_cb_name_to_hdr_map)
    {
      m_cb_out_map.insert(std::make_pair(it.first, std::ofstream(it.first + "-" + post_fix)));
      m_cb_out_map.at(it.first) << it.second << std::endl;
    }
  }
};

class DistanceCalculatorAndTracer : public TracerBase
{
private:
  ns3::Ptr<ns3::MobilityModel> m_target;

  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "time" + m_delimeter +
         "px" + m_delimeter +
         "py" + m_delimeter +
         "pz" + m_delimeter +
         "v";
    m_cb_name_to_hdr_map.insert(std::make_pair("pos", ss));
  }
public:

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("DistanceCalculatorAndTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<DistanceCalculatorAndTracer> ();
    return tid;
  }

  DistanceCalculatorAndTracer() = default;
  ~DistanceCalculatorAndTracer() = default;

  void SetNodeMobilityModel(ns3::Ptr<ns3::MobilityModel> t)
  {
    m_target = t;
  }

  void DumperCb(double next)
  {
    std::ofstream& ofs = m_cb_out_map.at("pos");

    ns3::Vector pos = m_target->GetPosition (); // Get position
    ns3::Vector vel = m_target->GetVelocity (); // Get velocity

    ofs << ns3::Simulator::Now ().GetSeconds() << m_delimeter
                      << pos.x << m_delimeter
                      << pos.y << m_delimeter
                      << pos.z << m_delimeter
                      << vel.GetLength() << std::endl;
    ns3::Simulator::Schedule(ns3::Seconds(next), &DistanceCalculatorAndTracer::DumperCb, this, next);
  }
};

class AllNodesMobilityTracer : public TracerBase
{
private:
  std::map<std::string, ns3::Ptr<ns3::MobilityModel> > m_nodes_mobility;
  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "time";

    for(auto& it : m_nodes_mobility)
    {
      ss = ss + m_delimeter + "px" + it.first
              + m_delimeter + "py" + it.first
              + m_delimeter + "pz" + it.first
              + m_delimeter + "v" + it.first;
    }

    m_cb_name_to_hdr_map.insert(std::make_pair("dump", ss));
  }
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("AllNodesMobilityTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<AllNodesMobilityTracer> ();
    return tid;
  }

  AllNodesMobilityTracer() = default;
  void AddNodeMobilityModel(ns3::Ptr<ns3::MobilityModel> t, std::string n_id)
  {
    if(m_nodes_mobility.find(n_id) != m_nodes_mobility.end())
    {
      return;
    }

    m_nodes_mobility.insert(std::make_pair(n_id, t));
  }

  void DumperCb(double next)
  {
    std::ofstream& ofs = m_cb_out_map.at("dump");

    ofs << ns3::Simulator::Now ().GetSeconds();

    for(auto& it : m_nodes_mobility)
    {
      ns3::Vector pos = it.second->GetPosition(); // Get position
      ns3::Vector vel = it.second->GetVelocity (); // Get velocity

      ofs << m_delimeter << pos.x
                        << m_delimeter << pos.y
                        << m_delimeter << pos.z
                        << m_delimeter << vel.GetLength();
    }
    ofs << std::endl;

    ns3::Simulator::Schedule(ns3::Seconds(next), &AllNodesMobilityTracer::DumperCb, this, next);
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

  void Trace(const ns3::Ptr< const ns3::Packet > packet, const ns3::Address &srcAddress, const ns3::Address &destAddress)
  {
    uint8_t* buf = new uint8_t[srcAddress.GetLength ()];
    srcAddress.CopyTo (buf);
    if(std::memcmp (&m_ip_addr, buf, 4) == 0)
      m_out << ns3::Simulator::Now ().GetSeconds () << "\t" << packet->GetSize () << std::endl;
  }
};

class QueCalcer : public TracerBase
{
private:
  std::map<ns3::Ptr<const ns3::Packet>, double > que;
  uint64_t m_total_send;

  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "diff" + m_delimeter
       + "total";

    m_cb_name_to_hdr_map.insert(std::make_pair("TraceTxQue", ss));
  }
public:

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("QueCalcer")
      .SetParent<TracerBase> ()
      .AddConstructor<QueCalcer> ();
    return tid;
  }

  QueCalcer() : m_total_send(0){}
  void TraceTxStart(ns3::Ptr< const ns3::Packet > packet)
  {
    que.insert (std::pair<ns3::Ptr<const ns3::Packet>, double>(packet, ns3::Simulator::Now ().GetSeconds ()));
  }

  void TraceTxEnd(ns3::Ptr< const ns3::Packet > packet)
  {
    std::ofstream& ofs = m_cb_out_map.at("TraceTxQue");

    double now = ns3::Simulator::Now ().GetSeconds ();
    double diff = now - que[packet];
    m_total_send += packet->GetSize ();
    ofs << now << "\t" << diff << "\t" << m_total_send << std::endl;
  }
};

class NodeMobTracer : public TracerBase
{
private:
  ns3::Ptr<ns3::MobilityModel> ap_mobility;

  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "px" + m_delimeter
       + "py" + m_delimeter
       + "v" + m_delimeter
       + "d" + m_delimeter
       + "apx" + m_delimeter
       + "apy";

    m_cb_name_to_hdr_map.insert(std::make_pair("CourseChange", ss));
  }

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("NodeMobTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<NodeMobTracer> ();
    return tid;
  }

  NodeMobTracer() : ap_mobility(nullptr){}
  void SetApMobilityModel(ns3::Ptr<ns3::MobilityModel> mob)
  {
    ap_mobility = mob;
  }
  void CourseChangeCb(ns3::Ptr< const ns3::MobilityModel > model)
  {
    std::ofstream& ofs = m_cb_out_map.at("CourseChange");

    ofs << ns3::Simulator::Now ().GetSeconds () << "\t"
        << model->GetPosition ().x << "\t"
        << model->GetPosition ().y << "\t"
        << model->GetVelocity ().GetLength () << "\t"
        << model->GetDistanceFrom (ap_mobility) << "\t"
        << ap_mobility->GetPosition ().x << "\t"
        << ap_mobility->GetPosition ().y
        << std::endl;
  }
};

class PingTracer : public TracerBase
{
private:
  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "rtt";

    m_cb_name_to_hdr_map.insert(std::make_pair("RttPing", ss));
  }

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("PingTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<PingTracer> ();
    return tid;
  }

  PingTracer() = default;

  void RttPingCb(ns3::Time rtt)
  {
    std::ofstream& ofs = m_cb_out_map.at("RttPing");

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << rtt.GetSeconds ()
        << std::endl;
  }
};

class ArpTracer : public TracerBase
{
private:
  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "p_id";

    m_cb_name_to_hdr_map.insert(std::make_pair("ArpDrop", ss));
  }

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("ArpTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<ArpTracer> ();
    return tid;
  }

  ArpTracer() = default;

  void ArpDropCb(ns3::Ptr<const ns3::Packet> p)
  {
    std::ofstream& ofs = m_cb_out_map.at("ArpDrop");
    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid()
        << std::endl;
  }
};

class WifiPhyTracer : public TracerBase
{
private:
  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "p_id" + m_delimeter
       + "reas";

    m_cb_name_to_hdr_map.insert(std::make_pair("WifiPhyDrop", ss));
  }
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("WifiPhyTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<WifiPhyTracer> ();
    return tid;
  }

  WifiPhyTracer() = default;

  void WifiPhyDropCb(ns3::Ptr<const ns3::Packet> p, ns3::WifiPhyRxfailureReason reason)
  {
    std::ofstream& ofs = m_cb_out_map.at("WifiPhyDrop");

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << static_cast<uint32_t>(reason)
        << std::endl;
  }
};

class WifiPhyStateTracer : public TracerBase
{
private:

  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "Time,\tpckt_id,\tsnr,\tmode_id,\tpreamb";
    m_cb_name_to_hdr_map.insert(std::make_pair("RxOkCb", ss));

    ss = "Time,\tpckt_id,\tsnr";
    m_cb_name_to_hdr_map.insert(std::make_pair("RxError", ss));

    ss = "Time,\tpckt_id,\tmode_id,\tpreamb,\tpw";
    m_cb_name_to_hdr_map.insert(std::make_pair("Tx", ss));
  }
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("WifiPhyStateTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<WifiPhyStateTracer> ();
    return tid;
  }

  WifiPhyStateTracer() = default;
  ~WifiPhyStateTracer() = default;

  void RxOkCb(ns3::Ptr<const ns3::Packet> p, double d, ns3::WifiMode m, ns3::WifiPreamble pr)
  {
    std::ofstream& ofs = m_cb_out_map.at("RxOkCb");
    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << d << m_delimeter
        << m_q << m.GetUniqueName() << m_q << m_delimeter
        << static_cast<uint32_t>(pr)
        << std::endl;
  }

  void RxErrorCb(ns3::Ptr<const ns3::Packet> p, double d)
  {
    std::ofstream& ofs = m_cb_out_map.at("RxError");
    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << d
        << std::endl;
  }

  void TxCb(ns3::Ptr<const ns3::Packet> p, ns3::WifiMode m, ns3::WifiPreamble pr, uint8_t hz)
  {
    std::ofstream& ofs = m_cb_out_map.at("Tx");
    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << m_q << m.GetUniqueName() << m_q << m_delimeter
        << static_cast<uint32_t>(pr) << m_delimeter
        << static_cast<uint32_t>(hz)
        << std::endl;
  }
};

class Ipv4L3ProtocolTracer : public TracerBase
{
private:

  //Static
  static StatsCollector m_stats;
  static std::ofstream m_stats_out;
  //
  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst";
    m_cb_name_to_hdr_map.insert(std::make_pair("Tx", ss));

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst";
    m_cb_name_to_hdr_map.insert(std::make_pair("Rx", ss));

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst" + m_delimeter +
         "reas";
    m_cb_name_to_hdr_map.insert(std::make_pair("Drop", ss));

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst";
    m_cb_name_to_hdr_map.insert(std::make_pair("UnicastForward", ss));

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst";
    m_cb_name_to_hdr_map.insert(std::make_pair("LocalDeliver", ss));
  }
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("Ipv4L3ProtocolTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<Ipv4L3ProtocolTracer> ();
    return tid;
  }

  static void DumpStatsTo(const std::string& str)
  {
    if(m_stats_out.is_open())
    {
      m_stats_out.close();
    }

    m_stats_out.open(str);

    m_stats_out << "t" << TracerBase::m_delimeter
                << "tx_pckt" << TracerBase::m_delimeter
                << "tx_bytes" << TracerBase::m_delimeter
                << "rx_pckt" << TracerBase::m_delimeter
                << "rx_bytes" << TracerBase::m_delimeter
                << "pdr_all"
                << std::endl;

    m_stats.SetRxPkts(0);
    m_stats.SetTxPkts(0);
    m_stats.SetTxBytes(0);
    m_stats.SetRxBytes(0);
  }

  static void StatsDumper(double interval)
  {
    uint32_t rx = m_stats.GetRxPkts();
    uint32_t tx = m_stats.GetTxPkts();
    double pdr = 1.0;
    if(tx != 0)
    {
       pdr = (double)rx / (double)tx;
    }
    m_stats_out << ns3::Simulator::Now().GetSeconds() << TracerBase::m_delimeter
                << tx << TracerBase::m_delimeter
                << m_stats.GetTxBytes() << TracerBase::m_delimeter
                << rx << TracerBase::m_delimeter
                << m_stats.GetRxBytes() << TracerBase::m_delimeter
                << pdr
                << std::endl;

    ns3::Simulator::Schedule(ns3::Seconds(interval), &Ipv4L3ProtocolTracer::StatsDumper, interval);
  }

  static void Stop()
  {
    m_stats_out.close();
  }
  //==========================================
  Ipv4L3ProtocolTracer() = default;
  ~Ipv4L3ProtocolTracer() = default;

  void TxCb(ns3::Ptr<const ns3::Packet> p, ns3::Ptr<ns3::Ipv4> ip,  uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("Tx");

    ns3::Ipv4Header ip_hdr;
    p->PeekHeader(ip_hdr);

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << std::endl;

    m_stats.IncTxPkts();
    m_stats.IncTxBytes(p->GetSize());
  }

  void RxCb(ns3::Ptr<const ns3::Packet> p, ns3::Ptr<ns3::Ipv4> ip,  uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("Rx");

    ns3::Ipv4Header ip_hdr;
    p->PeekHeader(ip_hdr);

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << std::endl;

    m_stats.IncRxPkts();
    m_stats.IncRxBytes(p->GetSize());
  }

  void DropCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, ns3::Ipv4L3Protocol::DropReason r, ns3::Ptr<ns3::Ipv4> ip, uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("Drop");

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << static_cast<uint32_t>(r)
        << std::endl;
  }

  void UnicastForwardCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("UnicastForward");

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << std::endl;

  }

  void LocalDeliverCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("LocalDeliver");

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << std::endl;
  }
};

class PDRAndThroughputMetr : public TracerBase
{
private:
  StatsCollector * m_stats;
  ns3::Time m_interval;
  ns3::Time m_total;
  uint32_t m_conn_cnter;

  void _declare_files_and_headers()
  {
    std::string ss;

    ss = "time,\ttx_all,\trx_all,\tthrput,\tpdr";
    m_cb_name_to_hdr_map.insert(std::make_pair("DumpPDR", ss));

    ss = "time,\ttx,\trx,\tp";
    m_cb_name_to_hdr_map.insert(std::make_pair("DumpConnectivity", ss));

  }
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("PDRAndThroughputMetr")
      .SetParent<TracerBase> ()
      .AddConstructor<PDRAndThroughputMetr> ();
    return tid;
  }

  PDRAndThroughputMetr() : m_stats(0), m_interval(ns3::Seconds(1.0)), m_conn_cnter(0){}

  void SetStatsCollector(StatsCollector* clt)
  {
    m_stats = clt;
  }

  void SetDumpInterval(double sec, double total_time)
  {
    m_interval = ns3::Seconds(sec);
    m_total = ns3::Seconds(total_time);
  }

  void Start()
  {
    NS_ASSERT(m_stats != nullptr);
    ns3::Simulator::Schedule(ns3::Time(0), &PDRAndThroughputMetr::DumpStatistics, this);
  }

  void DumpStatistics()
  {
    double now = ns3::Simulator::Now ().GetSeconds ();
    uint32_t bytes_rcv_interval = m_stats->GetRxBytes ();    //aggregated
    uint32_t packetsReceivedAll =  m_stats->GetCumulativeRxPkts();
    uint32_t packetsTransmitedAll =  m_stats->GetCumulativeTxPkts();

    double pdr = 1.0;
    if(packetsTransmitedAll)
    {
      pdr = (double)packetsReceivedAll / (double)packetsTransmitedAll;
    }

    m_cb_out_map.at("DumpPDR") << now << m_delimeter
                                << packetsTransmitedAll << m_delimeter
                                << packetsReceivedAll << m_delimeter
                                << (bytes_rcv_interval * 8.0) / m_interval.GetSeconds() << m_delimeter
                                << pdr << m_delimeter
                                << std::endl;

    if(m_stats->GetRxPkts() == m_stats->GetTxPkts())
    {
      m_conn_cnter++;
    }

    pdr = 1.0;
    if(m_stats->GetTxPkts())
    {
      pdr = (double)m_stats->GetRxPkts() / (double)m_stats->GetTxPkts();
    }

    m_cb_out_map.at("DumpConnectivity") << now << m_delimeter
                                        << m_stats->GetTxPkts() << m_delimeter
                                        << m_stats->GetRxPkts() << m_delimeter
                                        << pdr << m_delimeter
                                        << std::endl;

    //reset interval stats
    m_stats->SetRxBytes (0);
    m_stats->SetRxPkts (0);
    m_stats->SetTxBytes (0);
    m_stats->SetTxPkts (0);

    ns3::Simulator::Schedule(m_interval, &PDRAndThroughputMetr::DumpStatistics, this);
  }
  uint32_t GetConnectivityCnter() const
  {
    return m_conn_cnter;
  }
};

#endif // TRACERS_H

