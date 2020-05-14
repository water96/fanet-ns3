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

class TracerBase
{
protected:
  std::ofstream m_out;
  std::string   m_delimeter;
  std::string   m_q;
public:
  TracerBase()
  {
    m_delimeter = ",\t";
    m_q = "\"";
  }
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

class DistanceCalculatorAndTracer : public TracerBase
{
private:
  ns3::Ptr<ns3::MobilityModel> m_target;
public:
  DistanceCalculatorAndTracer() {}
  void SetNodeMobilityModel(ns3::Ptr<ns3::MobilityModel> t)
  {
    m_target = t;
  }

  void CreateOutput(const std::string& name)
  {
    TracerBase::CreateOutput(name);

    TracerBase::m_out << "time,\t"
                     << "px,\t"
                     << "py,\t"
                     << "pz,\t"
                     << "v"
                     << std::endl;
  }

  void DumperCb(double next)
  {
    ns3::Vector pos = m_target->GetPosition (); // Get position
    ns3::Vector vel = m_target->GetVelocity (); // Get velocity

    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds() << m_delimeter
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
public:
  AllNodesMobilityTracer() {}
  void AddNodeMobilityModel(ns3::Ptr<ns3::MobilityModel> t, std::string n_id)
  {
    if(m_nodes_mobility.find(n_id) != m_nodes_mobility.end())
    {
      return;
    }

    m_nodes_mobility.insert(std::make_pair(n_id, t));
  }

  void CreateOutput(const std::string& name)
  {
    TracerBase::CreateOutput(name);

    TracerBase::m_out << "time";

    for(auto& it : m_nodes_mobility)
    {
      TracerBase::m_out << ",\tpx" << it.first
                        << ",\tpy" << it.first
                        << ",\tpz" << it.first
                        << ",\tv" << it.first;
    }

    TracerBase::m_out << std::endl;
  }

  void DumperCb(double next)
  {
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds();

    for(auto& it : m_nodes_mobility)
    {
      ns3::Vector pos = it.second->GetPosition(); // Get position
      ns3::Vector vel = it.second->GetVelocity (); // Get velocity

      TracerBase::m_out << m_delimeter << pos.x
                        << m_delimeter << pos.y
                        << m_delimeter << pos.z
                        << m_delimeter << vel.GetLength();
    }
    TracerBase::m_out << std::endl;

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
  QueCalcer() : m_total_send(0){}
  void TraceTxStart(ns3::Ptr< const ns3::Packet > packet)
  {
    que.insert (std::pair<ns3::Ptr<const ns3::Packet>, double>(packet, ns3::Simulator::Now ().GetSeconds ()));
  }

  void TraceTxEnd(ns3::Ptr< const ns3::Packet > packet)
  {
    double now = ns3::Simulator::Now ().GetSeconds ();
    double diff = now - que[packet];
    m_total_send += packet->GetSize ();
    m_out << now << "\t" << diff << "\t" << m_total_send << std::endl;
  }
};

class MobTracer : public TracerBase
{
private:
  ns3::Ptr<ns3::MobilityModel> ap_mobility;
public:
  MobTracer() : ap_mobility(nullptr){}
  void SetApMobilityModel(ns3::Ptr<ns3::MobilityModel> mob)
  {
    ap_mobility = mob;
  }
  void CourseChangeCb(ns3::Ptr< const ns3::MobilityModel > model)
  {
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << "\t" << model->GetPosition ().x << "\t" << model->GetPosition ().y << "\t" << model->GetVelocity ().GetLength () << "\t" << model->GetDistanceFrom (ap_mobility) << "\t" << ap_mobility->GetPosition ().x << "\t" << ap_mobility->GetPosition ().y << std::endl;
  }
};

class PingTracer : public TracerBase
{
private:
public:
  PingTracer(){}
  void CreateOutput(const std::string& name)
  {
    TracerBase::CreateOutput(name);

    TracerBase::m_out << "time,\t"
                     << "rtt"
                     << std::endl;
  }

  void RttPingCb(ns3::Time rtt)
  {
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << m_delimeter << rtt.GetSeconds () << std::endl;
  }
};

class ArpTracer : public TracerBase
{
private:
public:
  ArpTracer(){}
  void CreateOutput(const std::string& name)
  {
    TracerBase::CreateOutput(name);

    TracerBase::m_out << "time,\t"
                     << "p_id"
                     << std::endl;
  }

  void ArpDropCb(ns3::Ptr<const ns3::Packet> p)
  {
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << m_delimeter << p->GetUid() << std::endl;
  }
};

class WifiPhyTracer : public TracerBase
{
private:
public:
  WifiPhyTracer(){}
  void CreateOutput(const std::string& name)
  {
    TracerBase::CreateOutput(name);

    TracerBase::m_out << "time,\t"
                     << "pckt_id,\t"
                     << "reas"
                     << std::endl;
  }

  void WifiPhyDropCb(ns3::Ptr<const ns3::Packet> p, ns3::WifiPhyRxfailureReason reason)
  {
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << m_delimeter << p->GetUid() << m_delimeter << static_cast<uint32_t>(reason) << std::endl;
  }
};

class WifiPhyStateTracer : public TracerBase
{
private:
  std::map<std::string, std::ofstream> m_cb_out_map;
  std::map<std::string, std::string> m_cb_name_to_hdr_map;
public:
  WifiPhyStateTracer()
  {
    std::string ss;

    ss = "Time,\tpckt_id,\tsnr,\tmode_id,\tpreamb";
    m_cb_name_to_hdr_map.insert(std::make_pair("RxOkCb", ss));

    ss = "Time,\tpckt_id,\tsnr";
    m_cb_name_to_hdr_map.insert(std::make_pair("RxError", ss));

    ss = "Time,\tpckt_id,\tmode_id,\tpreamb,\tpw";
    m_cb_name_to_hdr_map.insert(std::make_pair("Tx", ss));
  }

  ~WifiPhyStateTracer()
  {
    for(auto& it : m_cb_out_map)
    {
      it.second.close();
    }
  }
  void CreateOutput(const std::string& post_fix)
  {
    for(auto& it : m_cb_name_to_hdr_map)
    {
      m_cb_out_map.insert(std::make_pair(it.first, std::ofstream(it.first + "-" + post_fix)));
      m_cb_out_map.at(it.first) << it.second << std::endl;
    }
  }

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
  std::map<std::string, std::ofstream> m_cb_out_map;
  std::map<std::string, std::string> m_cb_name_to_hdr_map;
public:
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

  ~Ipv4L3ProtocolTracer()
  {
    for(auto& it : m_cb_out_map)
    {
      it.second.close();
    }
  }
  void CreateOutput(const std::string& post_fix)
  {
    for(auto& it : m_cb_name_to_hdr_map)
    {
      m_cb_out_map.insert(std::make_pair(it.first, std::ofstream(it.first + "-" + post_fix)));
      m_cb_out_map.at(it.first) << it.second << std::endl;
    }
  }

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
public:
  PDRAndThroughputMetr() : m_stats(0), m_interval(ns3::Seconds(1.0)){}
  void CreateOutput(const std::string& name)
  {
    TracerBase::CreateOutput(name);

    TracerBase::m_out << "time" << m_delimeter
                      << "tx_all" << m_delimeter
                      << "rx_all" << m_delimeter
                      << "thrput" << m_delimeter
                      << "pdr" << m_delimeter   //not increased
                      << std::endl;
  }
  void SetStatsCollector(StatsCollector* clt)
  {
    m_stats = clt;
  }

  void SetDumpInterval(double sec)
  {
    m_interval = ns3::Seconds(sec);
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

    TracerBase::m_out << now << m_delimeter
                      << packetsTransmitedAll << m_delimeter
                      << packetsReceivedAll << m_delimeter
                      << (bytes_rcv_interval * 8.0) / m_interval.GetSeconds() << m_delimeter
                      << pdr << m_delimeter
                      << std::endl;
    //reset interval stats
    m_stats->SetRxBytes (0);
    m_stats->SetRxPkts (0);

    ns3::Simulator::Schedule(m_interval, &PDRAndThroughputMetr::DumpStatistics, this);
  }
};


#endif // TRACERS_H

