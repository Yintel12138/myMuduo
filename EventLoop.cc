#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
// 防止一个线程创建多个eventloop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来通知处理新来的channel
int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG_FATAL("eventfd error:%d \n", errno);
  }
  return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_))
{
  LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
  // 在构造的时候发现如果已经有了，则报错
  if (t_loopInThisThread)
  {
    LOG_FATAL("Another eventloop %p has been created in this thread %d \n", t_loopInThisThread, threadId_);
  }
  else
  {
    t_loopInThisThread = this;
  }
  // 设置wakeupfd的时间类型以及发生事件后的回调操作
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  // 监听wakeup channel 的epollin 读事件
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  // 关闭文件描述符
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{

  looping_ = true;
  quit_ = false;

  LOG_INFO("EventLoop %p start looping", this);
  while (!quit_)
  {
    activeChannels_.clear();
    // 监听有哪些activate channels,写入activateChannels_
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    for (Channel *channel : activeChannels_)
    {
      // 检查有哪些activechannels_,处理对应的事件
      // ? 这为什么要传入这个参数(ans: 作为一个时间戳)
      channel->handleEvent(pollReturnTime_);
    }
    // 执行待处理的回调操作
    doPendingFunctors();
  }
  LOG_INFO("Eventloop %p stop looping. \n", this);
  looping_ = false;
}

void EventLoop::quit()
{
  // 这里有一个误区，不是说其他线程执行这个函数，而是从其他线程中调用这个函数
  quit_ = true;
  // 如果不在eventloop所在的线程,则先唤醒
  if (!isInLoopThread())
  {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    // 如果不是在当前的loop线程中执行，则需要先唤醒
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(Functor cb)
{
  // 通过代码块包括，控制锁的生命周期
  {
    std::unique_lock<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(cb);
  } // 这里锁会在代码块结束时自动释放

  // 唤醒对应的
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

// 专门处理wakeupFd_文件描述符上的读事件，实际上是个唤醒操作？
void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("Eventloop::handleRead() reads %lu bytes instead of 8", n);
  }
}
// 唤醒loop所在的线程
void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("Eventloop::wakeup() reads %lu bytes instead of 8", n);
  }
}

void EventLoop::updateChannel(Channel *channel)
{
  poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
  poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
  return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const Functor &functor : functors)
  {
    functor();
  }
  callingPendingFunctors_ = false;
}