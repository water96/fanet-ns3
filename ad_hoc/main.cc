#include <iostream>
#include <fstream>
#include <functional>
#include <numeric>

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

#include "utils/tracers.h"
#include "fanetrouting.h"
#include "utils/script.h"

using namespace ns3;

class ScalarsByNodesTracer : public TracerBase
{
private:
  std::string m_cached_hdr;
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("ScalarsByNodesTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<ScalarsByNodesTracer> ();
    return tid;
  }

  ScalarsByNodesTracer()
  {
    std::string ss;
    m_cb_name_to_hdr_map.insert(std::make_pair("all", ss));
  }

  void Dump(uint32_t n, const ExpResults& r)
  {
    std::string hdr, vals;
    hdr = "nodes" + m_delimeter;
    vals = std::to_string(n) + m_delimeter;

    for(const auto& it : r)
    {
       hdr += it.first + m_delimeter;
       vals += it.second + m_delimeter;
    }
    hdr = hdr.substr(0, hdr.length() - m_delimeter.length());
    vals = vals.substr(0, vals.length() - m_delimeter.length());

    if(m_cached_hdr.length() == 0)
    {
      m_cached_hdr = hdr;
    }
    else
    {
      NS_ASSERT(m_cached_hdr == hdr);
    }

    m_cb_out_map.at("all") << hdr << std::endl << vals;
  }
};

class ScalarsTracer : public TracerBase
{
private:
public:
  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("ScalarsTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<ScalarsTracer> ();
    return tid;
  }

  ScalarsTracer()
  {
    std::string ss;

    ss = "";
    m_cb_name_to_hdr_map.insert(std::make_pair("scalars", ss));
  }

  void Dump(const ExpResults& r)
  {
    m_cb_out_map.at("scalars").seekp(0);

    std::string hdr, vals;

    for(const auto& it : r)
    {
       hdr += it.first + m_delimeter;
       vals += it.second + m_delimeter;
    }
    hdr = hdr.substr(0, hdr.length() - m_delimeter.length());
    vals = vals.substr(0, vals.length() - m_delimeter.length());
    m_cb_out_map.at("scalars") << hdr << std::endl << vals;
  }
};

class ScalarsStatisticTracer : public TracerBase
{
private:
  std::vector<ExpResults> m_scalars;
  uint16_t m_params;
public:

  ExpResults m_mean, m_median, m_devation;

  static ns3::TypeId GetTypeId (void)
  {
    static ns3::TypeId tid = ns3::TypeId ("ScalarsStatisticTracer")
      .SetParent<TracerBase> ()
      .AddConstructor<ScalarsStatisticTracer> ();
    return tid;
  }

  ScalarsStatisticTracer()
  {
    std::string ss;

    m_cb_name_to_hdr_map.insert(std::make_pair("vector", ss));

    m_cb_name_to_hdr_map.insert(std::make_pair("stats", ss));
  }

  void Reserv(uint16_t num_of_tests)
  {
    m_scalars.reserve(num_of_tests);
  }

  void Add(const ExpResults& c)
  {
    if(m_scalars.empty())
    {
      m_params = c.size();
    }
    else
    {
      NS_ASSERT(c.size() == m_params);
    }
    m_scalars.push_back(c);
  }

