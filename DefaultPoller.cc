#include "Poller.h"
#include <stdlib.h>

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
  if (::getenv("MUDUO_USE_POLL"))
  {
    return nullptr; // 生成一个PollPoller对象
  }
  else
  {
    return nullptr; // 生成一个EPollPoller对象
  }
}
