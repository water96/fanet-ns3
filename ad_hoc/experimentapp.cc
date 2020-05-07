#include "experimentapp.h"

ExperimentApp::ExperimentApp ()
{
}

ExperimentApp::~ExperimentApp ()
{
}

void
ExperimentApp::Simulate (int argc, char **argv)
{
  // Simulator Program Flow:
  // (source:  NS-3 Annual Meeting, May, 2014, session 2 slides 6, 28)
  //   (HandleProgramInputs:)
  //   SetDefaultAttributeValues
  //   ParseCommandLineArguments
  //   (ConfigureTopology:)
  //   ConfigureNodes
  //   ConfigureChannels
  //   ConfigureDevices
  //   ConfigureMobility
  //   ConfigureApplications
  //     e.g AddInternetStackToNodes
  //         ConfigureIpAddressingAndRouting
  //         configureSendMessages
  //   ConfigureTracing
  //   RunSimulation
  //   ProcessOutputs

  SetDefaultAttributeValues ();
  ParseCommandLineArguments (argc, argv);
  ConfigureNodes ();
  ConfigureChannels ();
  ConfigureDevices ();
  ConfigureMobility ();
  ConfigureApplications ();
  ConfigureTracing ();
  RunSimulation ();
  ProcessOutputs ();
}

void
ExperimentApp::SetDefaultAttributeValues ()
{
}

void
ExperimentApp::ParseCommandLineArguments (int argc, char **argv)
{
}

void
ExperimentApp::ConfigureNodes ()
{
}

void
ExperimentApp::ConfigureChannels ()
{
}

void
ExperimentApp::ConfigureDevices ()
{
}

void
ExperimentApp::ConfigureMobility ()
{
}

void
ExperimentApp::ConfigureApplications ()
{
}

void
ExperimentApp::ConfigureTracing ()
{
}

void
ExperimentApp::RunSimulation ()
{
}

void
ExperimentApp::ProcessOutputs ()
{
}
