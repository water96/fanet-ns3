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

class DistanceCalculatorAndTracer : public TracerBase
{
private:
  //std::vector<ns3::Ptr<ns3::MobilityModel> > nodes_mobility;
  ns3::Ptr<ns3::MobilityModel> m_target;
public:
  DistanceCalculatorAndTracer() {}
  void SetNodesMobilityModel(ns3::Ptr<ns3::MobilityModel> t/*, const ns3::NodeContainer& c*/)
  {
    m_target = t;
  }

  void CreateOutput(const std::string& name)
  {
    TracerBase::CreateOutput(name);

    TracerBase::m_out << "time,"
                     << "p_x,"
                     << "p_y,"
                     << "p_z,"
                     << "v_x,"
                     << "v_y,"
                     << "v_z"
                     << std::endl;
  }

  void DumperCb(double next)
  {
    ns3::Vector pos = m_target->GetPosition (); // Get position
    ns3::Vector vel = m_target->GetVelocity (); // Get velocity

    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds() << ","
                      << pos.x << ","
                      << pos.y << ","
                      << pos.z << ","
                      << vel.x << ","
                      << vel.y << ","
                      << vel.z << std::endl;
    ns3::Simulator::Schedule(ns3::Seconds(next), &DistanceCalculatorAndTracer::DumperCb, this, next);
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
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << ",\t" << rtt.GetSeconds () << std::endl;
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
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << ",\t" << p->GetUid() << std::endl;
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
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << ",\t" << p->GetUid() << ",\t" << static_cast<uint32_t>(reason) << std::endl;
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
    ofs << ns3::Simulator::Now ().GetSeconds () << ",\t"
        << p->GetUid() << ",\t"
        << d << ",\t"
        << "\"" << m.GetUniqueName() << "\"" << ",\t"
        << static_cast<uint32_t>(pr)
        << std::endl;
  }

  void RxErrorCb(ns3::Ptr<const ns3::Packet> p, double d)
  {
    std::ofstream& ofs = m_cb_out_map.at("RxError");
    ofs << ns3::Simulator::Now ().GetSeconds () << ",\t"
        << p->GetUid() << ",\t"
        << d
        << std::endl;
  }

  void TxCb(ns3::Ptr<const ns3::Packet> p, ns3::WifiMode m, ns3::WifiPreamble pr, uint8_t hz)
  {
    std::ofstream& ofs = m_cb_out_map.at("Tx");
    ofs << ns3::Simulator::Now ().GetSeconds () << ",\t"
        << p->GetUid() << ",\t"
        << "\"" << m.GetUniqueName() << "\"" << ",\t"
        << static_cast<uint32_t>(pr) << ",\t"
        << static_cast<uint32_t>(hz)
        << std::endl;
  }
};

#endif // TRACERS_H
