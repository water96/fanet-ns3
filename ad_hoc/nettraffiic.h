#ifndef NETTRAFFIIC_H
#define NETTRAFFIIC_H

#include <vector>
#include <string>
#include "ns3/core-module.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-interface-container.h"
#include "utils/tracers.h"
#include "ns3/traffic-control-layer.h"

class NetTraffic;

class NetTraffic : public ns3::Object
{
protected:
  virtual NetTraffic* Clone() const = 0;
private:
  uint64_t m_index;
  double m_total_time;

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("NetTraffic")
      .SetParent<ns3::Object> ();
    return tid;
  }

  NetTraffic();
  virtual ~NetTraffic();
  NetTraffic& operator=(const NetTraffic&) = delete;
  NetTraffic(const NetTraffic&) = delete;

  virtual void SetStreamIndex(uint32_t index);
  virtual void SetSimulationTime(double t);
  virtual uint64_t GetStreamIndex() const;
  virtual double GetTotalSimTime() const;
  virtual int Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c) = 0;
  virtual int ConfigreTracing() = 0;

  friend class NetTrafficCreator;
};

class PingTraffic : public NetTraffic
{
private:
  std::vector<ns3::Ptr<PingTracer> > m_ping_trace;
  virtual NetTraffic* Clone() const override;
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("Ping")
      .SetParent<NetTraffic> ()
      .AddConstructor<PingTraffic> ();
    return tid;
  }

  PingTraffic();
  virtual ~PingTraffic();
  virtual int Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c) override;
  virtual int ConfigreTracing() override;
};


class UdpCbrTraffic : public NetTraffic
{
private:
  ns3::Ptr<PDRAndThroughputMetr> m_pckt_tracer;
  std::vector<std::pair<ns3::Ptr<ns3::Socket>, ns3::Ptr<ns3::Socket> > > m_pair_sockets;
  uint16_t m_pckt_size;
  double m_interval;

  void GenerateTraffic(ns3::Ptr<ns3::Socket> socket);

  virtual NetTraffic* Clone() const override;
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("UDP_CBR")
      .SetParent<NetTraffic> ()
      .AddConstructor<UdpCbrTraffic> ();
    return tid;
  }
  UdpCbrTraffic();
  virtual ~UdpCbrTraffic();
  virtual int Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c) override;
  virtual int ConfigreTracing() override;
  //to callbacks
  void RxCb(ns3::Ptr<ns3::Socket> socket);
};

class L3NodesDiscoverTraffic : public NetTraffic
{
private:
  ns3::Ptr<AdjTracer> m_adj_tracer;
  ns3::NetDeviceContainer m_devs;
  uint16_t m_pckt_size;
  double m_interval;
  void GenerateTraffic(ns3::Ptr<ns3::Socket> socket);

  virtual NetTraffic* Clone() const override;

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("L3ND")
      .SetParent<NetTraffic> ()
      .AddConstructor<L3NodesDiscoverTraffic> ();
    return tid;
  }

  static const uint16_t PROT_NUMBER;

  L3NodesDiscoverTraffic();
  virtual ~L3NodesDiscoverTraffic();
  virtual int Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c) override;
  virtual int ConfigreTracing() override;


  void ReceiveCb(ns3::Ptr<ns3::NetDevice> device, ns3::Ptr<const ns3::Packet> p, uint16_t protocol, const ns3::Address &from,
                 const ns3::Address &to, ns3::NetDevice::PacketType packetType );
  void TransmitCb(ns3::Ptr<ns3::NetDevice> dev, ns3::Ptr<ns3::TrafficControlLayer> tc, ns3::Ipv4InterfaceAddress ip_addr);
};

#endif // NETTRAFFIIC_H
