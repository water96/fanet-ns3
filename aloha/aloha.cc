#include "aloha.h"


using namespace ns3;

//
//      Header
//

TypeId AlohaMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AlohaMacHeader")
    .SetParent<Header> ()
    .AddConstructor<AlohaMacHeader> ()
  ;
  return tid;
}

TypeId AlohaMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void AlohaMacHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "kind=" << static_cast<uint8_t>(m_kind) <<"\nseq=" << m_seq << "\nack" << m_ack_seq;
}

void AlohaMacHeader::Serialize (Buffer::Iterator start) const
{
  // we can serialize two bytes at the start of the buffer.
  // we write them in network byte order.
  start.WriteU8 (static_cast<uint8_t>(m_kind));
  start.WriteU8 (m_seq);
  start.WriteU8 (m_ack_seq);
}

uint32_t AlohaMacHeader::Deserialize (Buffer::Iterator start)
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

uint32_t AlohaMacHeader::GetSerializedSize (void) const
{
  return sizeof (m_kind) + sizeof(m_seq) + sizeof(m_ack_seq);2;
}

//
//      Channel
//

TypeId AlohaChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AlohaChannel")
    .SetParent<Channel> ()
    .SetGroupName("Network")
    .AddConstructor<AlohaChannel> ()
  ;
  return tid;
}

AlohaChannel::AlohaChannel () : m_delay (0), m_state(LINK_STATE::OFF), m_prop_delay_model(nullptr), m_prop_loss_model(nullptr)
{
  NS_LOG_FUNCTION (this);
  m_prop_delay_model = CreateObject<ns3::ConstantSpeedPropagationDelayModel>();
  m_prop_loss_model = CreateObject<ns3::RangePropagationLossModel>();
  m_prop_loss_model->SetAttribute("MaxRange", DoubleValue(3000.0));
}

AlohaChannel::~AlohaChannel(){}

void AlohaChannel::set_link_state(LINK_STATE st)
{
  m_state = st;
}

AlohaChannel::LINK_STATE AlohaChannel::GetLinkState() const
{
  return m_state;
}

uint8_t AlohaChannel::Send (Ptr<Packet> p, uint16_t protocol, Ptr<AlohaDevice> sender, Ptr<AlohaDevice> rcver)
{
  NS_LOG_FUNCTION (this << p << protocol << sender);

  if(std::find(m_devs.begin(), m_devs.end(), sender) == m_devs.end() ||
     std::find(m_devs.begin(), m_devs.end(), rcver) == m_devs.end())
  {
    return -1;
  }

  Mac8Address from = Mac8Address::ConvertFrom(sender->GetAddress ());
  Mac8Address to = Mac8Address::ConvertFrom(rcver->GetAddress ());

  const Ptr<ns3::MobilityModel> m_s = sender->GetNodeMobilityModel();
  ns3::Time delay(0);
  for(auto dev : m_devs)
  {
    if(dev == sender)
    {
      continue;
    }
    const Ptr<ns3::MobilityModel> m_r = dev->GetNodeMobilityModel();

    if(m_prop_loss_model && m_prop_loss_model->CalcRxPower(-10.0, m_s, m_r) == -1000)
    {
      continue;
    }

    if(m_prop_delay_model)
    {
      delay = m_prop_delay_model->GetDelay(m_s, m_r);
      Simulator::ScheduleWithContext (dev->GetNode()->GetId(),
                                      delay,
                                      &AlohaDevice::Receive,
                                      dev,
                                      p->Copy (),
                                      protocol,
                                      from,
                                      to);
    }

  }
  return 0;
}

bool AlohaChannel::Add (Ptr<AlohaDevice> dev)
{
  NS_LOG_FUNCTION (this << dev);
  bool rt = false;

  if(m_devs.size() == 256)
  {
    return false;
  }

  auto find_it = std::find(m_devs.begin(), m_devs.end(), dev);
  if(find_it == m_devs.end())
  {
    m_devs.push_back(dev);
    return true;
  }

  return false;
}

std::size_t AlohaChannel::GetNDevices (void) const
{
  return m_devs.size();
}

Ptr<NetDevice> AlohaChannel::GetDevice (std::size_t i) const
{
  if(i >= m_devs.size())
    return nullptr;
  return m_devs[i];
}

