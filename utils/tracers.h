#ifndef TRACERS_H
#define TRACERS_H

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <list>
#include <algorithm>
#include <limits>

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
#include "ns3/wifi-module.h"
#include "ns3/ip-l4-protocol.h"

#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-classifier.h"
#include "ns3/ipv4-flow-classifier.h"

#include "utils/graph.h"
#include "utils/script.h"

typedef std::map<std::string, std::string> ExpResults;

class StatsCollector
{
public:
  StatsCollector ();

  uint32_t GetRxBytes ();
  uint32_t GetCumulativeRxBytes ();
  uint32_t GetRxPkts ();
  uint32_t GetCumulativeRxPkts ();

  void IncRxBytes (uint32_t rxBytes);
  void IncRxPkts ();
  void SetRxBytes (uint32_t rxBytes);
  void SetRxPkts (uint32_t rxPkts);

  uint32_t GetTxBytes ();
  uint32_t GetCumulativeTxBytes ();
  uint32_t GetTxPkts ();
  uint32_t GetCumulativeTxPkts ();

  void IncTxBytes (uint32_t txBytes);
  void IncTxPkts ();
  void SetTxBytes (uint32_t txBytes);
  void SetTxPkts (uint32_t txPkts);

private:
  uint32_t m_RxBytes; ///< reeive bytes
  uint32_t m_cumulativeRxBytes; ///< cumulative receive bytes
  uint32_t m_RxPkts; ///< receive packets
  uint32_t m_cumulativeRxPkts; ///< cumulative receive packets
  uint32_t m_TxBytes; ///< transmit bytes
  uint32_t m_cumulativeTxBytes; ///< cumulative transmit bytes
  uint32_t m_TxPkts; ///< transmit packets
  uint32_t m_cumulativeTxPkts; ///< cumulative transmit packets
};

class TracerBase;

class TracerBase : public ns3::Object
{
private:
  static ExpResults m_all;
protected:
  std::map<std::string, std::ofstream> m_cb_out_map;
  std::map<std::string, std::string>   m_cb_name_to_hdr_map;
  ExpResults m_res;

  //static
  const static std::string   m_delimeter;
  const static std::string   m_q;

  static void _insert_results_of_subtraces(const ExpResults& r)
  {
    for(const auto& kv : r)
    {
      NS_ASSERT(m_all.find(kv.first) == m_all.end());
    }
    m_all.insert(r.begin(), r.end());
  }

  //funcs
  virtual const ExpResults& _dump_results()
  {
    _insert_results_of_subtraces(m_res);
    return m_res;
  }
public:

  static ExpResults GetAllTraceResults()
  {
    return m_all;
  }

  static void ResetAllTraceResults()
  {
    m_all.clear();
  }


  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("TracerBase")
      .SetParent<ns3::Object> ()
      .AddConstructor<TracerBase> ();
    return tid;
  }

  TracerBase()
  {
    ns3::Simulator::ScheduleDestroy(&TracerBase::_dump_results, this);
  }

  virtual ~TracerBase()
  {
    for(auto& it : m_cb_out_map)
    {
      it.second.close();
    }

//    auto find_it = std::find(m_all_inst.begin(), m_all_inst.end(), this);
//    m_all_inst.erase(find_it);
  }

  void CreateOutput(const std::string& post_fix)
  {
    for(auto& it : m_cb_name_to_hdr_map)
    {
      m_cb_out_map.insert(std::make_pair(it.first, std::ofstream(it.first + "-" + post_fix)));
      if(it.second.length())
      {
        m_cb_out_map.at(it.first) << it.second << std::endl;
      }
    }
  }
};

class DistanceCalculatorAndTracer : public TracerBase
{
private:
  ns3::Ptr<ns3::MobilityModel> m_target;
public:

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("DistanceCalculatorAndTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<DistanceCalculatorAndTracer> ();
    return tid;
  }

  DistanceCalculatorAndTracer()
  {
    std::string ss;

    ss = "time" + m_delimeter +
         "px" + m_delimeter +
         "py" + m_delimeter +
         "pz" + m_delimeter +
         "v";
    m_cb_name_to_hdr_map.insert(std::make_pair("pos", ss));
  }
  ~DistanceCalculatorAndTracer() = default;

  void SetNodeMobilityModel(ns3::Ptr<ns3::MobilityModel> t)
  {
    m_target = t;
  }

  void DumperCb(double next)
  {
    std::ofstream& ofs = m_cb_out_map.at("pos");

    ns3::Vector pos = m_target->GetPosition (); // Get position
    ns3::Vector vel = m_target->GetVelocity (); // Get velocity

    ofs << ns3::Simulator::Now ().GetSeconds() << m_delimeter
                      << pos.x << m_delimeter
                      << pos.y << m_delimeter
                      << pos.z << m_delimeter
                      << vel.GetLength() << std::endl;
    ns3::Simulator::Schedule(ns3::Seconds(next), &DistanceCalculatorAndTracer::DumperCb, this, next);
  }
};

