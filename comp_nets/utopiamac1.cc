#include "utopiamac1.h"

using namespace ns3;

//
//      Header
//

TypeId UtopiaMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UtopiaMacHeader")
    .SetParent<Header> ()
    .AddConstructor<UtopiaMacHeader> ()
  ;
  return tid;
}

TypeId UtopiaMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void UtopiaMacHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "kind=" << static_cast<uint8_t>(m_kind) <<"\nseq=" << m_seq << "\nack" << m_ack_seq;
}

void UtopiaMacHeader::Serialize (Buffer::Iterator start) const
{
  // we can serialize two bytes at the start of the buffer.
  // we write them in network byte order.
  start.WriteU8 (static_cast<uint8_t>(m_kind));
  start.WriteU8 (m_seq);
  start.WriteU8 (m_ack_seq);
}

uint32_t UtopiaMacHeader::Deserialize (Buffer::Iterator start)
{
  // we can deserialize two bytes from the start of the buffer.
  // we read them in network byte order and store them
  // in host byte order.
  m_kind = static_cast<frame_kind>(start.ReadU8 ());
  m_seq = start.ReadU8 ();
  m_ack_seq = start.ReadU8 ();

  // we return the number of bytes effectively read.
  return sizeof (m_kind) + sizeof(m_seq) + sizeof(m_ack_seq);
}

uint32_t UtopiaMacHeader::GetSerializedSize (void) const
{
  return sizeof (m_kind) + sizeof(m_seq) + sizeof(m_ack_seq);2;
}

//
//      Channel
//

TypeId UtopiaChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UtopiaChannel")
    .SetParent<Channel> ()
    .SetGroupName("Network")
    .AddConstructor<UtopiaChannel> ()
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&UtopiaChannel::m_delay),
                   MakeTimeChecker ())
  ;
  return tid;
}

UtopiaChannel::UtopiaChannel () : m_delay (0), m_state(LINK_STATE::OFF), m_dev1(nullptr), m_dev2(nullptr)
{
  NS_LOG_FUNCTION (this);
}

UtopiaChannel::~UtopiaChannel(){}

void UtopiaChannel::set_link_state(LINK_STATE st)
{
  m_state = st;
}

UtopiaChannel::LINK_STATE UtopiaChannel::GetLinkState() const
{
  return m_state;
}

uint8_t UtopiaChannel::Send (Ptr<Packet> p, uint16_t protocol, Ptr<UtopiaDevice> sender)
{
  NS_LOG_FUNCTION (this << p << protocol << sender);

  if(sender == m_dev1)
  {
      Simulator::ScheduleWithContext (m_dev2->GetNode()->GetId(), m_delay, &UtopiaDevice::Receive, m_dev2, p->Copy (), protocol, Mac8Address::ConvertFrom(sender->GetAddress ()));
      //Simulator::ScheduleWithContext (this->GetId (), m_delay, &UtopiaChannel::set_link_state, this, LINK_STATE::IDLE);
      Simulator::Schedule (m_delay, &UtopiaChannel::set_link_state, this, LINK_STATE::IDLE);
      m_state = LINK_STATE::BUSY;
      return 0;
  }

  if(sender == m_dev2)
  {
      Simulator::ScheduleWithContext (m_dev1->GetNode()->GetId(), m_delay, &UtopiaDevice::Receive, m_dev1, p->Copy (), protocol, Mac8Address::ConvertFrom(sender->GetAddress ()));
      //Simulator::ScheduleWithContext (this->GetId (), m_delay, &UtopiaChannel::set_link_state, this, LINK_STATE::IDLE);
      Simulator::Schedule (m_delay, &UtopiaChannel::set_link_state, this, LINK_STATE::IDLE);
      m_state = LINK_STATE::BUSY;
      return 0;
  }

  m_state = LINK_STATE::IDLE;
  return 1;
}

bool UtopiaChannel::Add (Ptr<UtopiaDevice> dev)
{
  NS_LOG_FUNCTION (this << dev);
  bool rt = false;

  if(m_dev1 == nullptr)
  {
    m_dev1 = dev;
    rt = true;
  }
  else if(m_dev2 == nullptr)
  {
    m_dev2 = dev;
    rt = true;
  }

  if(m_dev1 != nullptr && m_dev2 != nullptr)
  {
    m_state = LINK_STATE::IDLE;
  }

  return rt;
}

