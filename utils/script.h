#ifndef SCRIPT_H
#define SCRIPT_H

#include <string>

class Script
{
private:
  std::string m_script_name;
  std::string m_output_dir;

  Script();
  Script(const Script& root) = delete;
  Script& operator=(const Script&) = delete;
public:

  static int MkDir(const std::string& output_dir);
  static int ChDir(const std::string& dir);
  static std::string GetEnv(const std::string& var);

  ~Script();
  static Script& Instance();
  void SetScriptName(const std::string& script_name);
  int CreateOutputDir(const std::string& output_dir);
  const std::string& GetOutputDir() const;
  const std::string& GetScriptName() const;

  static const std::string DEFAULT_OUTPUT_DIR;

  //utils
  static std::string GetBaseName(const std::string& path);
};
#endif // SCRIPT_H
