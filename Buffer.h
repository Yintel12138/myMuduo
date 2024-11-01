#pragma once

#include <vector>
#include <string>
#include <algorithm>

class Buffer
{
public:
  // kCheapPrepend 的作用是在缓冲区的前面预留一些空间，这样在需要在缓冲区前面插入数据时，可以避免频繁的内存重新分配和数据移动操作。这种设计可以提高性能，特别是在需要频繁在缓冲区前面插入数据的场景下。
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
  {
  }

  size_t readableBytes() const
  {
    return writerIndex_ - readerIndex_;
  }

  size_t writableBytes() const
  {
    return buffer_.size() - writerIndex_;
  }

  size_t prependableBytes() const
  {
    return readerIndex_;
  }

  // 返回缓冲区中可读数据的起始地址
  const char *peek() const
  {
    return begin() + readerIndex_;
  }

  //
  void retrieve(size_t len)
  {
    // TODO: 理解这里的含义
    if (len < readableBytes())
    {
      readerIndex_ += len; // 应用只读取了刻度缓冲区数据的一部分 长度为len
    }
    else
    {
      retrieveAll();
    }
  }

  void retrieveAll()
  {
    readerIndex_ = writerIndex_ = kCheapPrepend;
  }

  std::string retrieveAllAsString()
  {
    return retrieveAsString(readableBytes());
  }

  std::string retrieveAsString(size_t len)
  {
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  void ensureWriteableBytes(size_t len)
  {
    if (writableBytes() < len)
    {
      makeSpace(len); // 扩容函数
    }
  }

  void append(const char *data, size_t len)
  {
    ensureWriteableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
  }

  char *beginWrite()
  {
    return begin() + writerIndex_;
  }

  const char *beginWrite() const
  {
    return begin() + writerIndex_;
  }

  ssize_t readfd(int fd, int *saveErrno);

  ssize_t writeFd(int fd, int *saveErrno);

private:
  char *begin()
  {
    return &*buffer_.begin();
  }

  const char *begin() const
  {
    return &*buffer_.begin();
  }

  void makeSpace(size_t len)
  {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
      buffer_.resize(writerIndex_ + len);
    }
    else
    {
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_,
                begin() + writerIndex_,
                begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }
  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};