  void Dump()
  {
    std::string hdr, vals;
    std::map<std::string, std::vector<double> > numeric_vals;
    std::vector<std::string> output_lines(m_scalars.size());

    //Check
    ExpResults first_line = m_scalars.front();
    for(const auto& kv : first_line)
    {
      std::string k = kv.first;
      bool add_to_vec = true;

      if(k.find("_str") != std::string::npos)
      {
        //this is str value
        add_to_vec = false;
      }
      else
      {
        std::vector<double> vec;
        vec.reserve(m_scalars.size());
        numeric_vals.insert(std::make_pair(k, vec));
      }

      hdr += k + m_delimeter;

      auto str_line = output_lines.begin();
      for(const auto& lines : m_scalars)
      {
        auto find_it = lines.find(k);
        NS_ASSERT(find_it != lines.end());

        str_line->append(find_it->second + m_delimeter);
        vals += find_it->second + m_delimeter;
        if(add_to_vec)
        {
          numeric_vals.at(find_it->first).push_back(std::stod(find_it->second));
        }
        str_line++;
      }
    }

    hdr = hdr.substr(0, hdr.length() - m_delimeter.length());
    m_cb_out_map.at("vector") << hdr
                              << std::endl;

    for(auto& s : output_lines)
    {
      m_cb_out_map.at("vector") << s.substr(0, s.length() - m_delimeter.length())
                                << std::endl;
    }


    hdr.clear();
    vals.clear();
    for(const auto& nums : numeric_vals)
    {
      double res;
      std::string str_val, new_name;
      //mean
      res = std::accumulate(nums.second.begin(), nums.second.end(), 0.0);
      res /= nums.second.size();
      new_name = "mean_" + nums.first;
      str_val = std::to_string(res);
      m_mean.insert(std::make_pair(new_name, str_val));
      hdr.append(new_name + m_delimeter);
      vals.append(str_val + m_delimeter);
      //median
      //deviation
    }
    hdr = hdr.substr(0, hdr.length() - m_delimeter.length());
    vals = vals.substr(0, vals.length() - m_delimeter.length());

    m_cb_out_map.at("stats") << hdr << std::endl
                             << vals
                             << std::endl;
  }
};

int main(int argc, char **argv)
{
  //=====================================
  //LogComponentEnableAll (LogLevel::LOG_ALL);
  //=====================================

  const uint32_t start_nodes = 2;
  const uint32_t end_nodes = 16;
  const uint32_t step = 2;

  double total_time = 200.0;
  std::string mobility = "RWP";
  std::string routing = "AODV";
  std::string traffic = "UDP_CBR";
  double speed = 200.0;
  uint32_t seed_num = 5;

  //Go to output dir
  std::string out_dir = Script::GetEnv("PWD");
  if(out_dir.length())
  {
    out_dir += "/out";
    Script::MkDir(out_dir);
  }
  Script::ChDir(out_dir);
  //=====================================

  //Tracing
  ScalarsByNodesTracer tr;
  tr.CreateOutput("scalars-by-nodes.csv");
  //=====================================

  for(uint32_t n = start_nodes; n <= end_nodes; n+=step)
  {
    //Create exp dir
    std::string exp_name = mobility + "-" +
                           routing + "-" +
                           std::to_string(n) + "n-" +
                           std::to_string(total_time) + "s";
    std::cout << "Experiment: " << exp_name << std::endl;
    std::string exp_dir = out_dir + "/" + exp_name;
    Script::MkDir(exp_dir);
    Script::ChDir(exp_dir);
    std::cout << "Go to " << exp_dir << std::endl;

    ScalarsStatisticTracer statistics;
    statistics.CreateOutput("scalars.csv");
    statistics.Reserv(seed_num);

    for(uint32_t s = 0; s < seed_num; s++)
    {
      std::cout << "\tSeed: " << s << std::endl;
      std::string seed_dir = exp_dir + "/seed-" + std::to_string(s);
      Script::MkDir(seed_dir);
      Script::ChDir(seed_dir);

      ScalarsTracer scalar_of_one_impl;
      scalar_of_one_impl.CreateOutput("impl.csv");

      Ptr<FanetRoutingExperiment> exp = CreateObject<FanetRoutingExperiment>();

      exp->SetAttribute("nodes", UintegerValue(n));
      exp->SetAttribute("stream", UintegerValue(0));
      exp->SetAttribute("time", DoubleValue(total_time));
      exp->SetAttribute("mobility", StringValue(mobility));
      exp->SetAttribute("traffic", StringValue(traffic));
      exp->SetAttribute("routing", StringValue(routing));
      exp->SetAttribute("speed", DoubleValue(speed));
      exp->Simulate(argc, argv);

      const ExpResults& r = exp->GetSimulationResults();

      scalar_of_one_impl.Dump(r);
      statistics.Add(r);
    }
    //Скедуилть ивент в конструкторе базовом на вызов виртуального метода по дампу результатов в общий вектор результатов
    statistics.Dump();
    tr.Dump(n, statistics.m_mean);

    std::cout << "Done with " << exp_name << "\n=============================" << std::endl;
  }

  return 0;
}
