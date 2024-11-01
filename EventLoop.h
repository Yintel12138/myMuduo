#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
class Channel;
class Poller;

// 事件循环类
// 主要包含了两个模块 channel poller
class EventLoop : noncopyable
{
public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  // 开启事件循环
  void loop();
  // 退出事件循环
  void quit();

  Timestamp pollReturnTime() const { return pollReturnTime_; }

  // 在当前loop中执行cb
  void runInLoop(Functor cb);
  // 把cb放入队列中，唤醒loop所在的线程，执行cb
  void queueInLoop(Functor cb);

  // 用来唤醒loop所在的线程的
  void wakeup();

  // 调用channel里面的方法
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel);

  // 判断EventLoop对象是否在自己的线程里面
  bool isInLoopThread() const
  {
    return threadId_ == CurrentThread::tid();
  }

private:
  void handleRead();
  void doPendingFunctors();

  using ChannelList = std::vector<Channel *>;

  std::atomic_bool looping_;
  std::atomic_bool quit_;

  const pid_t threadId_; // 记录当前loop所在线程的id

  Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
  std::unique_ptr<Poller> poller_;

  int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;

  ChannelList activeChannels_;

  std::atomic_bool callingPendingFunctors_;
  std::vector<Functor> pendingFunctors_;
  std::mutex mutex_;
};