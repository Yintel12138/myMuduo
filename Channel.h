#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>
/**
 * channel 理解为通道，封装了sockfd以及它感兴趣的event
 */
class EventLoop;

class Channel : noncopyable
{
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  Channel(EventLoop *loop, int fd);
  ~Channel();
  // 处理事件，调用相应的回调函数
  void handleEvent(Timestamp receiveTime);
  // 设置读回调函数
  void setReadCallback(ReadEventCallback cb)
  {
    // 使用移动语义
    readCallback_ = std::move(cb);
  }
  // 设置写回调函数
  void setWriteCallback(EventCallback cb)
  {
    writeCallback_ = std::move(cb);
  }
  // 设置关闭回调函数
  void setCloseCallback(EventCallback cb)
  {
    closeCallback_ = std::move(cb);
  }
  // 设置错误回调函数
  void setErrorCallback(EventCallback cb)
  {
    errorCallback_ = std::move(cb);
  }

  // 防止channel被手动销毁，channel还在执行回调函数
  void tie(const std::shared_ptr<void> &);

  int fd() const
  {
    return fd_;
  }

  int events() const
  {
    return events_;
  }
  void set_revents(int revt)
  {
    revents_ = revt;
  }

  // 设置fd对应的事件状态
  void enableReading()
  {
    events_ |= kReadEvent;
    update();
  }
  void disableReading()
  {
    events_ &= ~kReadEvent;
    update();
  }
  void enableWriting()
  {
    events_ |= kWriteEvent;
    update();
  }
  void disableWriting()
  {
    events_ &= ~kWriteEvent;
    update();
  }
  void disableAll()
  {
    events_ = kNoneEvent;
    update();
  }
  // 判断fd对应的事件状态
  bool isWriting() const
  {
    return events_ & kWriteEvent;
  }
  bool isReading() const
  {
    return events_ & kReadEvent;
  }
  bool isNoneEvent() const
  {
    return events_ == kNoneEvent;
  }

  // for Poller
  int index()
  {
    return index_;
  }
  void set_index(int idx)
  {
    index_ = idx;
  }

  // for debug
  std::string reventsToString() const;
  std::string eventsToString() const;

  EventLoop *ownerLoop()
  {
    return loop_;
  }
  void remove();

private:
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop *loop_;
  const int fd_; // poller 监听的对象
  int events_;   // 关心的事件
  int revents_;  // poller 返回的具体发生的事件
  int index_;    // using by poller

  std::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;
  bool addedToLoop_;

  // 因为channel里面能够获知fd的具体事件revents，所以它需要设置相应的回调函数，他负责调用回调函数
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};