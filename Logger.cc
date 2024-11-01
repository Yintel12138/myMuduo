#include <iostream>
#include "Logger.h"
#include "Timestamp.h"
// 获取唯一实例
Logger &Logger::instance()
{
  static Logger logger;
  return logger;
}

void Logger::setLogLevel(int level)
{
  logLevel_ = level;
}

void Logger::log(std::string message)
{

  switch (logLevel_)
  {
  case INFO:
    std::cout << "[INFO] ";
    break;
  case ERROR:
    std::cout << "[ERROR] ";
    break;
  case FATAL:
    std::cout << "[FATAL] ";
    break;
  case DEBUG:
    std::cout << "[DEBUG] ";
    break;
  default:
    std::cout << "[INFO] ";
    break;
  }
  std::cout << Timestamp::now().toString() << " : " << message << std::endl;
}