#include <seng/log.hpp>

#include <iostream>
#include <string>

#define INFO_STR "[INFO] "
#define WARN_STR "[WARN] "
#define ERRO_STR "[ERRO] "
#define DBUG_STR "[DBUG] "

#define lvl_lt(lhs, rhs) (static_cast<uint32_t>(lhs) < static_cast<uint32_t>(rhs))

using namespace seng;

static log::LogLevels minLvl = log::LogLevels::DBUG;

void log::logOutput(seng::log::LogLevels lvl, std::string out)
{
  if (lvl_lt(lvl, minLvl)) return;

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

log::LogLevels log::minimumLoggingLevel()
{
  return minLvl;
}

void log::minimumLoggingLevel(log::LogLevels lvl)
{
  minLvl = lvl;
}
