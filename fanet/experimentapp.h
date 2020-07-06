#ifndef EXPERIMENTAPP_H
#define EXPERIMENTAPP_H

#include "ns3/core-module.h"

class ExperimentApp : public ns3::Object
{
public:
  static ns3::TypeId GetTypeId (void);

  /**
   * \brief Constructor
   * \return none
   */
  ExperimentApp ();

  /**
   * \brief Destructor
   * \return none
   */
  virtual ~ExperimentApp ();

  /**
   * \brief Enacts simulation of an ns-3 wifi application
   * \param argc program arguments count
   * \param argv program arguments
   * \return none
   */
  void Simulate (int argc, char **argv);

protected:
  /**
   * \brief Sets default attribute values
   * \return none
   */
  virtual void SetDefaultAttributeValues ();

  /**
   * \brief Process command line arguments
   * \param argc program arguments count
   * \param argv program arguments
   * \return none
   */
  virtual void ParseCommandLineArguments (int argc, char **argv);

  /**
   * \brief Configure nodes
   * \return none
   */
  virtual void ConfigureNodes ();

  /**
   * \brief Configure channels
   * \return none
   */
  virtual void ConfigureChannels ();

  /**
   * \brief Configure devices
   * \return none
   */
  virtual void ConfigureDevices ();

  /**
   * \brief Configure mobility
   * \return none
   */
  virtual void ConfigureMobility ();

  /**
   * \brief Configure applications
   * \return none
   */
  virtual void ConfigureApplications ();

  /**
   * \brief Configure tracing
   * \return none
   */
  virtual void ConfigureTracing ();

  /**
   * \brief Run the simulation
   * \return none
   */
  virtual void RunSimulation ();

  /**
   * \brief Process outputs
   * \return none
   */
  virtual void ProcessOutputs ();
};

#endif // EXPERIMENTAPP_H
