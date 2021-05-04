# Changes Log


## syscall.c
at line 182-188
```c
    int temp = syscalls[num]();
    #ifdef CS333_P1
      #ifdef PRINT_SYSCALLS
        cprintf("%s->%d\n", syscallnames[num], temp);
      #endif
    #endif
    curproc->tf->eax = temp;
```

## user.h
at line 28-30
```c
#ifdef CS333_P1
int date(struct rtcdate*);
#endif
```

## usys.S
at line 33
```c
SYSCALL(date)
```

## syscall.h
at line 24
```c
#define SYS_date    SYS_halt+1
```

## syscall.c
at line 111-113
```c
#ifdef CS333_P1
extern int sys_date(void);
#endif
```

at line 137-139
```c
#ifdef CS333_P1
[SYS_date]    sys_date,
#endif
```

## sysproc.c
at line 100-115
```c
#ifdef CS333_P1
int sys_date(void)
{
  struct rtcdate *d;
  if (argptr(0, (void *)&d, sizeof(struct rtcdate)) < 0)
  {

    return -1;
  }
  else
  {
    cmostime(d);
  }
  return 0;
}
#endif
```

## proc.c
at line 128-130
```c
  #ifdef CS333_P1
  p->start_ticks = ticks;
  #endif
```

at line 566-588
```c
void
procdumpP1(struct proc *p, char *state_string)
{
  uint elapsed = ticks - p->start_ticks;
  uint ms = elapsed % 1000;

  cprintf("%d\t%s\t", p->pid, p->name);
  cprintf("     %d", elapsed / 1000);

  // add leading zero
  if (ms < 10) {
     cprintf(".00%d\t", ms);
   
  } else if (ms < 100) {
     cprintf(".0%d\t", ms);
  } else {
    cprintf(".%d\t", ms);
  }
  
  cprintf("%s\t%d\t", states[p->state], p->sz);

  return;
}
```