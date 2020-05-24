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
class NetTrafficCreator;

class NetTrafficCreator
{
private:
  std::map<std::string, NetTraffic*> m_models;
  std::string m_default;

  NetTraffic* m_inst;

  NetTrafficCreator();
public:
  NetTrafficCreator(const NetTrafficCreator&) = delete;
  NetTrafficCreator& operator=(const NetTrafficCreator&) = delete;
  ~NetTrafficCreator();

  static NetTrafficCreator& Inst();

  std::string GetModelsList();
  std::string GetDefaultModel();
  int CreateNetTrafficModel(const std::string& model, uint64_t stream_index, double total_sim_time);
  NetTraffic& GetNetTrafficModel();
  void DestroyNetTrafficModel();
};

class NetTraffic
{
protected:
  virtual NetTraffic* Clone() const = 0;
private:
  uint64_t m_index;
  double m_total_time;

public:
  NetTraffic();
  virtual ~NetTraffic();
  NetTraffic& operator=(const NetTraffic&) = delete;
  NetTraffic(const NetTraffic&) = delete;

  virtual int Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c) = 0;
  virtual int ConfigreTracing() = 0;

  uint64_t GetStreamIndex() const;
  double GetTotalSimTime() const;

  friend class NetTrafficCreator;
};

class PingTraffic : public NetTraffic
{
private:
  std::vector<ns3::Ptr<PingTracer> > m_ping_trace;
  virtual NetTraffic* Clone() const override;
public:
  PingTraffic();
  virtual ~PingTraffic();
  virtual int Install(ns3::NodeContainer& nc, ns3::NetDeviceContainer& devs, ns3::Ipv4InterfaceContainer& ip_c) override;
  virtual int ConfigreTracing() override;
};


class UdpCbrTraffic : public NetTraffic
{
private:
  ns3::Ptr<PDRAndThroughputMetr> m_pckt_tracer;
  uint16_t m_pckt_size;
  double m_interval;

  void GenerateTraffic(ns3::Ptr<ns3::Socket> socket);

  virtual NetTraffic* Clone() const override;
public:
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
