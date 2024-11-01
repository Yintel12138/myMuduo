#pragma once
#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class EpollPoller : public Poller
{
public:
  EpollPoller(EventLoop *loop);
  ~EpollPoller() override;

  // 重写基类Poller的抽象方法
  Timestamp poll(int timeoutMs, ChannelList *activateChannels) override;
  void updateChannel(Channel *channel) override;
  void removeChannel(Channel *channel) override;

private:
  static const int kInitEventListSize = 16;

  // 填写活跃的连接
  void fillActiveChannels(int numEvents, ChannelList *activateChannels) const;
  // 更新channel通道
  void update(int operation, Channel *channel);
  // 保存活跃的事件
  using EventList = std::vector<epoll_event>;
  // 保存epoll的fd
  int epollfd_;

  EventList events_;
};
