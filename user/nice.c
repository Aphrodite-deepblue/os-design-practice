#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int
parse_nonnegative_int(char *text, int *value)
{
  int number = 0;

  if (*text == 0)
    return -1;

  for (; *text != 0; text++) {
    if (*text < '0' || *text > '9')
      return -1;
    int digit = *text - '0';
    if (number > (0x7fffffff - digit) / 10)
      return -1;
    number = number * 10 + digit;
  }

  *value = number;
  return 0;
}

int
main(int argc, char **argv)
{
  int pid;
  int priority;

  if (argc != 2 && argc != 3) {
    fprintf(2, "usage: nice pid [priority]\n");
    exit(1);
  }

  if (parse_nonnegative_int(argv[1], &pid) < 0 || pid == 0) {
    fprintf(2, "nice: invalid pid '%s'\n", argv[1]);
    exit(1);
  }

  if (argc == 2) {
    priority = getpriority(pid);
    if (priority < 0) {
      fprintf(2, "nice: process %d not found\n", pid);
      exit(1);
    }
    printf("pid %d priority = %d\n", pid, priority);
    exit(0);
  }

  if (parse_nonnegative_int(argv[2], &priority) < 0) {
    fprintf(2, "nice: invalid priority '%s'\n", argv[2]);
    exit(1);
  }

  if (setpriority(pid, priority) < 0) {
    fprintf(2, "nice: failed to set pid %d priority to %d\n", pid, priority);
    exit(1);
  }

  printf("pid %d priority changed to %d\n", pid, priority);
  exit(0);
}