class AdjTracer : public TracerBase
{
private:
  std::map<ns3::Ptr<ns3::NetDevice>, std::set<ns3::Ptr<ns3::NetDevice> > > m_nodes_links;
  ns3::Time m_interval;
  ns3::EventId m_dump_event;
  uint32_t m_link_connect_cnter;
  uint32_t m_total_time;

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("AdjTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<AdjTracer> ();
    return tid;
  }

  AdjTracer() : m_interval(ns3::Seconds(1.0)), m_link_connect_cnter(0), m_total_time(0)
  {
    std::string ss;
    m_cb_name_to_hdr_map.insert(std::make_pair("node-deg", ss));
  }

  void SetNodeDevices(ns3::NetDeviceContainer& devs)
  {
    ns3::PointerValue tmp_ptr_val;
    for(auto dev_it = devs.Begin(); dev_it != devs.End(); dev_it++)
    {
      std::set<ns3::Ptr<ns3::NetDevice> > v;
      m_nodes_links.insert(std::make_pair(*dev_it, std::move(v)));

      ns3::Ptr<ns3::WifiNetDevice> dev = ns3::DynamicCast<ns3::WifiNetDevice>(*dev_it);
      ns3::Ptr<ns3::WifiPhy> phy = dev->GetPhy();
      std::string n_name = ns3::Names::FindName(dev->GetNode());
      phy->GetAttribute("State", tmp_ptr_val);
      ns3::Ptr<ns3::WifiPhyStateHelper> state_hlp = ns3::DynamicCast<ns3::WifiPhyStateHelper>(tmp_ptr_val.Get<ns3::WifiPhyStateHelper>());
   }

    std::string hdr = "time";

    for(auto& it : m_nodes_links)
    {
      std::string node_id = ns3::Names::FindName(it.first->GetNode());
      hdr += m_delimeter + "deg_" + node_id;
    }
    hdr += m_delimeter + "c";   //current connectivity

    m_cb_out_map.at("node-deg") << hdr << std::endl;
  }

  void SetDumpInterval(double s, double start_time)
  {
    m_interval = ns3::Seconds(s);
    m_dump_event.Cancel();
    m_dump_event = ns3::Simulator::Schedule(ns3::Seconds(s + start_time), &AdjTracer::DumperCb, this);
  }

  void DumperCb()
  {
    m_total_time++;
    std::ofstream& ofs = m_cb_out_map.at("node-deg");
    Graph<ns3::Ptr<ns3::NetDevice> > g;

    ofs << ns3::Simulator::Now ().GetSeconds();

    bool conn = true;
    for(auto& it : m_nodes_links)
    {
      g.AddNodeAndItsLinks(it.first, it.second);
      ofs << m_delimeter << it.second.size();
      if(it.second.empty())
      {
        conn = false;
      }
      it.second.clear();
    }

    if(conn && g.IsConnected())
    {
      m_link_connect_cnter++;
    }

    double c = (double)m_link_connect_cnter / (double)m_total_time;

    ofs << m_delimeter << c;
    ofs << std::endl;

    ns3::Simulator::Schedule(m_interval, &AdjTracer::DumperCb, this);
  }

  void RxCb(ns3::Ptr<ns3::NetDevice> rx_dev, const ns3::Address &from, ns3::Ptr<ns3::Packet> p)
  {
    auto find_it = std::find_if(m_nodes_links.begin(),
                                m_nodes_links.end(),
                                [from](const std::pair<ns3::Ptr<ns3::NetDevice>, std::set<ns3::Ptr<ns3::NetDevice> > >& t) -> bool {
                                                                                                                            return (from == t.first->GetAddress());
                                                                                                                         });
    if(find_it != m_nodes_links.end())
    {
      find_it->second.insert(rx_dev);
    }
  }

  virtual const ExpResults& _dump_results()
  {
    double c = (double)m_link_connect_cnter / (double)m_total_time;
    m_res.insert(std::make_pair("data_link_conn", std::to_string(c)));
    _insert_results_of_subtraces(m_res);
    return m_res;
  }
};

class FlowMonTracer : public TracerBase
{
private:
  ns3::Time m_interval;
  ns3::EventId m_dump_event;
  uint32_t m_total_time;

  ns3::Ptr<ns3::FlowMonitor> m_flowmon;
  ns3::Ptr<ns3::Ipv4FlowClassifier> m_classifier;

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("FlowMonTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<FlowMonTracer> ();
    return tid;
  }

  FlowMonTracer() : m_interval(ns3::Seconds(1.0)), m_total_time(0)
  {
    std::string ss;
    m_cb_name_to_hdr_map.insert(std::make_pair("flow-mon", ss));
  }

  void SetNodes(ns3::NodeContainer& c)
  {
    ns3::FlowMonitorHelper flowmon_hlp;
    m_flowmon = flowmon_hlp.Install(c);

    m_classifier = ns3::DynamicCast<ns3::Ipv4FlowClassifier>(flowmon_hlp.GetClassifier());
  }

  void SetDumpInterval(double s)
  {
    m_interval = ns3::Seconds(s);
    m_dump_event.Cancel();
    m_dump_event = ns3::Simulator::Schedule(m_interval, &FlowMonTracer::DumperCb, this);
  }