std::size_t UtopiaChannel::GetNDevices (void) const
{
  return 2;
}

Ptr<NetDevice> UtopiaChannel::GetDevice (std::size_t i) const
{
  Ptr<NetDevice> ret = nullptr;
  if(i == 0)
  {
    ret = m_dev1;
  }
  else if (i == 1)
  {
    ret = m_dev2;
  }
  return ret;
}

//
//      Device
//

const uint16_t UtopiaDevice::UTOPIA_MTU = 1024;

UtopiaDevice::~UtopiaDevice(){}

TypeId
UtopiaDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UtopiaDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("Network")
    .AddConstructor<UtopiaDevice> ()
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been dropped "
                     "by the device during reception",
                     MakeTraceSourceAccessor (&UtopiaDevice::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

UtopiaDevice::UtopiaDevice ()
  : m_channel (0),
    m_node (0),
    m_mtu (UTOPIA_MTU),
    m_ifIndex (0),
    m_resend_timeout(MilliSeconds (RESEND_TIMEOUT_MS)),
    m_seq_cnter(0)
{
  NS_LOG_FUNCTION (this);
  m_net_lev_delay = CreateObject<UniformRandomVariable> ();
  m_net_lev_delay->SetAttribute ("Min", DoubleValue (MIN_US));
  m_net_lev_delay->SetAttribute ("Max", DoubleValue (MAX_US));

  m_queue = CreateObject<ns3::DropTailQueue<Packet> >();
  m_queue->SetMaxSize (ns3::QueueSize(QueueSizeUnit::PACKETS, 1));

  m_receiveErrorModel = CreateObject<ns3::RateErrorModel>();
  Ptr<ns3::RateErrorModel> ptr(m_receiveErrorModel->GetObject<ns3::RateErrorModel>());
  ptr->SetRate (0.001);
  ptr->SetUnit (RateErrorModel::ErrorUnit::ERROR_UNIT_PACKET);
  ptr->Enable ();
}

void
UtopiaDevice::SetChannel (Ptr<UtopiaChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);
  if(channel->Add (this))
  {
    m_channel = channel;
  }
}

//Ptr<Queue<Packet> >
//UtopiaDevice::GetQueue () const
//{
//  NS_LOG_FUNCTION (this);
//  return m_queue;
//}

//void
//UtopiaDevice::SetQueue (Ptr<Queue<Packet> > q)
//{
//  NS_LOG_FUNCTION (this << q);
//  m_queue = q;
//}

void
UtopiaDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
UtopiaDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}
uint32_t
UtopiaDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}
Ptr<Channel>
UtopiaDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}
void
UtopiaDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac8Address::ConvertFrom (address);
}
Address
UtopiaDevice::GetAddress (void) const
{
  //
  // Implicit conversion from Mac8Address to Address
  //
  NS_LOG_FUNCTION (this);
  return m_address;
}
bool
UtopiaDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}
uint16_t
UtopiaDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}
bool
UtopiaDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}
void
UtopiaDevice::AddLinkChangeCallback (Callback<void> callback)
{
 NS_LOG_FUNCTION (this << &callback);
 //m_linkChangeCallbacks.ConnectWithoutContext (callback);
}
bool
UtopiaDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;

}
Address
UtopiaDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac8Address (0xFF);
}
bool
UtopiaDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}
Address
UtopiaDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac8Address (0xFF);
}

Address UtopiaDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac8Address (0xFF);
}

bool
UtopiaDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return true;

}

bool
UtopiaDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void UtopiaDevice::proc_with_delay(ns3::Ptr<ns3::Packet> packet, uint16_t protocol, ns3::Mac8Address from)
{
  Ptr<Packet> p = packet->Copy ();
  UtopiaMacHeader rx_mch;
  p->RemoveHeader (rx_mch);

  m_rxCallback (this, p, protocol, from);
  Ptr<Packet> packet_answer = Create<Packet>();
  UtopiaMacHeader mch;
  mch.m_ack_seq = rx_mch.m_seq;
  mch.m_seq = m_seq_cnter;
  mch.m_kind = UtopiaMacHeader::frame_kind::ack;
  packet_answer->AddHeader (mch);
  m_channel->Send (packet_answer, protocol, this);
}

