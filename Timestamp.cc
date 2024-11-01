#include <time.h>

#include "Timestamp.h"

Timestamp::Timestamp()
{
}

Timestamp::Timestamp(int64_t microSeconds)
    : microSecondsSinceEpoch_(microSeconds)
{
}

Timestamp Timestamp::now()
{
  time_t t = time(NULL);
  return Timestamp(t);
}

std::string Timestamp::toString() const
{
  char buf[128] = {0};
  tm *tm_time = localtime(&microSecondsSinceEpoch_);
  // 得到格式化输出
  snprintf(buf, 128, "%4d-%02d-%02d %02d:%02d:%02d",
           tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
           tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
  return buf;
}

// 临时测试代码
// int main()
// {
//   Timestamp t;
//   std::cout << t.now().toString();
//   return 0;
// }