//
//      Device
//

AlohaDevice::~AlohaDevice(){}

TypeId
AlohaDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AlohaDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("Network")
    .AddConstructor<AlohaDevice> ()
    .AddAttribute ("DataRate",
                     "The default data rate for point to point links. Zero means infinite",
                     DataRateValue (DataRate ("0b/s")),
                     MakeDataRateAccessor (&AlohaDevice::m_bps),
                     MakeDataRateChecker ())
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been dropped "
                     "by the device during reception",
                     MakeTraceSourceAccessor (&AlohaDevice::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

AlohaDevice::AlohaDevice ()
  : m_channel (0),
    m_node (0),
    m_mtu (Aloha_MTU),
    m_ifIndex (0),
    m_state(State::IDLE)
{
  NS_LOG_FUNCTION (this);

  m_receiveErrorModel = CreateObject<ns3::RateErrorModel>();
  Ptr<ns3::RateErrorModel> ptr(m_receiveErrorModel->GetObject<ns3::RateErrorModel>());
  ptr->SetRate (0.01);
  ptr->SetUnit (RateErrorModel::ErrorUnit::ERROR_UNIT_PACKET);
  //ptr->Enable ();
  ptr->Disable();
}

void
AlohaDevice::SetChannel (Ptr<AlohaChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);
  if(channel->Add (this))
  {
    m_channel = channel;
  }
}

void
AlohaDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
AlohaDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}
uint32_t
AlohaDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}
Ptr<Channel>
AlohaDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}
void
AlohaDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac8Address::ConvertFrom (address);
}
Address
AlohaDevice::GetAddress (void) const
{
  //
  // Implicit conversion from Mac8Address to Address
  //
  NS_LOG_FUNCTION (this);
  return m_address;
}
bool
AlohaDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}
uint16_t
AlohaDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}
bool
AlohaDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}
void
AlohaDevice::AddLinkChangeCallback (Callback<void> callback)
{
 NS_LOG_FUNCTION (this << &callback);
 //m_linkChangeCallbacks.ConnectWithoutContext (callback);
}
bool
AlohaDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;

}
Address
AlohaDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac8Address (0xFF);
}
bool
AlohaDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}
Address
AlohaDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac8Address (0xFF);
}

Address AlohaDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac8Address (0xFF);
}

bool
AlohaDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return false;

}

bool
AlohaDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
AlohaDevice::Receive (ns3::Ptr<ns3::Packet> packet, uint16_t protocol, ns3::Mac8Address from, ns3::Mac8Address to)
{
  NS_LOG_FUNCTION (this << packet << protocol << from);

  if(m_state != State::IDLE)
  {
    m_phyRxDropTrace (packet);
    return;
  }

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
  {
    m_phyRxDropTrace (packet);
    return;
  }

  m_state = State::RX_STATE;
  Time rxTime = Time (0);
  if (m_bps > DataRate (0))
  {
    rxTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
  }
  Simulator::Schedule(rxTime, &AlohaDevice::receive_complete, this, packet, protocol, from, to);

}

void AlohaDevice::receive_complete(ns3::Ptr<ns3::Packet> packet, uint16_t protocol, ns3::Mac8Address from, ns3::Mac8Address to)
{
  m_state = State::IDLE;

  if( (to == m_address) || (to == Mac8Address::GetBroadcast()) )
  {
    m_rxCallback (this, packet, protocol, from);
  }
}

bool
AlohaDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);

  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
AlohaDevice::SendFrom (Ptr<Packet> p, const Address& source, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION (this << p << source << dest << protocolNumber);

    if( m_address != Mac8Address::ConvertFrom (source))
    {
        return false;
    }

    return true;
}

void
AlohaDevice::TransmitComplete (ns3::Ptr<ns3::Packet> p, uint16_t protocol)
{
  NS_LOG_FUNCTION (this);

  return;
}

Ptr<Node>
AlohaDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}
void
AlohaDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}
bool
AlohaDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}
void
AlohaDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void
AlohaDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_node = 0;

  NetDevice::DoDispose ();
}


void
AlohaDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
}

bool
AlohaDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

