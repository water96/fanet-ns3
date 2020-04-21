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
    .AddAttribute ("DataRate",
                     "The default data rate for point to point links. Zero means infinite",
                     DataRateValue (DataRate ("0b/s")),
                     MakeDataRateAccessor (&UtopiaDevice::m_bps),
                     MakeDataRateChecker ())
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
    m_send_seq_cnter(0),
    m_expect_seq_cnter(0),
    m_ack_expected_cnter(0),
    m_go_back_n(1),
    MAX_SEQ(2^m_go_back_n - 1),
    m_upper_level_locked(false),
    m_n_buffered(0)
{
  NS_LOG_FUNCTION (this);
  m_net_lev_delay = CreateObject<UniformRandomVariable> ();
  m_net_lev_delay->SetAttribute ("Min", DoubleValue (MIN_US));
  m_net_lev_delay->SetAttribute ("Max", DoubleValue (MAX_US));

  m_tx_queue = CreateObject<ns3::DropTailQueue<Packet> >();
  m_tx_queue->SetMaxSize (ns3::QueueSize(QueueSizeUnit::PACKETS, MAX_SEQ + 1));

  m_receiveErrorModel = CreateObject<ns3::RateErrorModel>();
  Ptr<ns3::RateErrorModel> ptr(m_receiveErrorModel->GetObject<ns3::RateErrorModel>());
  ptr->SetRate (0.01);
  ptr->SetUnit (RateErrorModel::ErrorUnit::ERROR_UNIT_PACKET);
  //ptr->Enable ();
  ptr->Disable();
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

void UtopiaDevice::SetGoBackN(uint8_t n)
{
  m_go_back_n = n;
  MAX_SEQ = 1;
  while(n)
  {
    MAX_SEQ = MAX_SEQ << 1;
    n--;
  }
  MAX_SEQ -= 1;
  m_buffer = std::vector<std::pair<ns3::Ptr<ns3::Packet>, ns3::EventId > >(MAX_SEQ + 1);
  m_tx_queue->SetMaxSize (ns3::QueueSize(QueueSizeUnit::PACKETS, 0));
}

bool UtopiaDevice::in_between(uint8_t a, uint8_t b, uint8_t c) const
{
  if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
    return(true);
  else
    return(false);
}

inline void UtopiaDevice::incr(uint8_t& i)
{
  if(i < MAX_SEQ)
    i++;
  else
    i = 0;
}

inline uint8_t UtopiaDevice::wrap(uint8_t i)
{
  if(i > MAX_SEQ)
    return MAX_SEQ;
  return i;
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

  if(mch.m_seq == m_expect_seq_cnter)
  {
    m_rxCallback (this, p, protocol, from);
    incr(m_expect_seq_cnter);
  }

  while(in_between(m_ack_expected_cnter, mch.m_ack_seq, m_send_seq_cnter))
  {
    m_buffer[m_ack_expected_cnter].second.Cancel();
    m_n_buffered--;
    incr(m_ack_expected_cnter);
  }

  if (mch.m_kind == UtopiaMacHeader::frame_kind::data)
  {
      send_packet_internal(mch.m_seq,
                           m_send_seq_cnter,
                           UtopiaMacHeader::frame_kind::ack,
                           nullptr,
                           protocol);
    //incr(m_send_seq_cnter);
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

    if( m_address != Mac8Address::ConvertFrom (source))
    {
        return false;
    }

    if(m_n_buffered == MAX_SEQ)
    {
      return false;
    }

    if(m_upper_level_locked)
    {
      return false;
    }

    Mac8Address to = Mac8Address::ConvertFrom (dest);
    Mac8Address from = Mac8Address::ConvertFrom (source);

    ns3::Ptr<Packet> p_tmp = p->Copy();
    bool res = send_packet_internal((m_expect_seq_cnter + MAX_SEQ) % (MAX_SEQ + 1),
                              m_send_seq_cnter,
                              UtopiaMacHeader::frame_kind::data,
                              p_tmp,
                              protocolNumber);
    if(res)
    {
        m_buffer[m_send_seq_cnter].first = p;
        m_buffer[m_send_seq_cnter].second = Simulator::Schedule (m_resend_timeout, &UtopiaDevice::timeout_event, this, m_send_seq_cnter, protocolNumber);
        incr(m_send_seq_cnter);
        m_n_buffered++;
    }
    return res;
}

bool UtopiaDevice::send_packet_internal(ns3::Ptr<ns3::Packet> p, uint16_t prot)
{
  if(m_tx_queue->GetNPackets () == m_tx_queue->GetMaxSize ().GetValue ())
  {
    return false;
  }

  if (p->GetSize () > GetMtu ())
  {
    return false;
  }

  if(TransmitCompleteEvent.IsRunning ())
  {
    return m_tx_queue->Enqueue (p);
  }

  Time txTime = Time (0);
  if (m_bps > DataRate (0))
  {
    txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
  }
  Simulator::Schedule (txTime, &UtopiaChannel::Send, m_channel, p, prot, this);
  TransmitCompleteEvent = Simulator::Schedule (txTime, &UtopiaDevice::TransmitComplete, this, p, prot);

  return true;
}

bool UtopiaDevice::send_packet_internal(uint8_t ack, uint8_t seq, UtopiaMacHeader::frame_kind kind, ns3::Ptr<ns3::Packet> p, uint16_t prot)
{
  Ptr<Packet> packet = p;
  if(packet == nullptr)
  {
    packet = Create<Packet>();
  }

  UtopiaMacHeader mch;
  mch.m_ack_seq = ack;
  mch.m_seq = seq;
  mch.m_kind = kind;
  packet->AddHeader (mch);

  return send_packet_internal(packet, prot);
}

void UtopiaDevice::timeout_event(uint8_t seq_num, uint16_t protocolNumber)
{
  bool status = true;
  m_upper_level_locked = true;
  m_buffer[seq_num].second.Cancel();

  status = send_packet_internal((m_expect_seq_cnter + MAX_SEQ) % (MAX_SEQ + 1),
                                  seq_num,
                                  UtopiaMacHeader::frame_kind::data,
                                  m_buffer[m_send_seq_cnter].first->Copy(),
                                  protocolNumber);
  if(status == false)
  {
    ns3::Time time_for_tx_complete = Simulator::GetDelayLeft(TransmitCompleteEvent);
    m_buffer[m_send_seq_cnter].second = Simulator::Schedule (time_for_tx_complete, &UtopiaDevice::timeout_event, this, seq_num, protocolNumber);
  }
  else
  {
    m_buffer[m_send_seq_cnter].second = Simulator::Schedule (m_resend_timeout, &UtopiaDevice::timeout_event, this, seq_num, protocolNumber);
  }

//  Simulator::Remove(m_buffer[seq_num].second);
  /*
  while(seq_num != m_send_seq_cnter)
  {
    m_buffer[seq_num].second.Cancel();
    Simulator::Remove(m_buffer[seq_num].second);
    if(status)
    {
      status &= send_packet_internal((m_expect_seq_cnter + MAX_SEQ) % (MAX_SEQ + 1),
                                      seq_num,
                                      UtopiaMacHeader::frame_kind::data,
                                      m_buffer[m_send_seq_cnter].first->Copy(),
                                      protocolNumber);
      if(status == false)
      {
        ns3::Time time_for_tx_complete = Simulator::GetDelayLeft(TransmitCompleteEvent);
        m_buffer[m_send_seq_cnter].second = Simulator::Schedule (time_for_tx_complete, &UtopiaDevice::timeout_event, this, seq_num, protocolNumber);
      }
      else
      {
        m_buffer[m_send_seq_cnter].second = Simulator::Schedule (m_resend_timeout, &UtopiaDevice::timeout_event, this, seq_num, protocolNumber);
      }
    }
    incr(seq_num);
  }*/
  m_upper_level_locked = !status;
}

void
UtopiaDevice::TransmitComplete (ns3::Ptr<ns3::Packet> p, uint16_t protocol)
{
  NS_LOG_FUNCTION (this);

  if(m_tx_queue->GetNPackets ())
  {
    ns3::Ptr<ns3::Packet> p_tmp = m_tx_queue->Dequeue ();

    Time txTime = Time (0);
    if (m_bps > DataRate (0))
    {
      txTime = m_bps.CalculateBytesTxTime (p_tmp->GetSize ());
    }
    Simulator::Schedule (txTime, &UtopiaChannel::Send, m_channel, p_tmp, protocol, this);
    TransmitCompleteEvent = Simulator::Schedule (txTime, &UtopiaDevice::TransmitComplete, this, p_tmp, protocol);
  }

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
  m_tx_queue->Flush ();
  auto stop = [](std::pair<ns3::Ptr<ns3::Packet>, ns3::EventId> e ){ e.second.Cancel (); };
  std::for_each(m_buffer.begin (), m_buffer.end (), stop);
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
