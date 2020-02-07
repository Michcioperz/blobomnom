#define _DEFAULT_SOURCE
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define min(a, b) ((a) < (b) ? (a) : (b))

#define BUFFER_SIZE (1024 * 1024 * 1024)

uint8_t buf[BUFFER_SIZE];
size_t len, off;

int main(void) {
  if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK)) {
    perror("failed to turn stdin async");
    return EXIT_FAILURE;
  }
  if (fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) | O_NONBLOCK)) {
    perror("failed to turn stdout async");
    return EXIT_FAILURE;
  }
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
    if (poll(fds, 2, -1) < 0) {
      perror("failed to poll");
      return EXIT_FAILURE;
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
      }
    }
    if (fds[1].revents & POLLHUP) {
      fds[1].fd = -1;
    }
  }
  return EXIT_SUCCESS;
}
