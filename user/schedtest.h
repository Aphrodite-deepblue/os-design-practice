// Small user-space helpers shared by the task-5 scheduler tests.
// These tests do not add kernel instrumentation or system calls.
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static char *test_name;
static int child_pids[16];
static int child_count;
static int is_child;

static inline void
cleanup_children(void)
{
  for (int i = 0; i < child_count; i++)
    kill(child_pids[i]);
  while (wait(0) > 0)
    ;
  child_count = 0;
}

static inline void
fail(char *reason)
{
  printf("%s: FAIL reason=%s\n", test_name, reason);
  if (!is_child)
    cleanup_children();
  exit(1);
}

static inline void
check(int condition, char *reason)
{
  if (!condition)
    fail(reason);
}

static inline int
spawn(void)
{
  check(child_count < 16, "too many test children");
  int pid = fork();
  check(pid >= 0, "fork failed");
  if (pid == 0)
    is_child = 1;
  else
    child_pids[child_count++] = pid;
  return pid;
}

static inline void
read_all(int fd, void *buffer, int length)
{
  char *p = buffer;
  while (length > 0) {
    int n = read(fd, p, length);
    check(n > 0, "short pipe read");
    p += n;
    length -= n;
  }
}

static inline void
write_all(int fd, void *buffer, int length)
{
  check(write(fd, buffer, length) == length, "short pipe write");
}

static inline void
reap_success(void)
{
  int n = child_count;
  for (int i = 0; i < n; i++) {
    int status = -1;
    check(wait(&status) > 0 && status == 0, "child failed");
  }
  child_count = 0;
  check(wait(0) == -1, "unreaped child");
}

static inline uint
burn(uint iterations)
{
  volatile uint value = 1;
  for (uint i = 0; i < iterations; i++)
    value = value * 1664525U + 1013904223U;
  return value;
}
