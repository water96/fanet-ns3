#ifndef FANETMOBILITY_H
#define FANETMOBILITY_H

#include "ns3/mobility-module.h"
#include "ns3/node-container.h"
#include "ns3/core-module.h"
#include "utils/tracers.h"

#include <map>

class FanetMobility;

class FanetMobilityCreator
{
private:
  std::map<std::string, FanetMobility*> m_models;
  std::string m_default;
  FanetMobility* m_inst;

  FanetMobilityCreator();
public:
  FanetMobilityCreator(const FanetMobilityCreator&) = delete;
  FanetMobilityCreator& operator=(const FanetMobilityCreator&) = delete;

  ~FanetMobilityCreator();

  static FanetMobilityCreator& Inst();

  std::string GetModelsList();
  std::string GetDefaultModel();

  int CreateMobilityModel(const std::string& model);
  FanetMobility& GetMobilityModel();
  void DestroyMobilityModel();
};

class FanetMobility
{
protected:
  ns3::Time m_sim_time;
  ns3::NodeContainer m_nodes;
  ns3::Vector3D m_area;
  uint32_t m_sindex;
  double m_speed;

  ns3::Ptr<AllNodesMobilityTracer> m_all_nodes_mobility_trace;

  virtual FanetMobility* Clone() const = 0;
  virtual ns3::Ptr<ns3::PositionAllocator> CreateInitialPositionAllocater();

public:
  FanetMobility& operator=(const FanetMobility&) = delete;
  FanetMobility(const FanetMobility&) = delete;

  FanetMobility();
  virtual ~FanetMobility();
  virtual void SetStreamIndex(uint32_t index);
  virtual void SetSimulationTime(const ns3::Time& t);
  virtual void SetMobilityAreaAndSpeed(const ns3::Vector3D& vec, double speed);
  virtual void Install(const ns3::NodeContainer& c) = 0;
  virtual void ConfigureMobilityTracing();

  friend class FanetMobilityCreator;
};

//=====================================//

class RWPFANETMobility : public FanetMobility
{
private:

  virtual FanetMobility* Clone() const override;
public:
  RWPFANETMobility() = default;
  virtual ~RWPFANETMobility();
  void Install(const ns3::NodeContainer& c) override;
};

//=====================================//

class GMFANETMobility : public FanetMobility
{
private:

  virtual FanetMobility* Clone() const override;
public:
  GMFANETMobility() = default;
  virtual ~GMFANETMobility();
  void Install(const ns3::NodeContainer& c) override;
};

//=====================================//

class CONSTFANETMobility : public FanetMobility
{
private:

  virtual FanetMobility* Clone() const override;
public:
  CONSTFANETMobility() = default;
  virtual ~CONSTFANETMobility();
  void Install(const ns3::NodeContainer& c) override;
};


#endif // FANETMOBILITY_H
