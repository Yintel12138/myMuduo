#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
  if (loop == nullptr)
  {
    LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
  }
  return loop;
}

TcpConnection::TcpConnection(
    EventLoop *loop,
    const std::string &nameArg,
    int sockfd,
    const InetAddress &localAddr,
    const InetAddress &peerAddr) : loop_(CheckLoopNotNull(loop)),
                                   name_(nameArg),
                                   state_(kConnecting),
                                   reading_(true),
                                   socket_(new Socket(sockfd)),
                                   channel_(new Channel(loop, sockfd)),
                                   localAddr_(localAddr),
                                   peerAddr_(peerAddr),
                                   highWaterMark_(64 * 1024 * 1024)
{
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));

  channel_->setWriteCallback(
      std::bind(&TcpConnection::handleWrite, this));

  channel_->setCloseCallback(
      std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(
      std::bind(&TcpConnection::handleError, this));
  LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n",
           name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)
{
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
}

void TcpConnection::shutdown()
{
}

void TcpConnection::shutdownInLoop()
{
}

void TcpConnection::connectEstablished()
{
}

void TcpConnection::connectDestroyed()
{
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
}

void TcpConnection::handleWrite()
{
  if (channel_->isWriting())
  {
    int savedErrno = 0;
    ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0)
      {
        channel_->disableWriting();
        if (writeCompleteCallback_)
        {
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting)
        {
          shutdownInLoop();
        }
      }
    }
    else
    {
      LOG_ERROR("TcpConnection::handleWrite");
    }
  }
}

void TcpConnection::handleClose()
{
  LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr connPtr(shared_from_this());
  connectionCallback_(connPtr);
  closeCallback_(connPtr);
}

void TcpConnection::handleError()
{
  int optval;
  socklen_t optlen = sizeof optval;
  int err = 0;
  if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
  {
    err = errno;
  }
  else
  {
    err = optval;
  }
  LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}