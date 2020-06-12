/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/gpsr-module.h"
#include "ns3/pagpsr-module.h"
#include "ns3/mmgpsr-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-client.h"
#include "ns3/udp-echo-helper.h"
#include <iostream>
#include <cmath>
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "string.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/applications-module.h"
#include <string>
#include <sys/stat.h>
#include <iomanip>
#include <fstream>

#include "ns3/wave-net-device.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/wave-helper.h"


using namespace ns3;

uint32_t totalHello = 0;
uint32_t rx_drop = 0;
uint32_t tx_drop = 0;

static void PRxDrop (Ptr<const Packet> m_phyRxDropTrace, ns3::WifiPhyRxfailureReason){

  //std::cout<<"PhyRXDrop at == " << Simulator::Now ().GetSeconds () << "\n";
  rx_drop += 1;
}

static void PTxDrop (Ptr<const Packet> m_phyTxDropTrace){

  //std::cout<<"PhyTXDrop at == " << Simulator::Now ().GetSeconds () << "\n";
  tx_drop +=1;
}


void handler (int arg0)
{
  std::cout << "The simulation is now at: "<<arg0 <<" seconds" << std::endl;
}


class GpsrExample
{
public:
  GpsrExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

  void writeToFile(uint32_t lostPackets, uint32_t totalTx, uint32_t totalRx, double hopCount,double count, double delay);

private:

  uint32_t size;

  std::string phyMode;
  /// Width of the Node Grid
  uint32_t gridWidth;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  // seed to generate random numbers
  uint32_t seed;
  std::string path;
  Time entrylifetime;
  uint32_t headersize;
  uint32_t packetsize;
  bool newfile;
  std::string algorithm;
  int sourcenode;
  int destinationNode;
  int speed;
  uint32_t drift;
  uint32_t nPairs;


  NodeContainer AllNodes;
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  //\}

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();

};

