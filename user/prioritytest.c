#include "schedtest.h"

#define WORK        120000000U
#define MAX_WORKERS 6

struct result {
  int id;
  int pid;
  int priority;
  int start;
  int finish;
  uint checksum;
};

static void
api_tests(void)
{
  int self = getpid();
  check(getpriority(self) == 10, "fresh shell must use default priority 10");
  int values[] = {0, 10, 31, 7};
  for (int i = 0; i < 4; i++) {
    check(setpriority(self, values[i]) == 0, "set valid priority");
    check(getpriority(self) == values[i], "get after set");
  }
  check(setpriority(self, 0) == 0, "pin test coordinator priority");
  int invalid[] = {-1, 32, 0x7fffffff, (-0x7fffffff - 1)};
  for (int i = 0; i < 4; i++) {
    check(setpriority(self, invalid[i]) == -1, "reject invalid priority");
    check(getpriority(self) == 0, "invalid set changed old value");
  }
  int badpids[] = {-1, 0, 0x7fffffff};
  for (int i = 0; i < 3; i++) {
    check(getpriority(badpids[i]) == -1, "reject missing query pid");
    check(setpriority(badpids[i], 10) == -1, "reject missing set pid");
  }
  printf("CASE api boundaries invalid-values missing-pids PASS\n");

  for (int i = 0; i < 4; i++) {
    int reply[2], gate[2];
    check(pipe(reply) == 0 && pipe(gate) == 0, "inheritance pipes");
    check(setpriority(self, values[i]) == 0, "parent priority before fork");
    int pid = spawn();
    if (pid == 0) {
      close(reply[0]);
      close(gate[1]);
      int initial = getpriority(getpid());
      write_all(reply[1], &initial, sizeof(initial));
      char token;
      read_all(gate[0], &token, 1);
      int independent = getpriority(getpid());
      write_all(reply[1], &independent, sizeof(independent));
      close(gate[0]);
      close(reply[1]);
      exit(0);
    }
    close(reply[1]);
    close(gate[0]);
    int observed;
    read_all(reply[0], &observed, sizeof(observed));
    check(observed == values[i], "fork did not inherit priority");
    check(setpriority(self, 0) == 0, "change parent after fork");
    write_all(gate[1], "x", 1);
    read_all(reply[0], &observed, sizeof(observed));
    check(observed == values[i], "child priority changed with parent");
    close(reply[0]);
    close(gate[1]);
    reap_success();
    check(getpriority(pid) == -1 && setpriority(pid, 10) == -1,
          "reaped pid remained accessible");
  }
  // Repeated fork/exit cycles exercise process-slot reuse with new values.
  for (int i = 0; i < 32; i++) {
    int expected = i % 32;
    check(setpriority(self, expected) == 0, "reuse parent priority");
    if (spawn() == 0)
      exit(getpriority(getpid()) == expected ? 0 : 1);
    reap_success();
  }
  check(setpriority(self, 0) == 0, "restore coordinator");
  printf("CASE fork-inheritance independent-copy exit slot-reuse PASS\n");
}

static void
nice_case(char **args, int expected_status)
{
  if (spawn() == 0) {
    exec("nice", args);
    exit(99);
  }
  int status = -1;
  check(wait(&status) > 0 && status == expected_status, "nice exit status");
  child_count = 0;
}

static void
nice_tests(void)
{
  char pidbuf[16];
  int pid = getpid(), digits = 0;
  do {
    pidbuf[digits++] = '0' + pid % 10;
    pid /= 10;
  } while (pid);
  pidbuf[digits] = 0;
  for (int i = 0; i < digits / 2; i++) {
    char t = pidbuf[i];
    pidbuf[i] = pidbuf[digits - 1 - i];
    pidbuf[digits - 1 - i] = t;
  }
  char *query[] = {"nice", pidbuf, 0};
  char *set[] = {"nice", pidbuf, "2", 0};
  char *bad[] = {"nice", pidbuf, "32", 0};
  char *badpid[] = {"nice", "2147483647", 0};
  char *overflow[] = {"nice", "999999999999999999999", 0};
  char *text[] = {"nice", "abc", 0};
  char *negative[] = {"nice", pidbuf, "-1", 0};
  char *usage[] = {"nice", 0};
  nice_case(query, 0);
  nice_case(set, 0);
  check(getpriority(getpid()) == 2, "nice did not change priority");
  check(setpriority(getpid(), 0) == 0, "restore after nice");
  nice_case(bad, 1);
  nice_case(badpid, 1);
  nice_case(overflow, 1);
  nice_case(text, 1);
  nice_case(negative, 1);
  nice_case(usage, 1);
  check(getpriority(getpid()) == 0, "invalid nice changed priority");
  printf("CASE nice query set invalid-input exit-status PASS\n");
}