  void DumperCb()
  {
    m_flowmon->CheckForLostPackets();

    std::map<ns3::FlowId, ns3::FlowMonitor::FlowStats> stats = m_flowmon->GetFlowStats();



    ns3::Simulator::Schedule(m_interval, &FlowMonTracer::DumperCb, this);
  }

  virtual const ExpResults& _dump_results()
  {
    _insert_results_of_subtraces(m_res);
    return m_res;
  }
};

class AdjTxPowerTracer : public TracerBase
{
private:
  ns3::Time m_interval;
  ns3::EventId m_dump_event;
  uint32_t m_link_connect_cnter;
  uint32_t m_total_time;

  std::vector<std::pair<ns3::Ptr<ns3::WifiPhy>, ns3::Ptr<ns3::MobilityModel> > > m_nodes_mobility;
  ns3::Ptr<ns3::PropagationLossModel> m_prop_model;

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("AdjTxPowerTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<AdjTxPowerTracer> ();
    return tid;
  }

  AdjTxPowerTracer() : m_interval(ns3::Seconds(1.0)), m_link_connect_cnter(0), m_total_time(0)
  {
    std::string ss;
    m_cb_name_to_hdr_map.insert(std::make_pair("node-deg", ss));
  }

  void SetNodesAndDelayModel(ns3::NetDeviceContainer& d, ns3::Ptr<ns3::PropagationLossModel> p)
  {
    m_prop_model = p;

    m_nodes_mobility.clear();
    std::string hdr_str = "time";
    for(auto dev_it = d.Begin(); dev_it != d.End(); dev_it++)
    {
      ns3::Ptr<ns3::Node> node = (*dev_it)->GetNode();
      std::string n_name = ns3::Names::FindName(node);
      ns3::Ptr<ns3::MobilityModel> node_mob = node->GetObject<ns3::MobilityModel>();
      ns3::Ptr<ns3::WifiNetDevice> dev = ns3::DynamicCast<ns3::WifiNetDevice>(*dev_it);
      ns3::Ptr<ns3::WifiPhy> phy = dev->GetPhy();

      m_nodes_mobility.push_back(std::make_pair(phy, node_mob));

      hdr_str += m_delimeter + "deg_" + n_name;
    }
    hdr_str += m_delimeter + "c";

    m_cb_out_map.at("node-deg") << hdr_str << std::endl;
  }

  void SetDumpInterval(double s, double start_time)
  {
    m_interval = ns3::Seconds(s);
    m_dump_event.Cancel();
    m_dump_event = ns3::Simulator::Schedule(ns3::Seconds(s + start_time), &AdjTxPowerTracer::DumperCb, this);
  }

  void DumperCb()
  {
    m_total_time++;
    std::ofstream& ofs = m_cb_out_map.at("node-deg");

    ofs << ns3::Simulator::Now();

    Graph<ns3::Ptr<ns3::WifiPhy> > g;
    for(const auto& it : m_nodes_mobility)
    {
      uint16_t cnter = 0;
      for(const auto& inner : m_nodes_mobility)
      {
        if(it == inner)
        {
          continue;
        }

        if(m_prop_model->CalcRxPower(it.first->GetTxPowerStart(), it.second, inner.second) >= inner.first->GetRxSensitivity())
        {
          g.AddNodeAndItsLinks(it.first, inner.first);
          cnter++;
        }
      }
      ofs << m_delimeter << cnter;
    }

    if(g.IsConnected())
    {
      m_link_connect_cnter++;
    }

    double c = (double)m_link_connect_cnter / (double)m_total_time;
    ofs << m_delimeter << c << std::endl;

    ns3::Simulator::Schedule(m_interval, &AdjTxPowerTracer::DumperCb, this);
  }

  virtual const ExpResults& _dump_results()
  {
    double c = (double)m_link_connect_cnter / (double)m_total_time;
    m_res.insert(std::make_pair("tx_power_link_conn", std::to_string(c)));
    _insert_results_of_subtraces(m_res);
    return m_res;
  }
};

class NetworkAdjTracer : public TracerBase
{
private:
  ns3::Ipv4InterfaceContainer m_ifs;
  ns3::Time m_interval;
  ns3::EventId m_dump_event;
  std::map<ns3::Ipv4Address, std::string> m_addr_name_map;
public:
  struct RoutingEntry
  {
    ns3::Ipv4Address dest;
    ns3::Ipv4Address gateway;
    ns3::Ipv4Address ifs;

    std::vector<std::string> records;
  };

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("NetworkAdjTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<NetworkAdjTracer> ();
    return tid;
  }

  NetworkAdjTracer() : m_interval(ns3::Seconds(1.0))
  {
    std::string ss;
    m_cb_name_to_hdr_map.insert(std::make_pair("node-deg", ss));
  }

  void SetNodeIfces(ns3::Ipv4InterfaceContainer& ifs)
  {
    m_ifs = ifs;

    std::string hdr = "time";
    for(auto it = m_ifs.Begin(); it != m_ifs.End(); it++)
    {
      NS_ASSERT(it->first->GetRoutingProtocol() != nullptr);

      std::string node_id = ns3::Names::FindName(it->first->GetNetDevice(it->second)->GetNode());
      m_addr_name_map.insert(std::make_pair(it->first->GetAddress(it->second, 0).GetLocal(), node_id));
      hdr += m_delimeter + "adj_" + node_id;
    }

    m_cb_out_map.at("node-adj") << hdr << std::endl;
  }