int main (int argc, char **argv)
{
  GpsrExample test;
  if (! test.Configure(argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
GpsrExample::GpsrExample () :
  // Number of Nodes
  size (30),
  // Grid Width
  gridWidth(10),
  // Distance between nodes
  step (100),
  // Simulation time
  totalTime (200),
  // Generate capture files for each node
  pcap (false),
  //seed to generate random numbers
  seed (1394),
  path("outputs/"),
  packetsize(512),
  algorithm("pagpsr"),
  newfile(true),
  speed(15),
  drift(0),
  nPairs(15),
  phyMode ("OfdmRate3MbpsBW10MHz")

{
}


bool
GpsrExample::Configure (int argc, char **argv)
{

  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("step", "Grid step, m", step);
  cmd.AddValue ("seed", "seed value", seed);
  cmd.AddValue ("path", "path of results file", path);
  cmd.AddValue ("conn", "number of conections", nPairs);
  cmd.AddValue ("algorithm", "routing algorithm", algorithm);
  cmd.AddValue ("newfile", "create new result file", newfile);
  cmd.AddValue ("speed", "node speed", speed);

// disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("500"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));
  cmd.Parse (argc, argv);
  return true;
}

void
GpsrExample::writeToFile(uint32_t lostPackets, uint32_t totalTx, uint32_t totalRx, double hopCount,double count, double delay){

  struct stat buf;
    std::string outputfile = "results/"+algorithm+"_results/pairs"+std::to_string(nPairs)+"/"+algorithm+std::to_string(size)+"_results.txt";
  //std::string outputfile = "results/pairs"+std::to_string(nPairs)+"/"+algorithm+std::to_string(size)+"_results.txt";
  //std::string outputfile = "results/teste.txt";
  int exist = stat(outputfile.c_str(), &buf);

  std::ofstream outfile;
  outfile.open(outputfile.c_str(), std::ios::app);

  if (outfile.is_open())
  {
    std::cout << "Output operation successfully performed1\n";
  }
  else
  {
    std::cout << "Error opening file";
  }

  if (newfile == true){

   std::ofstream outfile;
   outfile.open(outputfile.c_str(), std::ios::trunc);

   if (outfile.is_open())
   {
     std::cout << "Output operation successfully performed2\n";
   }
   else
   {
     std::cout << "Error opening file";
   }

    outfile<< "Seed\t"<<"LostPackets\t"<<"totalTx\t"<<"totalRx\t"<<"PDR (%)\t"<<"HopCount\t"<<"Delay (ms)\t"<<"PhyRxDrop\t"<<"PhyTxDrop\n";
    outfile.flush();
    exist = 1;
   }

  if (exist == -1){
        outfile<< "Seed\t"<<"LostPackets\t"<<"totalTx\t"<<"totalRx\t"<<"PDR (%)\t"<<"HopCount\t"<<"Delay (ms)\t"<<"PhyRxDrop\t"<<"PhyTxDrop\n";
        outfile.flush();
  }

  // write to outfile
  outfile <<seed<<"\t"; //Lost packets
  outfile.flush();
  outfile <<lostPackets<<"\t"; //Lost packets
  outfile.flush();
  outfile <<(double)totalTx<<"\t"; //Total transmited packets
  outfile.flush();
  outfile <<(double)totalRx<<"\t"; //Total received packets
  outfile.flush();

  if (count == 0){

    outfile <<0<<"\t"; //PDR
    outfile.flush();
    outfile <<0<<"\t"; //Mean Hop Count
    outfile.flush();
    outfile <<0<<"\t"; //Mean Delay (ms)
    outfile.flush();

  }else{

    outfile <<std::fixed<<std::setprecision(2)<< ((double)totalRx/(double)totalTx)*100.0<<"\t"; //PDR
    outfile.flush();
    outfile <<std::fixed<<std::setprecision(2)<< hopCount/count<<"\t"; //Mean Hop Count
    outfile.flush();
    outfile <<std::fixed<<std::setprecision(2)<< delay/count * 1000<<"\t"; //Mean Delay (ms)
    outfile.flush();

  }

  outfile <<rx_drop<<"\t";
  outfile.flush();
  outfile <<tx_drop<<"\n";
  outfile.flush();

  outfile.close();


}

void
GpsrExample::Run ()
{


  SeedManager::SetSeed(seed);

  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

if (algorithm=="gpsr"){
  std::cout<<"Using GPSR algorithm...\n";
  GpsrHelper gpsr;
  gpsr.Install ();

}else{
  if (algorithm=="mmgpsr"){
    std::cout<<"Using MMGPSR algorithm...\n";
    MMGpsrHelper mmgpsr;
    mmgpsr.Install ();
  }else{
    std::cout<<"Using PA-GPSR algorithm...\n";
    PAGpsrHelper pagpsr;
    pagpsr.Install ();

  }
}
  std::cout << "Starting simulation for " << totalTime << " s ...\n";
  std::cout << "Starting simulation for speed " << speed << " ms ...\n";

  for (int i=1; i<=totalTime; i++){
    if (i % 10 == 0) // at every 10s
      Simulator::Schedule(Seconds(i), &handler, i);
  }


  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();

 //Print per flow statstics
monitor->CheckForLostPackets();
Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
std::map<FlowId,FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
uint32_t lostPackets = 0;
uint32_t totalRx=0;
uint32_t totalTx=0;
double delay=0;
double count=0;
double hopCount=0;


for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i= stats.begin();i!=stats.end();++i)
{
  Ipv4FlowClassifier::FiveTuple t=classifier->FindFlow(i->first);

   if(i->second.rxPackets!=0)
     {
        totalRx+=i->second.rxPackets;
        totalTx+=i->second.txPackets;
        hopCount+=(i->second.timesForwarded/i->second.rxPackets+1); // this hopCount is not suitable for PA-GPSR because of packet duplication. For PA-GPSR HopCount calculation we need to use the IP TTL field instead (not implemented yet). This hopCount can be helpful to calculate network yield though.
        delay+=(i->second.delaySum.GetSeconds()/i->second.rxPackets);
        count++;
        lostPackets += i->second.lostPackets;
    }
}

writeToFile(lostPackets, totalTx, totalRx, hopCount, count, delay);


monitor->SerializeToXmlFile("gpsr.flowmon",true,true);

  Simulator::Destroy ();
}

void
GpsrExample::Report (std::ostream &)
{
}

void
GpsrExample::CreateNodes ()
{

 std::cout << "Creating  " << (unsigned)size << " nodes .."<<" with "<<nPairs<<" pairs..." <<"\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
     {
       std::ostringstream os;
       os << "node-" << i;
       Names::Add (os.str (), nodes.Get (i));

     }

  std::string m_traceFile;
  m_traceFile = "/home/aleksey/work/PA-GPSR/results/tclFiles/speed/"+std::to_string(speed)+"/newNs2mobility"+std::to_string(size)+".tcl";
  Ns2MobilityHelper mobility= Ns2MobilityHelper (m_traceFile);//for tracefile
  mobility.Install ();

  AllNodes.Add(nodes);

}

void
GpsrExample::CreateDevices ()
{

Wifi80211pHelper wifi = Wifi80211pHelper::Default ();

YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
YansWifiChannelHelper wifiChannel;

wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel",
				  "MaxRange", DoubleValue (250.0));
wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel",
								    "SystemLoss", DoubleValue(1),
								    "HeightAboveZ", DoubleValue(1.5));


  // For range near 250m
  wifiPhy.Set ("TxPowerStart", DoubleValue(33));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(33));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(0));
  wifiPhy.Set ("RxGain", DoubleValue(0));
  wifiPhy.Set ("RxSensitivity", DoubleValue(-61.8));
  wifiPhy.Set ("CcaEdThreshold", DoubleValue(-64.8));

 wifiPhy.SetChannel (wifiChannel.Create ());

