#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <array>

//ns3
#include "ns3/random-variable-stream.h"

//local
#include "utils/script.h"

using namespace ns3;

int main(int argc, char *argv[])
{
  Script::Instance ().SetScriptName ( Script::GetBaseName (argv[0]) );
  Script::Instance ().CreateOutputDir (Script::DEFAULT_OUTPUT_DIR);

  std::cout << "Scriptname: " << Script::Instance ().GetScriptName () << std::endl;
  std::cout << "Output dir: " << Script::Instance ().GetOutputDir () << std::endl;

  ns3::Ptr<ns3::UniformRandomVariable> x = CreateObject<UniformRandomVariable>();

  std::string out = Script::Instance ().GetOutputDir () + "/random.dat";

  std::fstream fs(out, std::ios_base::out);

  if (!fs.is_open ())
    {
      return 1;
    }

  std::map<int, int> hist;

  for(uint32_t i = 0; i < 100000; i++)
  {
    auto v = x->GetInteger (0, 100);
    fs << v << std::endl;

    auto find_it = hist.find (v);
    if(find_it != hist.end ())
    {
      auto val = hist.at (v);
      hist[v] = val + 1;
    }
    else
    {
      hist[v] = 0;
    }
  }
  fs.close ();

  out = Script::Instance ().GetOutputDir () + "/hist.dat";

  fs.open (out, std::ios_base::out);
  for(std::map<int, int>::iterator it = hist.begin (); it != hist.end(); it++)
  {
    fs << it->first << "\t" << it->second << std::endl;
  }
  fs.close ();
  return 0;
}
