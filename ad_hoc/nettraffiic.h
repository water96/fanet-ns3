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
  ExpResults m_res;
  virtual NetTraffic* Clone() const = 0;
private:
  uint64_t m_index;
  double m_total_time;

public:
  NetTraffic();
  virtual ~NetTraffic();
  NetTraffic& operator=(const NetTraffic&) = delete;
  NetTraffic(const NetTraffic&) = delete;

  virtual int Install(ns3::NodeContainer& nc, ns3::Ipv4InterfaceContainer& ip_c) = 0;
  virtual ExpResults& GetResultsMap();

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

  virtual NetTraffic* Clone() const override;
public:
  UdpCbrTraffic();
  virtual ~UdpCbrTraffic();
  ExpResults& GetResultsMap() override;
  virtual int Install(ns3::NodeContainer& nc, ns3::Ipv4InterfaceContainer& ip_c) override;
  //to callbacks
  void RxCb(ns3::Ptr<ns3::Socket> socket);
};


#endif // NETTRAFFIIC_H
