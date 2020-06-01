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

std::string utils::GetBaseName(const std::string& path)
{
  std::size_t last = path.find_last_of ('/');
  if(last != std::string::npos)
  {
    return path.substr (last);
  }
  return path;
}

std::string utils::TrimString(const std::string& s)
{
  std::size_t f;
  std::string ret = s;
  while( (f = ret.find(' ')) != std::string::npos )
  {
    ret.erase(f);
  }
  return ret;
}

void utils::SplitString(const std::string& in, std::string delim, std::vector<std::string>& out_vec)
{
  std::size_t pos;
  std::string s = in;
  while ( (pos = s.find(delim)) != std::string::npos )
  {
    std::string f = s.substr(0, pos);
    if(!f.empty())
    {
      f = TrimString(f);
      out_vec.push_back(f);
    }
    s = s.substr(pos + 1);
  }
  if(s != in)
  {
    s = TrimString(s);
    out_vec.push_back(s);
  }

}