  void SetDumpInterval(double s, double start_time)
  {
    m_interval = ns3::Seconds(s);
    m_dump_event.Cancel();
    m_dump_event = ns3::Simulator::Schedule(ns3::Seconds(s + start_time), &NetworkAdjTracer::DumperCb, this);
  }

  std::vector<RoutingEntry> ParseRoutingTable(const std::string& table)
  {
    std::vector<RoutingEntry> ret;
    std::size_t f = table.find("Destination");
    if(f == std::string::npos)
    {
      return ret;
    }

    std::string local = table.substr(f);
    std::vector<std::string> lines;
    utils::SplitString(local, "\n", lines);

    if(lines.empty())
    {
      return ret;
    }

    std::vector<std::string> colums;
    utils::SplitString(lines.front(), "\t", colums);
    if(colums.size() < 3)
    {
      return ret;
    }

    colums.clear();
    for(auto it = lines.begin() + 1; it != lines.end(); it++)
    {
      if(it->empty())
      {
        continue;
      }
      utils::SplitString(*it, "\t", colums);
      if(colums.size() < 3)
      {
        continue;
      }

      RoutingEntry re;
      re.dest.Set(utils::TrimString(colums[0]).c_str());
      re.gateway.Set(utils::TrimString(colums[1]).c_str());
//      re.ifs.Set(utils::TrimString(colums[2]).c_str());
      re.records.insert(re.records.begin(), colums.begin(), colums.end());

      ret.push_back(std::move(re));
      colums.clear();
    }
    return ret;
  }

  void DumperCb()
  {
    std::ofstream& ofs = m_cb_out_map.at("node-adj");

    ofs << ns3::Simulator::Now ().GetSeconds();

    for(auto it = m_ifs.Begin(); it != m_ifs.End(); it++)
    {
      std::stringstream ss;
      ns3::OutputStreamWrapper owr(&ss);
      ns3::Ptr<ns3::OutputStreamWrapper> owr_ptr(&owr);
      ns3::Ptr<ns3::Ipv4RoutingProtocol> routing = it->first->GetRoutingProtocol();
      routing->PrintRoutingTable(owr_ptr);

      std::vector<RoutingEntry> routs = ParseRoutingTable(ss.str());

      ns3::Ipv4InterfaceAddress local = it->first->GetAddress(it->second, 0);
      for(auto& it : routs)
      {
        //NS_ASSERT(it.ifs == local.GetLocal());
        if(it.dest != local.GetBroadcast())
        {
//          it.
        }
      }
    }

    ns3::Simulator::Schedule(m_interval, &NetworkAdjTracer::DumperCb, this);
  }

  virtual const ExpResults& _dump_results()
  {
    _insert_results_of_subtraces(m_res);
    return m_res;
  }
};

class AllNodesMobilityTracer : public TracerBase
{
private:
  std::map<std::string, ns3::Ptr<ns3::MobilityModel> > m_nodes_mobility;
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("AllNodesMobilityTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<AllNodesMobilityTracer> ();
    return tid;
  }

  AllNodesMobilityTracer()
  {
    std::string ss;

    ss = "time";

    for(auto& it : m_nodes_mobility)
    {
      ss = ss + m_delimeter + "px" + it.first
              + m_delimeter + "py" + it.first
              + m_delimeter + "pz" + it.first
              + m_delimeter + "v" + it.first;
    }

    m_cb_name_to_hdr_map.insert(std::make_pair("dump", ss));
  }
  void AddNodeMobilityModel(ns3::Ptr<ns3::MobilityModel> t, std::string n_id)
  {
    if(m_nodes_mobility.find(n_id) != m_nodes_mobility.end())
    {
      return;
    }

    std::string ss = m_delimeter + "px" + n_id
                  + m_delimeter + "py" + n_id
                  + m_delimeter + "pz" + n_id
                  + m_delimeter + "v" + n_id;

    m_cb_name_to_hdr_map.at("dump") += ss;

    m_nodes_mobility.insert(std::make_pair(n_id, t));
  }

  void DumperCb(double next)
  {
    std::ofstream& ofs = m_cb_out_map.at("dump");

    ofs << ns3::Simulator::Now ().GetSeconds();

    for(auto& it : m_nodes_mobility)
    {
      ns3::Vector pos = it.second->GetPosition(); // Get position
      ns3::Vector vel = it.second->GetVelocity (); // Get velocity

      ofs << m_delimeter << pos.x
                        << m_delimeter << pos.y
                        << m_delimeter << pos.z
                        << m_delimeter << vel.GetLength();
    }
    ofs << std::endl;

    ns3::Simulator::Schedule(ns3::Seconds(next), &AllNodesMobilityTracer::DumperCb, this, next);
  }
};

