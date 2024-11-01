#pragma once
#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
  using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
  Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback &cb)
  {
    // 这里考虑改成移动语义
    newConnectionCallback_ = cb;
  }
  bool listenning() const { return listenning_; }

  void listen();

private:
  void handleRead();

  EventLoop *loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listenning_;
};