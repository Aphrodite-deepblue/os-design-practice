// Experimental fixture: original xv6 scheduler from baseline dce44e4.
// Only used in a temporary comparison copy, never linked into the main build.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  for (;;) {
    intr_on();
    intr_off();
    int found = 0;
    for (p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->state == RUNNABLE) {
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);
        mycpu()->intena = 0;
        c->proc = 0;
        found = 1;
      }
      release(&p->lock);
    }
    if (found == 0)
      asm volatile("wfi");
  }
}
