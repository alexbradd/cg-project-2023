#include <seng/log.hpp>

#include <iostream>
#include <string>

#define INFO_STR "[INFO] "
#define WARN_STR "[WARN] "
#define ERRO_STR "[ERRO] "
#define DBUG_STR "[DBUG] "

void seng::log::logOutput(seng::log::LogLevels lvl, std::string out)
{
  switch (lvl) {
    case seng::log::LogLevels::DBUG:
      std::cerr << DBUG_STR;
      break;
    case seng::log::LogLevels::INFO:
      std::cerr << INFO_STR;
      break;
    case seng::log::LogLevels::WARN:
      std::cerr << WARN_STR;
      break;
    case seng::log::LogLevels::ERRO:
      std::cerr << ERRO_STR;
      break;
  }
  std::cerr << out << std::endl;
}
