#include "bbs.h"
#include <devctl.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define VEC_SIZE 1024
std::vector<std::uint32_t> vec(VEC_SIZE);
size_t current = -1;
bool circle = false;
bool sigint = false;

void push(std::vector<std::uint32_t> &vec, size_t &current,
          std::uint32_t number) {
  current++;
  if (current > vec.size()) {
    circle = true;
    current = 0;
  }
  vec[current] = number;
}

void sigint_handler(int sig) {
	sigint=true;
}

int main(int argc, char **argv) {
  // open a connection to the server (fd == coid)
  int fd = open("/dev/cryptobbs", O_RDWR);
  if (fd < 0) {
    std::cerr << "E: unable to open server connection: " << strerror(errno)
              << std::endl;
    return EXIT_FAILURE;
  }

  bbs::BBSParams params;
  params.p = 3;
  params.q = 263;
  params.seed = 866;

  int commandSetResult =
      devctl(fd, MY_DEVCTL_SET_PARAM, &params, sizeof(bbs::BBSParams), NULL);
  switch (commandSetResult) {
  case EOK:
    //     Success.
    break;
  case EAGAIN:
    fprintf(
        stderr,
        "The devctl() command couldn't be completed because the device driver "
        "was in use by another process, or the driver was unable to carry out");
    return EXIT_FAILURE;
    //     Thethe request due to an outstanding command in progress. devctl()
    //     command couldn't be completed because the device driver was in use by
    //     another process, or the driver was unable to carry out the request
    //     due to an outstanding command in progress.
    // EBADF
    //     Invalid open file descriptor, filedes.
    // EINTR
    //     The devctl() function was interrupted by a signal.
    // EINVAL
    //     The device driver detected an error in dev_data_ptr or n_bytes.
    // EIO
    //     The devctl() function couldn't complete because of a hardware error.
    // ENOSYS
    //     The device doesn't support the dcmd command.
    // ENOTTY
    //     The dcmd argument isn't a valid command for this device.
    // EPERM
    //     The process doesn't have sufficient permission to carry out the
    //     requested command.
  }
  if (commandSetResult != EOK) {
    return EXIT_FAILURE;
  }

  std::uint32_t number;

  signal(SIGINT, sigint_handler);

  commandSetResult = devctl(fd, MY_DEVCTL_START, &number, sizeof(number), NULL);
  while (commandSetResult == EOK && !sigint) {
    push(vec, current, number);

    commandSetResult =
        devctl(fd, MY_DEVCTL_START, &number, sizeof(number), NULL);
  };

  size_t end_index = current;
  if (circle == true)
    end_index = vec.size() - 1;
  for (size_t i = 0; i < end_index; i++)
    std::cout << vec[i] << "; ";
  std::cout << std::endl;

  close(fd);

  return EXIT_SUCCESS;
}
