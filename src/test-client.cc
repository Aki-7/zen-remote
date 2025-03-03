#include <sys/epoll.h>
#include <zen-remote/logger.h>
#include <zen-remote/loop.h>
#include <zen-remote/remote.h>

#include <iostream>

using namespace zen::remote;

bool running = true;

class LogSink : public ILogSink {
  void Sink(Severity remote_severity, const char * /*pretty_function*/,
      const char * /*file*/, int /*line*/, const char *format, va_list vp)
  {
    std::string prefix = "[remote]";
    switch (remote_severity) {
      case Severity::DEBUG:
        prefix += " D:";
        break;
      case Severity::INFO:
        prefix += " I:";
        break;
      case Severity::WARN:
        prefix += " W:";
        break;
      case Severity::ERROR:
        prefix += " E:";
        break;
      case Severity::FATAL:
        prefix += " F:";
        break;
      default:
        prefix += " ?:";
        break;
    }

    std::string full_format = prefix + format + "\n";

    vfprintf(stderr, full_format.c_str(), vp);
  }
};

class Loop : public ILoop {
 public:
  Loop(int epoll_fd) : epoll_fd_(epoll_fd) {}

  void AddFd(FdSource *source) override
  {
    epoll_event *event = static_cast<epoll_event *>(calloc(1, sizeof *event));
    source->data = event;
    event->data.ptr = source;

    if (source->mask & FdSource::kReadable) event->events |= EPOLLIN;
    if (source->mask & FdSource::kWritable) event->events |= EPOLLOUT;
    if (source->mask & FdSource::kHangup) event->events |= EPOLLHUP;
    if (source->mask & FdSource::kError) event->events |= EPOLLERR;

    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, source->fd, event);
  };

  void RemoveFd(FdSource *source) override
  {
    epoll_event *event = static_cast<epoll_event *>(source->data);
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, source->fd, event);
    free(event);
  };

  void Terminate() override { running = false; };

 private:
  int epoll_fd_;
};

int
main(int /*argc*/
    ,
    char const * /*argv*/[])
{
  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);

  std::unique_ptr<IRemote> remote;

  InitializeLogger(std::make_unique<LogSink>());

  {
    auto loop = std::make_unique<Loop>(epoll_fd);
    remote = CreateRemote(std::move(loop));
  }

  remote->Start();

  int epoll_count;
  struct epoll_event events[16];
  while (running) {
    epoll_count = epoll_wait(epoll_fd, events, 16, -1);
    for (int i = 0; i < epoll_count; i++) {
      auto source = static_cast<FdSource *>(events[i].data.ptr);

      uint32_t mask = 0;
      if (events[i].events & EPOLLIN) mask |= FdSource::kReadable;
      if (events[i].events & EPOLLOUT) mask |= FdSource::kWritable;
      if (events[i].events & EPOLLHUP) mask |= FdSource::kHangup;
      if (events[i].events & EPOLLERR) mask |= FdSource::kError;

      source->callback(source->fd, mask);
    }
  }

  return 0;
}