class QueCalcer : public TracerBase
{
private:
  std::map<ns3::Ptr<const ns3::Packet>, double > que;
  uint64_t m_total_send;

public:

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("QueCalcer")
      .SetParent<TracerBase> ()
      .AddConstructor<QueCalcer> ();
    return tid;
  }

  QueCalcer() : m_total_send(0)
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "diff" + m_delimeter
       + "total";

    m_cb_name_to_hdr_map.insert(std::make_pair("TraceTxQue", ss));
  }
  void TraceTxStart(ns3::Ptr< const ns3::Packet > packet)
  {
    que.insert (std::pair<ns3::Ptr<const ns3::Packet>, double>(packet, ns3::Simulator::Now ().GetSeconds ()));
  }

  void TraceTxEnd(ns3::Ptr< const ns3::Packet > packet)
  {
    std::ofstream& ofs = m_cb_out_map.at("TraceTxQue");

    double now = ns3::Simulator::Now ().GetSeconds ();
    double diff = now - que[packet];
    m_total_send += packet->GetSize ();
    ofs << now << m_delimeter << diff << m_delimeter << m_total_send << std::endl;
  }
};

class NodeMobTracer : public TracerBase
{
private:
  ns3::Ptr<ns3::MobilityModel> ap_mobility;

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("NodeMobTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<NodeMobTracer> ();
    return tid;
  }

  NodeMobTracer() : ap_mobility(nullptr)
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "px" + m_delimeter
       + "py" + m_delimeter
       + "v" + m_delimeter
       + "d" + m_delimeter
       + "apx" + m_delimeter
       + "apy";

    m_cb_name_to_hdr_map.insert(std::make_pair("CourseChange", ss));
  }
  void SetApMobilityModel(ns3::Ptr<ns3::MobilityModel> mob)
  {
    ap_mobility = mob;
  }
  void CourseChangeCb(ns3::Ptr< const ns3::MobilityModel > model)
  {
    std::ofstream& ofs = m_cb_out_map.at("CourseChange");

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << model->GetPosition ().x << m_delimeter
        << model->GetPosition ().y << m_delimeter
        << model->GetVelocity ().GetLength () << m_delimeter
        << model->GetDistanceFrom (ap_mobility) << m_delimeter
        << ap_mobility->GetPosition ().x << m_delimeter
        << ap_mobility->GetPosition ().y
        << std::endl;
  }
};

class PingTracer : public TracerBase
{
private:
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("PingTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<PingTracer> ();
    return tid;
  }

  PingTracer()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "rtt";

    m_cb_name_to_hdr_map.insert(std::make_pair("RttPing", ss));
  }

  void RttPingCb(ns3::Time rtt)
  {
    std::ofstream& ofs = m_cb_out_map.at("RttPing");

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << rtt.GetSeconds ()
        << std::endl;
  }
};

class ArpTracer : public TracerBase
{
private:
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("ArpTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<ArpTracer> ();
    return tid;
  }

  ArpTracer()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "p_id";

    m_cb_name_to_hdr_map.insert(std::make_pair("ArpDrop", ss));
  };

  void ArpDropCb(ns3::Ptr<const ns3::Packet> p)
  {
    std::ofstream& ofs = m_cb_out_map.at("ArpDrop");
    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid()
        << std::endl;
  }

};

class WifiPhyTracer : public TracerBase
{
private:
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("WifiPhyTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<WifiPhyTracer> ();
    return tid;
  }

  WifiPhyTracer()
  {
    std::string ss;

    ss = "time" + m_delimeter
       + "p_id" + m_delimeter
       + "reas";

    m_cb_name_to_hdr_map.insert(std::make_pair("WifiPhyDrop", ss));
  }

  void WifiPhyDropCb(ns3::Ptr<const ns3::Packet> p, ns3::WifiPhyRxfailureReason reason)
  {
    std::ofstream& ofs = m_cb_out_map.at("WifiPhyDrop");

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << static_cast<uint32_t>(reason)
        << std::endl;
  }
};

class WifiPhyStateTracer : public TracerBase
{
private:
  StatsCollector m_stats;
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("WifiPhyStateTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<WifiPhyStateTracer> ();
    return tid;
  }

  WifiPhyStateTracer()
  {
    std::string ss;

    ss = "Time" + m_delimeter + "pckt_id" + m_delimeter + "snr";
    m_cb_name_to_hdr_map.insert(std::make_pair("RxOkCb", ss));

    ss = "Time" + m_delimeter + "pckt_id" + m_delimeter + "snr";
    m_cb_name_to_hdr_map.insert(std::make_pair("RxError", ss));

    ss = "Time" + m_delimeter + "pckt_id" + m_delimeter + "size" + m_delimeter + "btr" + m_delimeter + "pw";
    m_cb_name_to_hdr_map.insert(std::make_pair("Tx", ss));
  }
  ~WifiPhyStateTracer() = default;

  void RxOkCb(ns3::Ptr<const ns3::Packet> p, double d, ns3::WifiMode m, ns3::WifiPreamble pr)
  {
    std::ofstream& ofs = m_cb_out_map.at("RxOkCb");
    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << d
        << std::endl;
  }

  void RxErrorCb(ns3::Ptr<const ns3::Packet> p, double d)
  {
    std::ofstream& ofs = m_cb_out_map.at("RxError");
    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << d
        << std::endl;
  }

