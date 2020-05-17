#include "utils/script.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

const std::string Script::DEFAULT_OUTPUT_DIR = "";

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

std::string Script::GetEnv(const std::string& name)
{
  char* val = getenv(name.c_str());
  std::string ret;
  if(val)
  {
    ret = std::string(val);
  }
  return ret;
}

int Script::ChDir(const std::string& dir)
{
  return chdir(dir.c_str());
}

int Script::MkDir(const std::string& output_dir)
{
  return mkdir(output_dir.c_str (), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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
