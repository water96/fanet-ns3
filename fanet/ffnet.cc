#include <iostream>
#include <string>

#include "utils/script.h"

int main(int argc, char *argv[])
{
  Script::Instance ().SetScriptName ( Script::GetBaseName (argv[0]) );
  Script::Instance ().CreateOutputDir (Script::DEFAULT_OUTPUT_DIR);

  std::cout << "Scriptname: " << Script::Instance ().GetScriptName () << std::endl;
  std::cout << "Output dir: " << Script::Instance ().GetOutputDir () << std::endl;

  return 0;
}
