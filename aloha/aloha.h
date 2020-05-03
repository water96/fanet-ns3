#ifndef ALOHA_H
#define ALOHA_H

#include <map>
#include <queue>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/node.h"

#include "ns3/channel.h"
#include "ns3/net-device.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h"

class AlohaDevice;

class AlohaMacHeader : public ns3::Header
{
public:
  enum class frame_kind
  {
    data = 0u,
    ack,
    nak
  };

  frame_kind m_kind;
  uint8_t m_seq;
  uint8_t m_ack_seq;

public:

  AlohaMacHeader (){}
  virtual ~AlohaMacHeader (){}

  static ns3::TypeId GetTypeId (void);
  virtual ns3::TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (ns3::Buffer::Iterator start) const;
  virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;
};


class AlohaChannel : public ns3::Channel
{
public:
  enum class LINK_STATE
  {
    OFF = 0u,
    IDLE,
    BUSY
  };

private:
  ns3::Time m_delay; //!< The assigned speed-of-light delay of the channel
  std::vector<ns3::Ptr<AlohaDevice>> m_devs; //!< devices connected by the channel

  LINK_STATE m_state;
  ns3::Ptr<ns3::PropagationDelayModel> m_prop_delay_model;
  ns3::Ptr<ns3::PropagationLossModel> m_prop_loss_model;

protected:
  virtual void set_link_state(LINK_STATE st);

public:

  static ns3::TypeId GetTypeId (void);
  AlohaChannel ();
  virtual ~AlohaChannel();

  virtual uint8_t Send (ns3::Ptr<ns3::Packet> p, uint16_t protocol, ns3::Ptr<AlohaDevice> sender, ns3::Ptr<AlohaDevice> rcver);

  virtual bool Add (ns3::Ptr<AlohaDevice> dev);

  virtual AlohaChannel::LINK_STATE GetLinkState() const;

  // inherited from ns3::Channel
  virtual std::size_t GetNDevices (void) const override;

  virtual ns3::Ptr<ns3::NetDevice> GetDevice (std::size_t i) const override;
};

class AlohaDevice : public ns3::NetDevice
{
private:

  enum class State
  {
    IDLE,
    RX_STATE,
    TX_STATE
  };

  ns3::Ptr<AlohaChannel> m_channel; //!< the channel the device is connected to
  ns3::NetDevice::ReceiveCallback m_rxCallback; //!< Receive callback
  ns3::Ptr<ns3::Node> m_node; //!< Node this ns3::NetDevice is associated to
  uint16_t m_mtu;   //!< MTU
  uint32_t m_ifIndex; //!< Interface index
  ns3::Mac8Address m_address; //!< MAC address
  ns3::Ptr<ns3::ErrorModel> m_receiveErrorModel; //!< Receive error model.
  ns3::TracedCallback<ns3::Ptr<const ns3::Packet> > m_phyRxDropTrace;

  void TransmitComplete (ns3::Ptr<ns3::Packet> p, uint16_t protocol);

  bool m_linkUp; //!< Flag indicating whether or not the link is up
  ns3::DataRate m_bps; //!< The device nominal Data rate. Zero means infinite
  ns3::EventId TransmitCompleteEvent; //!< the Tx Complete event

  ns3::Ptr<ns3::MobilityModel> m_mob;
  double m_tx_power;
  double m_rx_sense;

  void receive_complete(ns3::Ptr<ns3::Packet> packet, uint16_t protocol, ns3::Mac8Address from, ns3::Mac8Address to);

  State m_state;

public:

  const static uint16_t Aloha_MTU = 128;

  static ns3::TypeId GetTypeId (void);
  AlohaDevice ();
  virtual ~AlohaDevice();

  ns3::Ptr<const ns3::MobilityModel> GetNodeMobilityModel() const;

  void Receive (ns3::Ptr<ns3::Packet> packet, uint16_t protocol, ns3::Mac8Address from, ns3::Mac8Address to);

  void SetChannel (ns3::Ptr<AlohaChannel> channel);

  void SetReceiveErrorModel (ns3::Ptr<ns3::ErrorModel> em);

  // inherited from ns3::NetDevice base class.
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual ns3::Ptr<ns3::Channel> GetChannel (void) const;
  virtual void SetAddress (ns3::Address address);
  virtual ns3::Address GetAddress (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (ns3::Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual ns3::Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual ns3::Address GetMulticast (ns3::Ipv4Address multicastGroup) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  virtual bool Send (ns3::Ptr<ns3::Packet> packet, const ns3::Address& dest, uint16_t protocolNumber);
  virtual bool SendFrom (ns3::Ptr<ns3::Packet> packet, const ns3::Address& source, const ns3::Address& dest, uint16_t protocolNumber);
  virtual ns3::Ptr<ns3::Node> GetNode (void) const;
  virtual void SetNode (ns3::Ptr<ns3::Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (ns3::NetDevice::ReceiveCallback cb);

  virtual ns3::Address GetMulticast (ns3::Ipv6Address addr) const;

  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;

protected:
  virtual void DoDispose (void);
};




#endif // ALOHA_H