static void
workload(int mixed, int workers, int strict)
{
  int ready[2], gate[2], replies[2];
  check(pipe(ready) == 0 && pipe(gate) == 0 && pipe(replies) == 0,
        "workload pipes");
  check(setpriority(getpid(), 0) == 0, "coordinator priority");
  int levels[] = {3, 10, 20};
  for (int i = 0; i < workers; i++) {
    if (spawn() == 0) {
      close(ready[0]);
      close(gate[1]);
      close(replies[0]);
      struct result r;
      r.id = i;
      r.pid = getpid();
      r.priority = mixed ? levels[i % 3] : 10;
      check(setpriority(r.pid, r.priority) == 0, "worker priority");
      write_all(ready[1], "r", 1);
      close(ready[1]);
      int epoch;
      read_all(gate[0], &epoch, sizeof(epoch));
      close(gate[0]);
      r.start = uptime() - epoch;
      r.checksum = burn(WORK);
      r.finish = uptime() - epoch;
      write_all(replies[1], &r, sizeof(r));
      close(replies[1]);
      exit(0);
    }
  }
  close(ready[1]);
  close(gate[0]);
  close(replies[1]);
  char tokens[MAX_WORKERS];
  read_all(ready[0], tokens, workers);
  close(ready[0]);
  int epoch = uptime();
  for (int i = 0; i < workers; i++)
    write_all(gate[1], &epoch, sizeof(epoch));
  close(gate[1]);
  struct result results[MAX_WORKERS];
  int seen[MAX_WORKERS] = {0};
  for (int i = 0; i < workers; i++) {
    struct result r;
    read_all(replies[0], &r, sizeof(r));
    check(r.id >= 0 && r.id < workers && !seen[r.id], "duplicate result");
    seen[r.id] = 1;
    results[r.id] = r;
  }
  close(replies[0]);
  reap_success();
  int first_finish = 0x7fffffff, last_start = 0;
  for (int i = 0; i < workers; i++) {
    struct result *r = &results[i];
    check(r->finish >= r->start && r->start >= 0, "invalid timing");
    check(r->checksum == results[0].checksum, "different work result");
    if (r->finish < first_finish)
      first_finish = r->finish;
    if (r->start > last_start)
      last_start = r->start;
    printf("METRIC case=%s id=%d pid=%d priority=%d start_ticks=%d "
           "finish_ticks=%d work=%d\n",
           mixed ? "mixed" : "equal", i, r->pid, r->priority, r->start,
           r->finish, WORK);
  }
  check(first_finish >= 2, "work too short for tick-resolution measurement");
  if (!mixed)
    check(last_start < first_finish,
          "equal-priority worker starved until exit");
  if (mixed && strict) {
    check(results[0].finish <= results[1].start &&
            results[1].finish <= results[2].start,
          "single-CPU priority isolation");
    check(results[0].finish < results[1].finish &&
            results[1].finish < results[2].finish,
          "single-CPU priority completion order");
  }
  printf("CASE %s workers=%d PASS\n", mixed ? "mixed" : "equal", workers);
}

int
main(int argc, char **argv)
{
  test_name = "prioritytest";
  int smp = argc == 2 && strcmp(argv[1], "--smp") == 0;
  int bench = argc == 2 && strcmp(argv[1], "bench") == 0;
  if (argc > 2 || (argc == 2 && !smp && !bench))
    fail("usage: prioritytest [--smp|bench]");
  int original = getpriority(getpid());
  if (!bench) {
    api_tests();
    nice_tests();
  }
  workload(0, smp ? 6 : 3, 0);
  workload(1, smp ? 6 : 3, !smp && !bench);
  check(setpriority(getpid(), original) == 0, "restore original priority");
  printf("prioritytest: PASS\n");
  exit(0);
}
