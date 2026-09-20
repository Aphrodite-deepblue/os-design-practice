#include "schedtest.h"

// Eight busy competitors cover both the 1-CPU experiment and 3-CPU stress run.
// Low priority 3 requires several promotions, keeping the experiment bounded.
#define HIGH_WORKERS   8
#define LOW_PRIORITY   3
#define DEADLINE_TICKS 250

struct event {
  int kind; // 1 = low process actually ran, 2 = timeout, 3 = sampled promotion
  int tick;
  int priority;
};

int
main(void)
{
  test_name = "agingtest";
  int original = getpriority(getpid());
  check(setpriority(getpid(), 0) == 0, "coordinator priority");
  int ready[2], high_gate[2], low_gate[2], events[2];
  check(pipe(ready) == 0 && pipe(high_gate) == 0 && pipe(low_gate) == 0 &&
          pipe(events) == 0,
        "aging pipes");
  for (int i = 0; i < HIGH_WORKERS; i++) {
    if (spawn() == 0) {
      close(ready[0]);
      close(high_gate[1]);
      close(low_gate[0]);
      close(low_gate[1]);
      close(events[0]);
      close(events[1]);
      check(setpriority(getpid(), 0) == 0, "high worker priority");
      write_all(ready[1], "r", 1);
      close(ready[1]);
      char token;
      read_all(high_gate[0], &token, 1);
      close(high_gate[0]);
      for (;;)
        burn(100000U);
    }
  }
  int low = spawn();
  if (low == 0) {
    close(ready[0]);
    close(high_gate[0]);
    close(high_gate[1]);
    close(low_gate[1]);
    close(events[0]);
    check(setpriority(getpid(), LOW_PRIORITY) == 0, "low worker priority");
    write_all(ready[1], "r", 1);
    close(ready[1]);
    int epoch;
    read_all(low_gate[0], &epoch, sizeof(epoch));
    close(low_gate[0]);
    struct event e = {1, uptime() - epoch, getpriority(getpid())};
    write_all(events[1], &e, sizeof(e));
    close(events[1]);
    exit(0);
  }
  close(ready[1]);
  close(high_gate[0]);
  close(low_gate[0]);
  char tokens[HIGH_WORKERS + 1];
  read_all(ready[0], tokens, sizeof(tokens));
  close(ready[0]);
  int epoch = uptime();
  if (spawn() == 0) {
    close(events[0]);
    close(high_gate[1]);
    close(low_gate[1]);
    int last = LOW_PRIORITY;
    while (uptime() - epoch < DEADLINE_TICKS) {
      int current = getpriority(low);
      if (current >= 0 && current < last) {
        struct event e = {3, uptime() - epoch, current};
        write_all(events[1], &e, sizeof(e));
        last = current;
      }
      pause(2);
    }
    struct event e = {2, uptime() - epoch, getpriority(low)};
    write_all(events[1], &e, sizeof(e));
    close(events[1]);
    exit(0);
  }
  close(events[1]);
  for (int i = 0; i < HIGH_WORKERS; i++)
    write_all(high_gate[1], "g", 1);
  close(high_gate[1]);
  write_all(low_gate[1], &epoch, sizeof(epoch));
  close(low_gate[1]);

  struct event e;
  do {
    read_all(events[0], &e, sizeof(e));
    if (e.kind == 3)
      printf("AGING promotion elapsed_ticks=%d observed_priority=%d\n", e.tick,
             e.priority);
  } while (e.kind == 3);
  close(events[0]);
  // Competitors are infinite CPU loops and are only killed after this event.
  // Thus success cannot be caused by finite high-priority jobs finishing.
  int alive = 0;
  for (int i = 0; i < HIGH_WORKERS; i++)
    if (getpriority(child_pids[i]) == 0)
      alive++;
  cleanup_children();
  check(setpriority(getpid(), original) == 0, "restore original priority");
  printf("AGING_RESULT elapsed_ticks=%d requested=%d observed=%d "
         "high_alive=%d timeout=%d\n",
         e.tick, LOW_PRIORITY, e.priority, alive, e.kind == 2);
  check(e.kind == 1, "timeout while high-priority competitors remained alive");
  check(alive == HIGH_WORKERS, "competitor exited before low process ran");
  check(e.priority == 0, "low process ran without observed promotion to zero");
  printf("agingtest: PASS\n");
  exit(0);
}