  void TxCb(ns3::Ptr<const ns3::Packet> p, ns3::WifiMode m, ns3::WifiPreamble pr, uint8_t hz)
  {
    std::ofstream& ofs = m_cb_out_map.at("Tx");

    static double time_stamp = 0.0;
    double now = ns3::Simulator::Now ().GetSeconds();

    double btr = 0.0;
    if(now)
    {
      btr = ((double)m_stats.GetTxBytes() * 8.0) / (now - time_stamp);
    }
    m_stats.SetTxPkts(0);
    m_stats.SetTxBytes(0);
    m_stats.IncTxPkts();
    m_stats.IncTxBytes(p->GetSize());

    ofs << now << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << btr << m_delimeter
        << static_cast<uint32_t>(hz)
        << std::endl;
  }
};

class Ipv4L3ProtocolTracer : public TracerBase
{
private:
  //Static
  static StatsCollector m_stats;
  static std::ofstream m_stats_out;
  //
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("Ipv4L3ProtocolTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<Ipv4L3ProtocolTracer> ();
    return tid;
  }

  Ipv4L3ProtocolTracer()
  {
    std::string ss;

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst";
    m_cb_name_to_hdr_map.insert(std::make_pair("Tx", ss));

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst";
    m_cb_name_to_hdr_map.insert(std::make_pair("Rx", ss));

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst" + m_delimeter +
         "reas";
    m_cb_name_to_hdr_map.insert(std::make_pair("Drop", ss));

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst";
    m_cb_name_to_hdr_map.insert(std::make_pair("UnicastForward", ss));

    ss = "t" + m_delimeter +
         "pid" + m_delimeter +
         "s" + m_delimeter +
         "src" + m_delimeter +
         "dst";
    m_cb_name_to_hdr_map.insert(std::make_pair("LocalDeliver", ss));
  }
  ~Ipv4L3ProtocolTracer() = default;

  //==========================================
  void TxCb(ns3::Ptr<const ns3::Packet> p, ns3::Ptr<ns3::Ipv4> ip,  uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("Tx");

    ns3::Ipv4Header ip_hdr;
    p->PeekHeader(ip_hdr);

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << std::endl;

    m_stats.IncTxPkts();
    m_stats.IncTxBytes(p->GetSize());
  }

  void RxCb(ns3::Ptr<const ns3::Packet> p, ns3::Ptr<ns3::Ipv4> ip,  uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("Rx");

    ns3::Ipv4Header ip_hdr;
    p->PeekHeader(ip_hdr);

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << std::endl;

    m_stats.IncRxPkts();
    m_stats.IncRxBytes(p->GetSize());
  }

  void DropCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, ns3::Ipv4L3Protocol::DropReason r, ns3::Ptr<ns3::Ipv4> ip, uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("Drop");

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q << m_delimeter
        << static_cast<uint32_t>(r)
        << std::endl;
  }

  void UnicastForwardCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("UnicastForward");

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << std::endl;

  }

  void LocalDeliverCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    std::ofstream& ofs = m_cb_out_map.at("LocalDeliver");

    std::stringstream ssource;
    ip_hdr.GetSource().Print(ssource);

    std::stringstream sdst;
    ip_hdr.GetDestination().Print(sdst);

    ofs << ns3::Simulator::Now ().GetSeconds () << m_delimeter
        << p->GetUid() << m_delimeter
        << p->GetSize() << m_delimeter
        << m_q << ssource.str() << m_q << m_delimeter
        << m_q << sdst.str() << m_q
        << std::endl;
  }

};

class IPv4AllStatsTracer : public TracerBase
{
private:
  StatsCollector m_stats;
  uint32_t m_dropped, m_forwarded;
  ns3::Time m_interval;
  ns3::EventId m_dump_event;

  std::map<std::string, uint32_t> m_ip_forward_cnter_map;
  //
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("IPv4AllStatsTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<IPv4AllStatsTracer> ();
    return tid;
  }

  IPv4AllStatsTracer() : m_interval(ns3::Seconds(1.0)), m_dropped(0), m_forwarded(0)
  {
    std::stringstream ss;

    ss << "t" << TracerBase::m_delimeter
                << "tx_pckt" << TracerBase::m_delimeter
                << "tx_bytes" << TracerBase::m_delimeter
                << "rx_pckt" << TracerBase::m_delimeter
                << "rx_bytes" << TracerBase::m_delimeter
                << "pdr_all";
    m_cb_name_to_hdr_map.insert(std::make_pair("dump-stats", ss.str()));

    std::string s;
    s = "node_id" + TracerBase::m_delimeter
      + "forw_pckts";
    m_cb_name_to_hdr_map.insert(std::make_pair("ip-forwards-by-node", s));

    ns3::Simulator::ScheduleDestroy(&IPv4AllStatsTracer::DumpIpForwardsStats, this);
  }

  void SetDumpInterval(double s, double start_time)
  {
    m_interval = ns3::Seconds(s);
    m_dump_event.Cancel();
    m_dump_event = ns3::Simulator::Schedule(ns3::Seconds(s + start_time), &IPv4AllStatsTracer::StatsDumper, this);
  }