NqosWaveMacHelper wifiMac = NqosWaveMacHelper::Default ();

 wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue(phyMode),
                                   "ControlMode",StringValue(phyMode));
devices = wifi.Install (wifiPhy, wifiMac, AllNodes);

wifiPhy.EnablePcapAll ("wifi-pcap");

}


void
GpsrExample::InstallInternetStack ()
{
  InternetStackHelper stack;
  std::string prefix = "10.0.0.";
  std::string sufix_dst = std::to_string(destinationNode+1); //the nodes numbers starts from zero, so we need to add 1 for the IP addresses
  std::string sufix_source = std::to_string(sourcenode+1);
  std::string dst_s = prefix+sufix_dst;
  std::string source_s = prefix+sufix_source;
  char const *dst_c = dst_s.c_str();
  char const *source_c = source_s.c_str();

if (algorithm=="gpsr"){
  GpsrHelper gpsr;
  stack.SetRoutingHelper (gpsr);
  stack.Install (AllNodes);
}else{
  if(algorithm=="pagpsr"){
  PAGpsrHelper pagpsr;
  stack.SetRoutingHelper (pagpsr);
  stack.Install (AllNodes);
  }
  else{
    MMGpsrHelper mmgpsr;
    stack.SetRoutingHelper (mmgpsr);
    stack.Install (AllNodes);
  }
}


  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces = address.Assign (devices);

}
void
GpsrExample::InstallApplications ()
{

  uint16_t port = 9;  // well-known echo port number

  uint32_t maxPacketCount = 10000; // number of packets to transmit
  Time interPacketInterval = Seconds (0.2); // interval between packet transmissions
  UdpEchoServerHelper server1 (port);
  ApplicationContainer apps;

  srand(276); //same seed to random pairs be the same at all simulations, can be any number (use 276 to reproduce our results)

  int source;
  int dest;
  std::vector<std::pair<int,int>> source_dest_pairs;
  std::vector<std::pair<int,int>>::iterator it;
  bool exist;
  float start_app;
  for (uint32_t i = 0; i < nPairs ; i++){
        source = 0;
        dest = 0;
        exist = true;
        start_app = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        while((source == dest) || exist == true){

             source = rand() % size;
             dest = rand() % size;
             it = std::find(source_dest_pairs.begin(), source_dest_pairs.end(), std::make_pair(source,dest));
             if (it != source_dest_pairs.end()){
                exist = true;
             }else{
                source_dest_pairs.push_back(std::make_pair(source,dest));
                exist = false;
             }
        }
        UdpEchoClientHelper client (interfaces.GetAddress (dest), port);
        client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
        client.SetAttribute ("Interval", TimeValue (interPacketInterval));
        client.SetAttribute ("PacketSize", UintegerValue (packetsize));
        apps = client.Install (nodes.Get (source));
        apps.Start (Seconds (start_app));
        apps.Stop (Seconds (totalTime-0.1));
  }

  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PRxDrop));
  Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PTxDrop));
}