void
UtopiaDevice::Receive (Ptr<Packet> packet, uint16_t protocol, Mac8Address from)
{
  NS_LOG_FUNCTION (this << packet << protocol << from);

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
      m_phyRxDropTrace (packet);
      return;
    }

  Ptr<Packet> p = packet->Copy ();
  UtopiaMacHeader mch;
  p->RemoveHeader (mch);

  switch (mch.m_kind)
  {
    case UtopiaMacHeader::frame_kind::ack:
      if( mch.m_ack_seq == m_seq_cnter )
      {
        m_queue->Dequeue ();
        m_resend_by_timeout_event.Cancel ();
        m_seq_cnter = (m_seq_cnter + 1)&SEQ_MASK;
      }
    break;

    case UtopiaMacHeader::frame_kind::nak:
    break;

    case UtopiaMacHeader::frame_kind::data:
      if(m_net_level_proc_event.IsRunning ())
      {

      }
      else
      {
        if(mch.m_seq == (m_seq_cnter & SEQ_MASK))
        {
          //auto d = this->m_net_lev_delay->GetInteger ();
          auto d = 0; //No delay to process
          m_net_level_proc_event = Simulator::Schedule ( MicroSeconds (d), &UtopiaDevice::proc_with_delay, this, packet, protocol, from);
          m_seq_cnter = (m_seq_cnter + 1)&SEQ_MASK;
        }
        else
        {
          Ptr<Packet> packet_answer = Create<Packet>();
          UtopiaMacHeader tx_mch;
          tx_mch.m_ack_seq = (m_seq_cnter - 1) & SEQ_MASK;
          tx_mch.m_seq = m_seq_cnter & SEQ_MASK;
          tx_mch.m_kind = UtopiaMacHeader::frame_kind::ack;
          packet_answer->AddHeader (tx_mch);
          m_channel->Send (packet_answer, protocol, this);
        }
      }
    break;

    default:
    break;
  }

}

bool
UtopiaDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);

  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
UtopiaDevice::SendFrom (Ptr<Packet> p, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << source << dest << protocolNumber);

  //if(!m_queue->IsEmpty ())
  if(m_resend_by_timeout_event.IsRunning ())
  {
    return false;
  }

  if( m_address != Mac8Address::ConvertFrom (source))
  {
    return false;
  }

  if(m_channel->GetLinkState () != UtopiaChannel::LINK_STATE::IDLE)
  {
    return false;
  }

  if (p->GetSize () > GetMtu ())
  {
    return false;
  }
  Ptr<Packet> packet = p->Copy ();

  Mac8Address to = Mac8Address::ConvertFrom (dest);
  Mac8Address from = Mac8Address::ConvertFrom (source);

  UtopiaMacHeader hdr;
  hdr.m_seq = m_seq_cnter & SEQ_MASK;
  hdr.m_kind = UtopiaMacHeader::frame_kind::data;
  hdr.m_ack_seq = (m_seq_cnter - 1) & SEQ_MASK;

  packet->AddHeader (hdr);

  m_channel->Send (packet, protocolNumber, this);
  m_resend_by_timeout_event = Simulator::Schedule (m_resend_timeout, &UtopiaChannel::Send, m_channel, packet, protocolNumber, this);
  m_queue->Enqueue (packet);
  return true;
}


void
UtopiaDevice::TransmitComplete ()
{
  NS_LOG_FUNCTION (this);
  return;
}

Ptr<Node>
UtopiaDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}
void
UtopiaDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}
bool
UtopiaDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}
void
UtopiaDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void
UtopiaDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_node = 0;
  m_queue->Flush ();
  if (TransmitCompleteEvent.IsRunning ())
    {
      TransmitCompleteEvent.Cancel ();
    }
  if (m_net_level_proc_event.IsRunning ())
    {
      m_net_level_proc_event.Cancel ();
    }

  NetDevice::DoDispose ();
}


void
UtopiaDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
}

bool
UtopiaDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}
