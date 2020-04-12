#ifndef TRACERS_H
#define TRACERS_H

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
  void RttPingCb(ns3::Time rtt)
  {
    TracerBase::m_out << ns3::Simulator::Now ().GetSeconds () << "\t" << rtt.GetMilliSeconds () << std::endl;
  }
};

#endif // TRACERS_H
