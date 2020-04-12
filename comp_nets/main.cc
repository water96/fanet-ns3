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

#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"

#include "ns3/channel.h"
#include "ns3/net-device.h"

#include "utopiamac1.h"

using namespace ns3;

const std::string delim_str = "\n====================================\n";

const ns3::Time period = Seconds (1.0);
const uint32_t BytesToSend = 1000000;
const uint16_t p_size = 500;


void Sender(ns3::Ptr<ns3::Node> sndr, ns3::Ptr<ns3::Node> rcver)
{
  static uint32_t cur_send_bytes = 0;
  Ptr<NetDevice> s = sndr->GetDevice (0);
  Ptr<NetDevice> r = rcver->GetDevice (0);

  Ptr<Packet> p = Create<Packet>(p_size);

  s->Send (p, r->GetAddress (), NetDevice::PacketType::PACKET_HOST);

  cur_send_bytes += p_size;
  if(cur_send_bytes < BytesToSend)
  {
    Simulator::Schedule (period, &Sender, sndr, rcver);
  }
}

int main()
{

  //===========================================
  LogComponentEnableAll (LogLevel::LOG_ALL);
  //===========================================

  //===========================================
  ns3::Ptr<ns3::Node> a_node = CreateObject<ns3::Node>(); //snder
  ns3::Ptr<ns3::Node> b_node = CreateObject<ns3::Node>(); //rcver
  //===========================================

  //===========================================
  ns3::Ptr<UtopiaChannel> ch = CreateObject<UtopiaChannel>();

  ns3::Ptr<UtopiaDevice> nd_a = CreateObject<UtopiaDevice>();
  ns3::Ptr<UtopiaDevice> nd_b = CreateObject<UtopiaDevice>();

  Mac8Address a_adr(0xA);
  Mac8Address b_adr(0xB);

  nd_a->SetAddress (a_adr);
  nd_b->SetAddress (b_adr);

  nd_a->SetChannel (ch);
  nd_b->SetChannel (ch);

  std::cout << "Channel state: " << static_cast<uint8_t>(ch->GetLinkState ()) << std::endl;

  a_node->AddDevice (nd_a);
  b_node->AddDevice (nd_b);

  //nd_a->SetNode (a_node);
  //nd_b->SetNode (b_node);
  //===========================================

  Simulator::Schedule (period, &Sender, a_node, b_node);

  //===========================================
  Simulator::Run ();
  Simulator::Destroy ();
  //===========================================

  return 0;
}

