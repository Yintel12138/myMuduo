#include <errno.h>
#include <unistd.h>
#include <strings.h>

#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
  if (epollfd_ < 0)
  {
    LOG_FATAL("epoll_create error:%d \n", errno);
  }
}
EpollPoller::~EpollPoller()
{
  ::close(epollfd_);
};

Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activateChannels)
{
  LOG_DEBUG("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());

  int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
  int saveErrno = errno;
  Timestamp now(Timestamp::now());

  if (numEvents > 0)
  {
    LOG_INFO("%d events happened \n", numEvents);
    fillActiveChannels(numEvents, activateChannels);
    // 如果事件数目等于events_的大小，说明events_数组已经满了，需要扩容
    {
      events_.resize(events_.size() * 2);
    }
  }
  // 超时
  else if (numEvents == 0)
  {
    LOG_DEBUG("%s timeout! \n", __FUNCTION__);
  }
  else
  {
    // 如果是被信号中断的话，不需要打印错误日志
    if (saveErrno != EINTR)
    {
      errno = saveErrno;
      LOG_ERROR("EPollPoller::poll() err!");
    }
  }
  return now;
}

void EpollPoller::updateChannel(Channel *channel)
{
  const int index = channel->index();
  LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

  if (index == kNew || index == kDeleted)
  {
    if (index == kNew)
    {
      int fd = channel->fd();
      channels_[fd] = channel;
    }
    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  }
  else
  {
    if (channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    }
    else
    {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EpollPoller::removeChannel(Channel *channel)
{
  int fd = channel->fd();
  channels_.erase(fd);
  LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);
  int index = channel->index();
  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activateChannels) const
{
  // 遍历所有发生的事件
  for (int i = 0; i < numEvents; ++i)
  {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr); // 获取事件对应的Channel
    channel->set_revents(events_[i].events); // 设置Channel的返回事件
    activateChannels->push_back(channel); // 将Channel加入激活列表
  }
}

void EpollPoller::update(int operation, Channel *channel)
{
  epoll_event event;
  // 将结构体清零
  bzero(&event, sizeof event);
  int fd = channel->fd();
  event.events = channel->events();
  event.data.fd = fd;
  event.data.ptr = channel;
  // 
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      LOG_ERROR("epoll_ctl op=%d fd=%d\n", operation, fd);
    }
    else
    {
      LOG_FATAL("epoll_ctl op=%d fd=%d\n", operation, fd);
    }
  }
}
