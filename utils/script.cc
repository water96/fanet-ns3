#include "utils/script.h"

#include <sys/types.h>
#include <sys/stat.h>

const std::string Script::DEFAULT_OUTPUT_DIR = "/mnt/shared/sim";

Script::Script() {}

Script::~Script() {}

Script& Script::Instance()
{
  static Script inst;
  return inst;
}

void Script::SetScriptName(const std::string& script_name)
{
  m_script_name = script_name;
}

int Script::CreateOutputDir(const std::string& output_dir)
{
  m_output_dir = output_dir;
  m_output_dir += "/" + m_script_name;
  return mkdir(m_output_dir.c_str (), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

const std::string& Script::GetOutputDir() const
{
  return m_output_dir;
}

const std::string& Script::GetScriptName() const
{
  return m_script_name;
}

std::string Script::GetBaseName(const std::string& path)
{
  std::size_t last = path.find_last_of ('/');
  if(last != std::string::npos)
  {
    return path.substr (last);
  }
  return path;
}
