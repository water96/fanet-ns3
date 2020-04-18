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
#include "ns3/callback.h"

#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"

#include "ns3/channel.h"
#include "ns3/net-device.h"

#include "utopiamac1.h"
#include "utils/tracers.h"

using namespace ns3;

const std::string delim_str = "\n====================================\n";

const ns3::Time period = MicroSeconds (10);
const uint32_t BytesToSend = 200000;
const uint16_t p_size = 500;

class UpperHeader : public ns3::Header
{
public:
  uint32_t m_seq;
public:

  UpperHeader (){}
  virtual ~UpperHeader (){}

  static ns3::TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::UpperHeader")
      .SetParent<Header> ()
      .AddConstructor<UpperHeader> ()
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
    os << "seq=" << m_seq;
  }
  virtual void Serialize (ns3::Buffer::Iterator start) const
  {
    // we can serialize two bytes at the start of the buffer.
    // we write them in network byte order.
    start.WriteHtonU32 (m_seq);
  }
  virtual uint32_t Deserialize (ns3::Buffer::Iterator start)
  {
    // we can deserialize two bytes from the start of the buffer.
    // we read them in network byte order and store them
    // in host byte order.
    m_seq = start.ReadNtohU32 ();

    // we return the number of bytes effectively read.
    return sizeof(m_seq);
  }
  virtual uint32_t GetSerializedSize (void) const
  {
    return sizeof(m_seq);
  }

};

class Snder : public TracerBase
{
private:
  uint32_t m_cur_send_bytes;
  uint32_t m_pckt_cnter;
public:
  Snder() : m_pckt_cnter(0), m_cur_send_bytes(0) {}

  void Snd(ns3::Ptr<ns3::Node> sndr, ns3::Ptr<ns3::Node> rcver)
  {
    Ptr<NetDevice> s = sndr->GetDevice (0);
    Ptr<NetDevice> r = rcver->GetDevice (0);

    Ptr<Packet> p = Create<Packet>(p_size);
    UpperHeader hdr;
    hdr.m_seq = m_pckt_cnter;

    p->AddHeader (hdr);

    if( s->Send (p, r->GetAddress (), NetDevice::PacketType::PACKET_HOST))
    {
      m_pckt_cnter++;
      m_cur_send_bytes += p_size;
    }

    if(m_cur_send_bytes < BytesToSend)
    {
      Simulator::Schedule (period, &Snder::Snd, this, sndr, rcver);
    }
  }

};


class RcvStatCollector : public TracerBase
{
private:
  uint32_t m_cnter;
  ns3::Time m_t;
public:
  RcvStatCollector() : m_cnter(0), m_t(0) {}

  bool Receive(Ptr<NetDevice> dev, Ptr<const Packet> packet, uint16_t protocol, const Address& from)
  {
    UpperHeader hdr;
    ns3::Time now = Simulator::Now ();
    Ptr<Packet> copy_p = packet->Copy ();
    double speed = packet->GetSize () * 8.0 / (now.GetSeconds () - m_t.GetSeconds ());
    copy_p->RemoveHeader (hdr);
    std::string ss("");
    if(hdr.m_seq != m_cnter)
    {
      ss = "1";
      m_cnter = hdr.m_seq;
    }

    TracerBase::m_out << now.GetSeconds () << "\t" << hdr.m_seq << "\t" << speed << "\t" << ss << std::endl;
    m_cnter++;
    m_t = now;
    return true;
  }
};

class DropsCollector : public TracerBase
{
private:

public:
  DropsCollector() {}

  void Drop(ns3::Ptr<const ns3::Packet> packet)
  {
    UtopiaMacHeader hdr;
    ns3::Time now = Simulator::Now ();
    Ptr<Packet> copy_p = packet->Copy ();
    copy_p->RemoveHeader (hdr);
    TracerBase::m_out << now.GetSeconds () << "\t" << static_cast<int>(hdr.m_kind) << std::endl;
  }
};



int main()
{

  //===========================================
  //LogComponentEnableAll (LogLevel::LOG_ALL);
  //===========================================

  //===========================================
  ns3::Ptr<ns3::Node> a_node = CreateObject<ns3::Node>(); //snder
  ns3::Ptr<ns3::Node> b_node = CreateObject<ns3::Node>(); //rcver
  //===========================================

  //===========================================
  ns3::Ptr<UtopiaChannel> ch = CreateObject<UtopiaChannel>();
  ch->SetAttribute ("Delay", TimeValue(MilliSeconds (50)));

  ns3::Ptr<UtopiaDevice> nd_a = CreateObject<UtopiaDevice>();
  ns3::Ptr<UtopiaDevice> nd_b = CreateObject<UtopiaDevice>();

  Mac8Address a_adr(0xA);
  Mac8Address b_adr(0xB);

  nd_a->SetAddress (a_adr);
  nd_b->SetAddress (b_adr);

  nd_a->SetChannel (ch);
  nd_b->SetChannel (ch);

  nd_a->SetAttribute ("DataRate", DataRateValue (DataRate ("50kb/s")));
  nd_b->SetAttribute ("DataRate", DataRateValue (DataRate ("50kb/s")));

  std::cout << "Channel state: " << static_cast<uint8_t>(ch->GetLinkState ()) << std::endl;

  a_node->AddDevice (nd_a);
  b_node->AddDevice (nd_b);
  //===========================================

  //===========================================
  Snder sn;
  Simulator::Schedule (period, &Snder::Snd, &sn, a_node, b_node);
  //===========================================

  //===========================================
  RcvStatCollector st;
  st.CreateOutput ("utopia3.dat");
  nd_b->SetReceiveCallback ( MakeCallback(&RcvStatCollector::Receive, &st ));
  //===========================================

  //===========================================
  DropsCollector dr;
  dr.CreateOutput ("drops_utopia3.dat");
  nd_a->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&DropsCollector::Drop, &dr));
  nd_b->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&DropsCollector::Drop, &dr));
  //===========================================


  //===========================================
  Simulator::Run ();
  Simulator::Destroy ();
  //===========================================

  return 0;
}

