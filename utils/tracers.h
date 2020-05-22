#ifndef TRACERS_H
#define TRACERS_H

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <list>
#include <algorithm>

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

class TracerBase;

class TracerBase : public ns3::Object
{
private:
  static ExpResults m_all;
protected:
  std::map<std::string, std::ofstream> m_cb_out_map;
  std::map<std::string, std::string>   m_cb_name_to_hdr_map;
  ExpResults m_res;

  //static
  const static std::string   m_delimeter;
  const static std::string   m_q;

  static void _insert_results_of_subtraces(const ExpResults& r)
  {
    for(const auto& kv : r)
    {
      NS_ASSERT(m_all.find(kv.first) == m_all.end());
    }
    m_all.insert(r.begin(), r.end());
  }

  //funcs
  virtual const ExpResults& _dump_results()
  {
    _insert_results_of_subtraces(m_res);
    return m_res;
  }
public:

  static ExpResults GetAllTraceResults()
  {
    return m_all;
  }

  static void ResetAllTraceResults()
  {
    m_all.clear();
  }


  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("TracerBase")
      .SetParent<ns3::Object> ()
      .AddConstructor<TracerBase> ();
    return tid;
  }

  TracerBase()
  {
    ns3::Simulator::ScheduleDestroy(&TracerBase::_dump_results, this);
  }

  virtual ~TracerBase()
  {
    for(auto& it : m_cb_out_map)
    {
      it.second.close();
    }

//    auto find_it = std::find(m_all_inst.begin(), m_all_inst.end(), this);
//    m_all_inst.erase(find_it);
  }

  void CreateOutput(const std::string& post_fix)
  {
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
public:

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("DistanceCalculatorAndTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<DistanceCalculatorAndTracer> ();
    return tid;
  }

  DistanceCalculatorAndTracer()
  {
    std::string ss;

    ss = "time" + m_delimeter +
         "px" + m_delimeter +
         "py" + m_delimeter +
         "pz" + m_delimeter +
         "v";
    m_cb_name_to_hdr_map.insert(std::make_pair("pos", ss));
  }
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

class AdjTracer : public TracerBase
{
private:
  std::vector<std::pair<ns3::Ptr<ns3::WifiPhy>, ns3::Ptr<ns3::MobilityModel> > > m_nodes_wm;
  ns3::Time m_interval;
  ns3::EventId m_dump_event;
  ns3::Ptr<ns3::PropagationLossModel> m_prop_mod;
  uint32_t m_phy_connect_cnter;
  uint32_t m_total_time;
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("AdjTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<AdjTracer> ();
    return tid;
  }

  AdjTracer() : m_interval(ns3::Seconds(1.0)), m_prop_mod(nullptr), m_phy_connect_cnter(0), m_total_time(0)
  {
    std::string ss;

    ss = "time";

    m_cb_name_to_hdr_map.insert(std::make_pair("node-deg", ss));
  }

  void SetPropModel(ns3::Ptr<ns3::PropagationLossModel> mod)
  {
    m_prop_mod = mod;
  }

  void AddNodeWifiAndMobility(std::string node_id, ns3::Ptr<ns3::WifiPhy> w, ns3::Ptr<ns3::MobilityModel> m)
  {
    m_nodes_wm.push_back(std::make_pair(w, m));

    std::string ss;
    ss = ss + m_delimeter + "deg" + node_id;
    m_cb_name_to_hdr_map.at("node-deg") += ss;
  }

  void SetDumpInterval(double s)
  {
    m_interval = ns3::Seconds(s);
    m_dump_event.Cancel();
    m_dump_event = ns3::Simulator::Schedule(m_interval, &AdjTracer::DumperCb, this);
  }

  void DumperCb()
  {
    std::ofstream& ofs = m_cb_out_map.at("node-deg");

    ofs << ns3::Simulator::Now ().GetSeconds();

    bool conn = true;
    for(auto& it : m_nodes_wm)
    {
      uint16_t nei_num = 0;
      double txp = it.first->GetTxPowerStart();
      for(auto& nei : m_nodes_wm)
      {
        if(nei == it)
        {
          continue;
        }
        double dist = it.second->GetDistanceFrom(nei.second);
        if( dist > 5000.0 )
        {
//          std::cout << "MNode";
        }
        double rxp = m_prop_mod->CalcRxPower(txp, it.second, nei.second);
        double rx_sense = nei.first->GetRxSensitivity();
        if(rxp >= rx_sense)
        {
          nei_num++;
        }
        else
        {
//          std::cout << "MNode";
        }
      }

      conn &= (bool)nei_num;

      ofs << m_delimeter << nei_num;
    }
    ofs << std::endl;

    if(conn)
      m_phy_connect_cnter++;
    m_total_time++;

    ns3::Simulator::Schedule(m_interval, &AdjTracer::DumperCb, this);
  }

  virtual const ExpResults& _dump_results()
  {
    double conn = (double) m_phy_connect_cnter / (double) m_total_time;
    m_res.insert(std::make_pair("phy_conn", std::to_string(conn)));

    _insert_results_of_subtraces(m_res);
    return m_res;
  }
};

class AllNodesMobilityTracer : public TracerBase
{
private:
  std::map<std::string, ns3::Ptr<ns3::MobilityModel> > m_nodes_mobility;
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("AllNodesMobilityTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<AllNodesMobilityTracer> ();
    return tid;
  }

  AllNodesMobilityTracer()
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
  void AddNodeMobilityModel(ns3::Ptr<ns3::MobilityModel> t, std::string n_id)
  {
    if(m_nodes_mobility.find(n_id) != m_nodes_mobility.end())
    {
      return;
    }

    std::string ss = m_delimeter + "px" + n_id
                  + m_delimeter + "py" + n_id
                  + m_delimeter + "pz" + n_id
                  + m_delimeter + "v" + n_id;

    m_cb_name_to_hdr_map.at("dump") += ss;

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

public:

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("QueCalcer")
      .SetParent<TracerBase> ()
      .AddConstructor<QueCalcer> ();
    return tid;
  }

  QueCalcer() : m_total_send(0)
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "diff" + m_delimeter
       + "total";

    m_cb_name_to_hdr_map.insert(std::make_pair("TraceTxQue", ss));
  }
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

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("NodeMobTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<NodeMobTracer> ();
    return tid;
  }

  NodeMobTracer() : ap_mobility(nullptr)
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
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("PingTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<PingTracer> ();
    return tid;
  }

  PingTracer()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "rtt";

    m_cb_name_to_hdr_map.insert(std::make_pair("RttPing", ss));
  }

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
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("ArpTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<ArpTracer> ();
    return tid;
  }

  ArpTracer()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "p_id";

    m_cb_name_to_hdr_map.insert(std::make_pair("ArpDrop", ss));
  };

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
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("WifiPhyTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<WifiPhyTracer> ();
    return tid;
  }

  WifiPhyTracer()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "p_id" + m_delimeter
       + "reas";

    m_cb_name_to_hdr_map.insert(std::make_pair("WifiPhyDrop", ss));
  }

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
  StatsCollector m_stats;
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("WifiPhyStateTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<WifiPhyStateTracer> ();
    return tid;
  }

  WifiPhyStateTracer()
  {
    std::string ss;

    ss = "Time,\tpckt_id,\tsnr";
    m_cb_name_to_hdr_map.insert(std::make_pair("RxOkCb", ss));

    ss = "Time,\tpckt_id,\tsnr";
    m_cb_name_to_hdr_map.insert(std::make_pair("RxError", ss));

    ss = "Time,\tpckt_id,\tsize,\tbtr,\tpw";
    m_cb_name_to_hdr_map.insert(std::make_pair("Tx", ss));
  }
  ~WifiPhyStateTracer() = default;

  void RxOkCb(ns3::Ptr<const ns3::Packet> p, double d, ns3::WifiMode m, ns3::WifiPreamble pr)
  {
    std::ofstream& ofs = m_cb_out_map.at("RxOkCb");
    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << d
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

    static double time_stamp = 0.0;
    double now = ns3::Simulator::Now ().GetSeconds();

    double btr = 0.0;
    if(now)
    {
      btr = ((double)m_stats.GetTxBytes() * 8.0) / (now - time_stamp);
    }
    m_stats.SetTxPkts(0);
    m_stats.SetTxBytes(0);
    m_stats.IncTxPkts();
    m_stats.IncTxBytes(p->GetSize());

    ofs << now << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << btr << m_delimeter
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
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("Ipv4L3ProtocolTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<Ipv4L3ProtocolTracer> ();
    return tid;
  }

  Ipv4L3ProtocolTracer()
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
  ~Ipv4L3ProtocolTracer() = default;

  //==========================================
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
        << m_q << sdst.str() << m_q << m_delimeter
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

class IPv4AllStatsTracer : public TracerBase
{
private:
  StatsCollector m_stats;
  uint32_t m_dropped, m_forwarded;
  ns3::Time m_interval;
  ns3::EventId m_dump_event;
  //
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("IPv4AllStatsTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<IPv4AllStatsTracer> ();
    return tid;
  }

  IPv4AllStatsTracer() : m_interval(ns3::Seconds(1.0)), m_dropped(0), m_forwarded(0)
  {
    std::stringstream ss;

    ss << "t" << TracerBase::m_delimeter
                << "tx_pckt" << TracerBase::m_delimeter
                << "tx_bytes" << TracerBase::m_delimeter
                << "rx_pckt" << TracerBase::m_delimeter
                << "rx_bytes" << TracerBase::m_delimeter
                << "pdr_all";

    m_cb_name_to_hdr_map.insert(std::make_pair("dump-stats", ss.str()));
  }

  void SetDumpInterval(double s)
  {
    m_interval = ns3::Seconds(s);
    m_dump_event.Cancel();
    m_dump_event = ns3::Simulator::Schedule(m_interval, &IPv4AllStatsTracer::StatsDumper, this);
  }

  void AddCollectingStatsFrom(ns3::Ptr<ns3::Ipv4> ip)
  {
    ip->TraceConnectWithoutContext("Tx", MakeCallback(&IPv4AllStatsTracer::TxCb, this));
    ip->TraceConnectWithoutContext("Rx", MakeCallback(&IPv4AllStatsTracer::RxCb, this));
    ip->TraceConnectWithoutContext("Drop", MakeCallback(&IPv4AllStatsTracer::DropCb, this));
    ip->TraceConnectWithoutContext("UnicastForward", MakeCallback(&IPv4AllStatsTracer::UnicastForwardCb, this));
    ip->TraceConnectWithoutContext("LocalDeliver", MakeCallback(&IPv4AllStatsTracer::LocalDeliverCb, this));
  }

  void StatsDumper()
  {
    uint32_t rx = m_stats.GetRxPkts();
    uint32_t tx = m_stats.GetTxPkts();
    double pdr = 1.0;
    if(tx != 0)
    {
       pdr = (double)rx / (double)tx;
    }
    m_cb_out_map.at("dump-stats") << ns3::Simulator::Now().GetSeconds() << TracerBase::m_delimeter
                                  << tx << TracerBase::m_delimeter
                                  << m_stats.GetTxBytes() << TracerBase::m_delimeter
                                  << rx << TracerBase::m_delimeter
                                  << m_stats.GetRxBytes() << TracerBase::m_delimeter
                                  << pdr
                                  << std::endl;

    ns3::Simulator::Schedule(m_interval, &IPv4AllStatsTracer::StatsDumper, this);
  }

  //Callbacks
  void TxCb(ns3::Ptr<const ns3::Packet> p, ns3::Ptr<ns3::Ipv4> ip,  uint32_t ifs)
  {
    m_stats.IncTxPkts();
    m_stats.IncTxBytes(p->GetSize());
  }
  void RxCb(ns3::Ptr<const ns3::Packet> p, ns3::Ptr<ns3::Ipv4> ip,  uint32_t ifs)
  {

  }
  void DropCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, ns3::Ipv4L3Protocol::DropReason r, ns3::Ptr<ns3::Ipv4> ip, uint32_t ifs)
  {
    m_dropped++;
  }
  void UnicastForwardCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    m_forwarded++;
  }
  void LocalDeliverCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    m_stats.IncRxPkts();
    m_stats.IncRxBytes(p->GetSize());
  }
  //

  virtual const ExpResults& _dump_results()
  {
    m_res.insert(std::make_pair("ip_tx", std::to_string(m_stats.GetCumulativeTxPkts())));
    m_res.insert(std::make_pair("ip_rx", std::to_string(m_stats.GetCumulativeRxPkts())));
    m_res.insert(std::make_pair("ip_drop", std::to_string(m_dropped)));
    m_res.insert(std::make_pair("ip_forward", std::to_string(m_forwarded)));

    _insert_results_of_subtraces(m_res);
    return m_res;
  }
};

