#ifndef FANETROUTINGEXPERIMENT_H
#define FANETROUTINGEXPERIMENT_H

#include <stdint.h>
#include <map>


#include "experimentapp.h"
#include "adhoc.h"
#include "utils/tracers.h"

class FanetRoutingExperiment : public ExperimentApp
{
public:
  FanetRoutingExperiment();
  virtual ~FanetRoutingExperiment();

protected:
  virtual void SetDefaultAttributeValues () override;
  virtual void ParseCommandLineArguments (int argc, char **argv) override;
  virtual void ConfigureNodes () override;
  virtual void ConfigureChannels () override;
  virtual void ConfigureDevices () override;
  virtual void ConfigureMobility () override;
  virtual void ConfigureApplications () override;
  virtual void ConfigureTracing () override;
  virtual void RunSimulation () override;
  virtual void ProcessOutputs () override;

private:

  //callbacks

  std::vector<DistanceCalculatorAndTracer*> m_dist_calc_trace;
  std::vector<PingTracer*> m_ping_trace;
  std::vector<ArpTracer*> m_arp_tracers;
  std::vector<WifiPhyTracer*> m_wifi_phy_tracers;
  std::vector<WifiPhyStateTracer*> m_wifi_state_tracers;

  //============================

  void EnableLogComponent();
  void CheckThroughput();


  std::string m_CSVfileName;
  uint16_t m_port; ///< port
  uint16_t m_nsinks;
  RoutingHelper::ROUTING_PROTOCOL m_rout_prot;
  double m_total_sim_time;

  double m_txp; ///< distance
  bool m_traceMobility; ///< trace mobility
  uint32_t m_protocol; ///< protocol

  std::string m_lossModelName; ///< loss model name

  std::string m_phyMode; ///< phy mode

  std::string m_traceFile; ///< trace file
  std::string m_logFile; ///< log file
  uint32_t m_nNodes; ///< number of nodes
  std::string m_rate; ///< rate
  std::string m_trName; ///< trace file name
  int m_nodeSpeed; ///< in m/s
  int m_nodePause; ///< in s
  uint32_t m_wavePacketSize; ///< bytes
  double m_waveInterval; ///< seconds
  int m_verbose; ///< verbose
  std::ofstream m_os; ///< output stream
  ns3::NetDeviceContainer m_adhocTxDevices; ///< adhoc transmit devices
  ns3::Ipv4InterfaceContainer m_adhocTxInterfaces; ///< adhoc transmit interfaces
  bool m_print_routingTables; ///< routing tables
  int m_asciiTrace; ///< ascii trace
  int m_pcap; ///< PCAP

  ns3::Ptr<RoutingHelper> m_routingHelper; ///< routing helper

  int m_log; ///< log
  /// used to get consistent random numbers across scenarios
  int64_t m_streamIndex;
  ns3::NodeContainer m_adhocTxNodes; ///< adhoc transmit nodes

};

#endif // FANETROUTINGEXPERIMENT_H
