#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define min(a, b) ((a) < (b) ? (a) : (b))

#define BUFFER_SIZE (1024 * 1024 * 1024)

uint8_t buf[BUFFER_SIZE];
size_t len, maxlen, off;
bool info_requested = false;

static void sighandler(int sig) {
  info_requested = true;
}

int main(void) {
  if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK)) {
    perror("failed to turn stdin async");
    return EXIT_FAILURE;
  }
  if (fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) | O_NONBLOCK)) {
    perror("failed to turn stdout async");
    return EXIT_FAILURE;
  }

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = sighandler;
  sa.sa_flags = 0;
  if (sigaction(SIGUSR1, &sa, NULL))
    perror("failed to register SIGUSR1 handler (continuing nonetheless)");

  struct pollfd fds[] = {
    { .fd = STDOUT_FILENO, .events = 0, .revents = 0 },
    { .fd = STDIN_FILENO, .events = POLLIN, .revents = 0 },
  };
  while (fds[0].fd >= 0 && (fds[1].fd >= 0 || len > 0)) {
    if (len == 0) {
      off = 0;
    }
    fds[0].revents = 0;
    fds[1].revents = 0;
    fds[0].events = len > 0 ? POLLOUT : 0;
    fds[1].events = len < BUFFER_SIZE ? POLLIN : 0;
    int res = poll(fds, 2, -1);
    if (res < 0 && errno != EINTR) {
      perror("failed to poll");
      return EXIT_FAILURE;
    }
    if (info_requested) {
      info_requested = false;
      fprintf(stderr, "blobomnom info\tcur: %zu\tmax: %zu\n", len, maxlen);
    }
    if (fds[0].revents & POLLERR || fds[1].revents & POLLERR) {
      return EXIT_FAILURE;
    }
    if (fds[0].revents & POLLHUP) {
      return EXIT_SUCCESS; // TODO: is it though?
    }
    if (fds[0].revents & POLLOUT) {
      size_t to_write = min(len, BUFFER_SIZE-off);
      ssize_t written = write(fds[0].fd, &buf[off], to_write);
      if (written < 0) {
        perror("failed to write");
        return EXIT_FAILURE;
      }
      off += written;
      if (off >= BUFFER_SIZE)
        off -= BUFFER_SIZE;
      len -= written;
    }
    if (fds[1].revents & POLLIN) {
      size_t to_read = min(BUFFER_SIZE-len, BUFFER_SIZE-off);
      size_t end_offset = off+len;
      if (end_offset >= BUFFER_SIZE)
        end_offset -= BUFFER_SIZE;
      ssize_t readen = read(fds[1].fd, &buf[end_offset], to_read);
      if (readen < 0) {
        perror("failed to read");
        return EXIT_FAILURE;
      } else if (readen == 0) {
        fds[1].fd = -1;
      } else {
        len += readen;
        if (maxlen < len)
          maxlen = len;
      }
    }
    if (fds[1].revents & POLLHUP) {
      fds[1].fd = -1;
    }
  }
  return EXIT_SUCCESS;
}