class PDRAndThroughputMetr : public TracerBase
{
private:
  StatsCollector m_stats;
  ns3::Time m_interval;
  ns3::Time m_total;
  uint32_t m_conn_cnter;
  double m_pdr_val;

  std::map<ns3::Address, StatsCollector> m_tx_rx_per_node;

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("PDRAndThroughputMetr")
      .SetParent<TracerBase> ()
      .AddConstructor<PDRAndThroughputMetr> ();
    return tid;
  }

  PDRAndThroughputMetr() : m_interval(ns3::Seconds(1.0)), m_conn_cnter(0)
  {
    std::string ss;

    ss = "time,\ttx_all,\trx_all,\tthrput,\tpdr";
    m_cb_name_to_hdr_map.insert(std::make_pair("DumpPDR", ss));

    ss = "time,\ttx,\trx,\tp";
    m_cb_name_to_hdr_map.insert(std::make_pair("DumpConnectivity", ss));

    ss = "node_id" + m_delimeter + "tx" + m_delimeter + "rx";
    m_cb_name_to_hdr_map.insert(std::make_pair("tx_rx_per_node", ss));

  }

  void TxCb(ns3::Ptr<ns3::Packet> p, ns3::Ptr<const ns3::Socket> src)
  {
    m_stats.IncTxPkts();
    m_stats.IncTxBytes(p->GetSize());

    ns3::Address addr;
    src->GetPeerName(addr);

    m_tx_rx_per_node[addr].IncTxPkts();
  }

  void RxCb(ns3::Ptr<ns3::Packet> p, ns3::Ptr<const ns3::Socket> local)
  {
    uint32_t RxBytes = p->GetSize ();
    m_stats.IncRxBytes (RxBytes);
    m_stats.IncRxPkts ();

    ns3::Address addr;
    local->GetSockName(addr);

    m_tx_rx_per_node[addr].IncRxPkts();
  }

  void SetDumpInterval(double sec, double total_time)
  {
    m_interval = ns3::Seconds(sec);
    m_total = ns3::Seconds(total_time);
  }
  void Start()
  {
    ns3::Simulator::Schedule(ns3::Time(0), &PDRAndThroughputMetr::DumpStatistics, this);
    ns3::Simulator::ScheduleDestroy(&PDRAndThroughputMetr::DumpTxRxPerNode, this);
  }

  void DumpTxRxPerNode()
  {

    for(auto& it : m_tx_rx_per_node)
    {
      ns3::Ipv4Address addr = ns3::InetSocketAddress::ConvertFrom(it.first).GetIpv4();
      std::stringstream n_addr;
      addr.Print(n_addr);
      m_cb_out_map.at("tx_rx_per_node") << m_q << n_addr.str() << m_q << m_delimeter
                                        << it.second.GetCumulativeTxPkts() << m_delimeter
                                        << it.second.GetCumulativeRxPkts()
                                        << std::endl;
    }
  }

  void DumpStatistics()
  {
    double now = ns3::Simulator::Now ().GetSeconds ();
    uint32_t bytes_rcv_interval = m_stats.GetRxBytes ();    //aggregated
    uint32_t packetsReceivedAll =  m_stats.GetCumulativeRxPkts();
    uint32_t packetsTransmitedAll =  m_stats.GetCumulativeTxPkts();

    m_pdr_val = 1.0;
    if(packetsTransmitedAll)
    {
      m_pdr_val = (double)packetsReceivedAll / (double)packetsTransmitedAll;
    }

    m_cb_out_map.at("DumpPDR") << now << m_delimeter
                                << packetsTransmitedAll << m_delimeter
                                << packetsReceivedAll << m_delimeter
                                << (bytes_rcv_interval * 8.0) / m_interval.GetSeconds() << m_delimeter
                                << m_pdr_val
                                << std::endl;

    double pdr = 1.0;
    packetsReceivedAll =  m_stats.GetRxPkts();
    packetsTransmitedAll =  m_stats.GetTxPkts();
    if(packetsReceivedAll == packetsTransmitedAll)
    {
      m_conn_cnter++;
    }

    if(packetsTransmitedAll)
    {
      pdr = (double)packetsReceivedAll / (double)packetsTransmitedAll;
    }

    m_cb_out_map.at("DumpConnectivity") << now << m_delimeter
                                        << packetsTransmitedAll << m_delimeter
                                        << packetsReceivedAll << m_delimeter
                                        << pdr
                                        << std::endl;

    //reset interval stats
    m_stats.SetRxBytes (0);
    m_stats.SetRxPkts (0);
    m_stats.SetTxBytes (0);
    m_stats.SetTxPkts (0);

    ns3::Simulator::Schedule(m_interval, &PDRAndThroughputMetr::DumpStatistics, this);
  }

  virtual const ExpResults& _dump_results()
  {
    double conn = (m_conn_cnter * m_interval.GetSeconds()) / m_total.GetSeconds();
    m_res.insert(std::make_pair("udp_conn", std::to_string(conn)));
    m_res.insert(std::make_pair("udp_pdr", std::to_string(m_pdr_val)));
    m_res.insert(std::make_pair("udp_tx", std::to_string(m_stats.GetCumulativeTxPkts())));
    m_res.insert(std::make_pair("udp_rx", std::to_string(m_stats.GetCumulativeRxPkts())));

    _insert_results_of_subtraces(m_res);
    return m_res;
  }

};

#endif // TRACERS_H

