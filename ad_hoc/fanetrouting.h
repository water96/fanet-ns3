#ifndef FANETROUTINGEXPERIMENT_H
#define FANETROUTINGEXPERIMENT_H

#include <stdint.h>
#include <map>


#include "experimentapp.h"
#include "adhoc.h"
#include "utils/tracers.h"
#include "nettraffiic.h"
#include "fanetmobility.h"


class FanetRoutingExperiment : public ExperimentApp
{
public:
  FanetRoutingExperiment();
  virtual ~FanetRoutingExperiment(); 
  const ExpResults& GetSimulationResults() const;

  static ns3::TypeId GetTypeId (void);

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
  void EnableLogComponent();

  //Results
  ExpResults m_results;
  //

  //callbacks
  std::vector<ns3::Ptr<WifiPhyTracer> > m_wifi_phy_tracers;
  std::vector<ns3::Ptr<WifiPhyStateTracer> > m_wifi_state_tracers;
  //============================

  //Usefull stuffs
  ns3::Ptr<ns3::PropagationLossModel> m_prop_model_ptr;
  //============================

  std::string m_CSVfileName;
  std::string m_mob_scenario;
  std::string m_rout_prot;
  double m_total_sim_time;
  std::string m_traffic_model;

  double m_txp; ///< distance
  std::string m_lossModelName; ///< loss model name
  std::string m_phyMode; ///< phy mode
  std::string m_traceFile; ///< trace file
  uint32_t m_nNodes; ///< number of nodes
  std::string m_trName; ///< trace file name
  double m_nodeSpeed; ///< in m/s

  ns3::NetDeviceContainer m_adhocTxDevices; ///< adhoc transmit devices
  ns3::Ipv4InterfaceContainer m_adhocTxInterfaces; ///< adhoc transmit interfaces
  int m_asciiTrace; ///< ascii trace
  int m_pcap; ///< PCAP

  RoutingHelper m_routingHelper; ///< routing helper

  int m_log; ///< log
  /// used to get consistent random numbers across scenarios
  int64_t m_streamIndex;
  ns3::NodeContainer m_adhocTxNodes; ///< adhoc transmit nodes

};

#endif // FANETROUTINGEXPERIMENT_H
