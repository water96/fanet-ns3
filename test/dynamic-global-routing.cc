#include "ns3/core-module.h"
#include "ns3/netanim-module.h"
#include "ns3/paparazzi-mobility-model.h"
#include "ns3/stats-module.h"
#include "ns3/mobility-module.h"
#include "ns3/simulator.h"
#include <iostream>
#include <sstream>
#include "ns3/random-variable-stream.h"
#include <cstdlib>

#include "utils/tracers.h"

using namespace ns3;

std::string GetScenarioName(uint32_t n, double s)
{
  std::stringstream ss;
  ss << "RPGM_" << n << "_" << s;
  return ss.str();
}

std::string CreateBmCommandString(const std::string& bm_path, uint32_t n, double s, double t, const ns3::Vector3D& area)
{
  std::stringstream ss;
  double r;
  if(area.x > area.y)
  {
    r = area.y * 0.1;
  }
  else
  {
    r = area.x * 0.1;
  }
  ss << bm_path << "/bm -f RPGM_" << n << "_" << s << " RPGM "
     << "-d " << t << " -i 100 -n " << n << " -x " << area.x << " -y " << area.y << " -J 2D "
     << " -h " << (s + 10) << " -l " << (s - 10) << " -p 0 "
     << "-a " << (double)(n / 3.0) << " -c 0.75 -r " << r << " -s " << (double)(n / 2.0);

  ss << "; " << bm_path << "/bm NSFile -f RPGM_" << n << "_" << s << " -b";
  return ss.str();
}

int main (int argc, char *argv[]) {

	CommandLine cmd;
	cmd.Parse(argc,argv);

	NodeContainer c;
	c.Create (12);

	AllNodesMobilityTracer tr;
	uint32_t cnter = 1;

	double z_start = 2000;
	Ptr<ns3::UniformRandomVariable> var = CreateObject<ns3::UniformRandomVariable>();
	var->SetAttribute ("Min", DoubleValue (z_start - 300));
	var->SetAttribute ("Max", DoubleValue (z_start + 300));

	var->SetStream(2);

	char* bm_tool_path = std::getenv("BM_TOOL");
	if(bm_tool_path == nullptr)
	{
	  std::cerr << "Can not find bm tool environment variable!\n Set BM_TOOL var\n";
	  return 1;
	}

	std::string cmd_str = CreateBmCommandString(bm_tool_path, 12, 140.0, 200.0, ns3::Vector3D(20000.0, 20000.0, 0));

	int res = std::system(cmd_str.c_str());

	if(res != 0)
	{
	  std::cerr << "Error to create trace file!\n";
	  return 1;
	}

	std::string filename = GetScenarioName(12, 140.0) + ".ns_movements";
	Ns2MobilityHelper ns2m(filename);
	ns2m.SetZCoord(var);
	ns2m.Install(c.Begin(), c.End());


	for (auto n = c.Begin(); n != c.End(); n++, cnter++)
	{

          Ptr<MobilityModel> node_mob = (*n)->GetObject<MobilityModel>();
          if(node_mob)
          {
            //add node to all mob trace
            tr.AddNodeMobilityModel(node_mob, std::to_string(cnter));
          }
        }

        tr.CreateOutput("all-nodes-mobs.csv");
        ns3::Simulator::Schedule(Time(0.0), &AllNodesMobilityTracer::DumperCb, &tr, 1.0);

        AnimationInterface anim ("rpgm.xml");

        Simulator::Stop (Seconds (200));

        Simulator::Run ();

	Simulator::Destroy ();
	return 0;
}
