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

class NetTraffic;

class NetTrafficCreator
{
private:
  std::vector<std::string> m_models;
  NetTraffic* m_inst;

public:

  enum NetTrafficClasses
  {
    PING_TRAFFIC = 0u,
    UDP_CBR,
  };

  NetTrafficCreator();
  ~NetTrafficCreator();

  int Create(NetTrafficClasses c, uint64_t stream_index, double total_sim_time);
  NetTraffic& Inst();

};

class NetTraffic
{
protected:
  ExpResults m_res;
private:
  uint64_t m_index;
  double m_total_time;
public:
  NetTraffic();
  virtual ~NetTraffic();
  virtual int Install(ns3::NodeContainer& nc, ns3::Ipv4InterfaceContainer& ip_c) = 0;
  virtual ExpResults& GetResultsMap();

  uint64_t GetStreamIndex() const;
  double GetTotalSimTime() const;

  friend class NetTrafficCreator;
};

class PingTraffic : public NetTraffic
{
private:
  std::vector<PingTracer*> m_ping_trace;
public:
  PingTraffic();
  virtual ~PingTraffic();
  virtual int Install(ns3::NodeContainer& nc, ns3::Ipv4InterfaceContainer& ip_c) override;

};


class UdpCbrTraffic : public NetTraffic
{
private:
  PDRAndThroughputMetr m_pckt_tracer;
  StatsCollector m_stats;
  uint16_t m_pckt_size;
  double m_interval;

  void GenerateTraffic(ns3::Ptr<ns3::Socket> socket);

public:
  UdpCbrTraffic();
  virtual ~UdpCbrTraffic();
  ExpResults& GetResultsMap() override;
  virtual int Install(ns3::NodeContainer& nc, ns3::Ipv4InterfaceContainer& ip_c) override;
  //to callbacks
  void RxCb(ns3::Ptr<ns3::Socket> socket);
};


#endif // NETTRAFFIIC_H