  void AddCollectingStatsFrom(ns3::Ptr<ns3::Ipv4> ip, const std::string n_id)
  {
    ip->TraceConnectWithoutContext("Tx", MakeCallback(&IPv4AllStatsTracer::TxCb, this));
    ip->TraceConnectWithoutContext("Rx", MakeCallback(&IPv4AllStatsTracer::RxCb, this));
    ip->TraceConnectWithoutContext("Drop", MakeCallback(&IPv4AllStatsTracer::DropCb, this));
    ip->TraceConnectWithoutContext("UnicastForward", MakeCallback(&IPv4AllStatsTracer::UnicastForwardCb, this));
    ip->TraceConnectWithoutContext("LocalDeliver", MakeCallback(&IPv4AllStatsTracer::LocalDeliverCb, this));

    m_ip_forward_cnter_map.insert(std::make_pair(n_id, 0));
    ip->TraceConnect("UnicastForward", n_id, MakeCallback(&IPv4AllStatsTracer::ForwardCnterCb, this));
  }

  void StatsDumper()
  {
    uint32_t rx = m_stats.GetRxPkts();
    uint32_t tx = m_stats.GetTxPkts();
    double pdr = 1.0;
    if(tx != 0)
    {
       pdr = (double)rx / (double)tx;
    }
    m_cb_out_map.at("dump-stats") << ns3::Simulator::Now().GetSeconds() << TracerBase::m_delimeter
                                  << tx << TracerBase::m_delimeter
                                  << m_stats.GetTxBytes() << TracerBase::m_delimeter
                                  << rx << TracerBase::m_delimeter
                                  << m_stats.GetRxBytes() << TracerBase::m_delimeter
                                  << pdr
                                  << std::endl;

    ns3::Simulator::Schedule(m_interval, &IPv4AllStatsTracer::StatsDumper, this);
  }

  void DumpIpForwardsStats()
  {
    for(auto& it : m_ip_forward_cnter_map)
    {
      m_cb_out_map.at("ip-forwards-by-node") << m_q << it.first << m_q << m_delimeter
                                             << it.second
                                             << std::endl;
    }
  }

  //Callbacks
  void TxCb(ns3::Ptr<const ns3::Packet> p, ns3::Ptr<ns3::Ipv4> ip,  uint32_t ifs)
  {
    m_stats.IncTxPkts();
    m_stats.IncTxBytes(p->GetSize());
  }
  void RxCb(ns3::Ptr<const ns3::Packet> p, ns3::Ptr<ns3::Ipv4> ip,  uint32_t ifs)
  {

  }
  void DropCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, ns3::Ipv4L3Protocol::DropReason r, ns3::Ptr<ns3::Ipv4> ip, uint32_t ifs)
  {
    m_dropped++;
  }
  void UnicastForwardCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    m_forwarded++;
  }
  void LocalDeliverCb(const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    m_stats.IncRxPkts();
    m_stats.IncRxBytes(p->GetSize());
  }
  void ForwardCnterCb(std::string n_id, const ns3::Ipv4Header & ip_hdr, ns3::Ptr<const ns3::Packet> p, uint32_t ifs)
  {
    m_ip_forward_cnter_map.at(n_id)++;
  }
  //

  virtual const ExpResults& _dump_results()
  {
    m_res.insert(std::make_pair("ip_tx", std::to_string(m_stats.GetCumulativeTxPkts())));
    m_res.insert(std::make_pair("ip_rx", std::to_string(m_stats.GetCumulativeRxPkts())));
    m_res.insert(std::make_pair("ip_drop", std::to_string(m_dropped)));
    m_res.insert(std::make_pair("ip_forward", std::to_string(m_forwarded)));

    _insert_results_of_subtraces(m_res);
    return m_res;
  }
};

class PDRAndThroughputMetr : public TracerBase
{
private:
  StatsCollector m_stats;
  ns3::Time m_interval;
  ns3::Time m_total;
  double m_pdr_val;
  ns3::EventId m_dump_event;
  uint32_t m_total_time_cnter;
  uint32_t m_connect_cnter;

