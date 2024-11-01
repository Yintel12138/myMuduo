#include <sys/epoll.h>
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
  // 应该在eventloop中移除channel
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
  tie_ = obj;
  tied_ = true;
}

// 当改变channel的事件时，需要调用update函数负责在poller中更新
void Channel::update()
{
  // 通过所属的eventloop更新channel
  // TODO
  // loop_->updateChannel(this);
}

// 在channel所属的eventloop中处理事件
void Channel::remove()
{
  // TODO
  // loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
  if (tied_)
  {
    std::shared_ptr<void> guard = tie_.lock();

    if (guard)
    {
      handleEventWithGuard(receiveTime);
    }
  }
  else
  {
    handleEventWithGuard(receiveTime);
  }
}

// 事件发生时，根据revents_的值，调用相应的回调函数
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  LOG_INFO("Channel::handleEventWithGuard() fd = %d revents = %d", fd_, revents_);
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
  {
    if (closeCallback_)
      closeCallback_();
  }
  if (revents_ & EPOLLERR)
  {
    if (errorCallback_)
      errorCallback_();
  }
  if (revents_ & (EPOLLIN | EPOLLPRI))
  {
    if (readCallback_)
      readCallback_(receiveTime);
  }
  if (revents_ & EPOLLOUT)
  {
    if (writeCallback_)
      writeCallback_();
  }
  eventHandling_ = false;
}