  std::map<ns3::Address, StatsCollector> m_tx_rx_per_node;
  std::vector<std::pair<uint32_t, ns3::Address> > m_tx_addr_pid;

public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("PDRAndThroughputMetr")
      .SetParent<TracerBase> ()
      .AddConstructor<PDRAndThroughputMetr> ();
    return tid;
  }

  PDRAndThroughputMetr() : m_interval(ns3::Seconds(1.0)), m_connect_cnter(0), m_total_time_cnter(0)
  {
    std::string ss;
    ss = "time" + m_delimeter + "tx_all" + m_delimeter + "rx_all" + m_delimeter + "thrput" + m_delimeter + "pdr";
    m_cb_name_to_hdr_map.insert(std::make_pair("DumpPDR", ss));

    ss = "time" + m_delimeter + "tx" + m_delimeter + "rx" + m_delimeter + "с";
    m_cb_name_to_hdr_map.insert(std::make_pair("DumpConnectivity", ss));

    ss = "node_id" + m_delimeter + "tx" + m_delimeter + "rx";
    m_cb_name_to_hdr_map.insert(std::make_pair("tx_rx_per_node", ss));

  }

  void SetSocketsPair(const std::vector<std::pair<ns3::Ptr<ns3::Socket>, ns3::Ptr<ns3::Socket> > >& s)
  {
    for(const auto& i : s)
    {
      ns3::Address addr;
      i.second->GetSockName(addr);
      m_tx_rx_per_node.insert(std::make_pair(addr, StatsCollector()));
    }
  }

  void TxCb(ns3::Ptr<ns3::Packet> p, ns3::Ptr<const ns3::Socket> src)
  {
    m_stats.IncTxPkts();
    m_stats.IncTxBytes(p->GetSize());

    ns3::Address addr;
    src->GetPeerName(addr);

    m_tx_rx_per_node[addr].IncTxPkts();

    //Store in vector all txed packets and remote address
    m_tx_addr_pid.push_back(std::make_pair(p->GetUid(), addr));
  }

  void RxCb(ns3::Ptr<ns3::Packet> p, ns3::Ptr<const ns3::Socket> local)
  {
    uint32_t RxBytes = p->GetSize ();
    m_stats.IncRxBytes (RxBytes);
    m_stats.IncRxPkts ();

    ns3::Address addr;
    local->GetSockName(addr);

    m_tx_rx_per_node[addr].IncRxPkts();

    uint32_t p_id_rx = p->GetUid();
    auto find_it = std::find_if(m_tx_addr_pid.begin(),
                                m_tx_addr_pid.end(),
                                [p_id_rx](const std::pair<uint32_t, ns3::Address>& pr ) -> bool
                                {
                                  return (p_id_rx == pr.first);
                                });

    if(find_it != m_tx_addr_pid.end())
    {
      m_tx_addr_pid.erase(find_it);
    }
  }

  void SetDumpInterval(double sec, double start_time)
  {
    m_interval = ns3::Seconds(sec);
    m_dump_event.Cancel();
    m_dump_event = ns3::Simulator::Schedule(ns3::Seconds(sec + start_time), &PDRAndThroughputMetr::DumpStatistics, this);
  }

  void DumpTxRxPerNode()
  {

    for(auto& it : m_tx_rx_per_node)
    {
      ns3::Ipv4Address addr = ns3::InetSocketAddress::ConvertFrom(it.first).GetIpv4();
      std::stringstream n_addr;
      addr.Print(n_addr);
      m_cb_out_map.at("tx_rx_per_node") << m_q << n_addr.str() << m_q << m_delimeter
                                        << it.second.GetCumulativeTxPkts() << m_delimeter
                                        << it.second.GetCumulativeRxPkts()
                                        << std::endl;
    }
  }

  void DumpStatistics()
  {
    double now = ns3::Simulator::Now ().GetSeconds ();
    m_total_time_cnter++;
    uint32_t bytes_rcv_interval = m_stats.GetRxBytes ();    //aggregated
    uint32_t packetsReceivedAll =  m_stats.GetCumulativeRxPkts();
    uint32_t packetsTransmitedAll =  m_stats.GetCumulativeTxPkts();

    m_pdr_val = 1.0;
    if(packetsTransmitedAll)
    {
      m_pdr_val = (double)packetsReceivedAll / (double)packetsTransmitedAll;
    }

    m_cb_out_map.at("DumpPDR") << now << m_delimeter
                                << packetsTransmitedAll << m_delimeter
                                << packetsReceivedAll << m_delimeter
                                << (bytes_rcv_interval * 8.0) / m_interval.GetSeconds() << m_delimeter
                                << m_pdr_val
                                << std::endl;

    double pdr = 1.0;
    packetsReceivedAll =  m_stats.GetRxPkts();
    packetsTransmitedAll =  m_stats.GetTxPkts();

    if(m_tx_addr_pid.empty())
    {
      m_connect_cnter++;
    }
    m_tx_addr_pid.clear();

    pdr = (double)m_connect_cnter / (double)m_total_time_cnter;

    m_cb_out_map.at("DumpConnectivity") << now << m_delimeter
                                        << packetsTransmitedAll << m_delimeter
                                        << packetsReceivedAll << m_delimeter
                                        << pdr
                                        << std::endl;

    //reset interval stats
    m_stats.SetRxBytes (0);
    m_stats.SetRxPkts (0);
    m_stats.SetTxBytes (0);
    m_stats.SetTxPkts (0);

    ns3::Simulator::Schedule(m_interval, &PDRAndThroughputMetr::DumpStatistics, this);
  }

  virtual const ExpResults& _dump_results()
  {
    DumpTxRxPerNode();

    double conn = (double)m_connect_cnter / (double)m_total_time_cnter;
    m_res.insert(std::make_pair("udp_conn", std::to_string(conn)));
    m_res.insert(std::make_pair("udp_pdr", std::to_string(m_pdr_val)));
    m_res.insert(std::make_pair("udp_tx", std::to_string(m_stats.GetCumulativeTxPkts())));
    m_res.insert(std::make_pair("udp_rx", std::to_string(m_stats.GetCumulativeRxPkts())));

    _insert_results_of_subtraces(m_res);
    return m_res;
  }

};

#endif // TRACERS_H

