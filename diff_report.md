##diff between commit acad53a and ad24ab7
```diff
diff --git a/.gitignore b/.gitignore
index 8d6e9be..2f410f8 100644
--- a/.gitignore
+++ b/.gitignore
@@ -13,4 +13,5 @@ initcode.out
 kernel
 kernelmemfs
 mkfs
-.gdbinit
\ No newline at end of file
+.gdbinit
+.vscode
\ No newline at end of file
```

```diff
diff --git a/xv6-pdx/Makefile b/xv6-pdx/Makefile
index acba3d9..36d1228 100644
--- a/xv6-pdx/Makefile
+++ b/xv6-pdx/Makefile
@@ -1,6 +1,6 @@
 # Set flag to correct CS333 project number: 1, 2, ...
 # 0 == original xv6-pdx distribution functionality
-CS333_PROJECT ?= 1
+CS333_PROJECT ?= 2
 PRINT_SYSCALLS ?= 0
 CS333_CFLAGS ?= -DPDX_XV6
 ifeq ($(CS333_CFLAGS), -DPDX_XV6)
 ```
 ```diff
diff --git a/xv6-pdx/defs.h b/xv6-pdx/defs.h
index f85557d..3e074c8 100644
--- a/xv6-pdx/defs.h
+++ b/xv6-pdx/defs.h
@@ -10,6 +10,10 @@ struct sleeplock;
 struct stat;
 struct superblock;
 
+#ifdef CS333_P2
+struct uproc;
+#endif
+
 // bio.c
 void            binit(void);
 struct buf*     bread(uint, uint);
@@ -124,6 +128,13 @@ void            userinit(void);
 int             wait(void);
 void            wakeup(void*);
 void            yield(void);
+
+#ifdef CS333_P2
+int             setuid(int * uid);
+int             setgid(int * gid);
+int             getprocs(uint max, struct uproc* table);
+#endif
+
 #ifdef CS333_P3
 void            printFreeList(void);
 void            printList(int);
 ```
 
```diff
diff --git a/xv6-pdx/proc.c b/xv6-pdx/proc.c
index 2fd0316..bacd30c 100644
--- a/xv6-pdx/proc.c
+++ b/xv6-pdx/proc.c
@@ -7,6 +7,10 @@
 #include "proc.h"
 #include "spinlock.h"
 
+#ifdef CS333_P2
+#include "uproc.h"
+#endif
+
 static char *states[] = {
   [UNUSED]    "unused",
   [EMBRYO]    "embryo",
@@ -128,6 +132,12 @@ allocproc(void)
   #ifdef CS333_P1
   p->start_ticks = ticks;
   #endif
+
+  #ifdef CS333_P2
+  p->cpu_ticks_in = 0;
+  p->cpu_ticks_total = 0;
+  #endif
+
   p->pid = nextpid++;
   release(&ptable.lock);
 
@@ -253,6 +263,10 @@ fork(void)
 
   acquire(&ptable.lock);
   np->state = RUNNABLE;
+  #ifdef CS333_P2
+  np->gid = curproc -> gid;
+  np->uid = curproc -> uid;
+  #endif
   release(&ptable.lock);
 
   return pid;
@@ -395,6 +409,10 @@ scheduler(void)
       swtch(&(c->scheduler), p->context);
       switchkvm();
 
+      #ifdef CS333_P2
+      p->cpu_ticks_in = ticks;
+      #endif
+
       // Process is done running for now.
       // It should have changed its p->state before coming back.
       c->proc = 0;
@@ -423,6 +441,12 @@ sched(void)
   int intena;
   struct proc *p = myproc();
 
+  #ifdef CS333_P2
+  if (p->state != SLEEPING) 
+    p->cpu_ticks_total += (ticks - p->cpu_ticks_in);
+  #endif
+  
+
   if(!holding(&ptable.lock))
     panic("sched ptable.lock");
   if(mycpu()->ncli != 1)
@@ -559,7 +583,41 @@ kill(int pid)
 void
 procdumpP2P3P4(struct proc *p, char *state_string)
 {
-  cprintf("TODO for Project 2, delete this line and implement procdumpP2P3P4() in proc.c to print a row\n");
+  int elapsed_time_ms = ticks - p->start_ticks;
+  int elapsed_time_s = elapsed_time_ms/1000;
+  elapsed_time_ms = elapsed_time_ms % 1000;
+  char* zeros = "";
+  if(elapsed_time_ms < 100 && elapsed_time_ms >= 10){
+    zeros = "0";
+  }
+  if(elapsed_time_ms < 10){
+    zeros = "00";
+  }
+  int cpu_sec = p->cpu_ticks_total / 1000;
+  int cpu_ms  = p->cpu_ticks_total % 1000;
+  char* cpu_zeros = "";
+  if(cpu_ms < 100 && cpu_ms >= 10){
+    cpu_zeros = "0";
+  }
+  if(cpu_ms < 10){
+    cpu_zeros = "00";
+  }  
+
+  cprintf("%d\t%s\t\t%d\t%d\t%d\t%d.%s%d\t%d.%s%d\t%s\t%d\t", p->pid, 
+                                                            p->name,
+                                                            p->uid,
+                                                            p->gid,
+                                                            p->parent ? p->parent->pid : p->pid,
+                                                           
+                                                            elapsed_time_s,
+                                                            zeros,
+                                                            elapsed_time_ms,
+                                                            
+                                                            cpu_sec,
+                                                            cpu_zeros,
+                                                            cpu_ms,
+                                                             states[p->state],
+                                                            p->sz);
   return;
 }
 #elif defined(CS333_P1)
@@ -935,3 +993,48 @@ checkProcs(const char *file, const char *func, int line)
 }
 #endif // DEBUG
 
+#ifdef CS333_P2
+int 
+setuid(int * uid){
+  acquire(&ptable.lock);
+  myproc()->uid = * uid;
+  release(&ptable.lock);
+  return 0;
+}
+
+int 
+setgid(int * gid){
+  acquire(&ptable.lock);
+  myproc()->gid = * gid;
+  release(&ptable.lock);
+  return 0;
+}
+
+int
+getprocs(uint max, struct uproc* table){
+  struct proc* p;
+  int num_proc = 0;
+  acquire(&ptable.lock);
+  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
+    if(num_proc == max)
+      break;
+    if(p->state == UNUSED||p->state == EMBRYO)
+      continue;
+    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
+      safestrcpy(table[num_proc].state, states[p->state],STRMAX);
+    else
+      safestrcpy(table[num_proc].state,"???",STRMAX);
+    table[num_proc].pid = p->pid;
+    table[num_proc].uid = p->uid;
+    table[num_proc].gid = p->gid;
+    table[num_proc].ppid = p->parent ? p->parent->pid : p->pid;
+    table[num_proc].elapsed_ticks = ticks - p->start_ticks;;
+    table[num_proc].CPU_total_ticks = p->cpu_ticks_total;
+    table[num_proc].size = p->sz;
+    safestrcpy(table[num_proc].name,p->name,STRMAX);
+    num_proc++;
+  }
+  release(&ptable.lock);
+  return num_proc;
+}
+#endif
\ No newline at end of file
```
```diff
diff --git a/xv6-pdx/proc.h b/xv6-pdx/proc.h
index b097511..b2697f9 100644
--- a/xv6-pdx/proc.h
+++ b/xv6-pdx/proc.h
@@ -49,9 +49,18 @@ struct proc {
   struct file *ofile[NOFILE];  // Open files
   struct inode *cwd;           // Current directory
   char name[16];               // Process name (debugging)
-  #ifdef CS333_P1
+  
+  #if defined(CS333_P1) || defined(CS333_P2)
   uint start_ticks;
   #endif
+  
+  #ifdef CS333_P2
+  uint cpu_ticks_total ;
+  uint cpu_ticks_in ;
+  uint uid;
+  uint gid;
+  #endif
+
 };
 // Process memory is laid out contiguously, low addresses first:
 ```
 
 ```diff
diff --git a/xv6-pdx/ps.c b/xv6-pdx/ps.c
new file mode 100644
index 0000000..e07a029
--- /dev/null
+++ b/xv6-pdx/ps.c
@@ -0,0 +1,53 @@
+
+#include "types.h"
+#include "user.h"
+#include "uproc.h"
+
+#define MAX 16
+int
+main(void)
+{
+  struct uproc *proc = malloc(sizeof(struct uproc)*MAX);
+  int num_procs = getprocs(MAX,proc);
+  printf(1,"PID\tName\t\tUID\tGID\tPPID\tElapsed\tCPU\tState\tSize\n");
+  int i;
+  for(i = 0; i<num_procs;i++){
+
+    uint elapsed_time_ms = proc[i].elapsed_ticks;
+    uint elapsed_time_s = elapsed_time_ms / 1000;
+    elapsed_time_ms = elapsed_time_ms % 1000;
+
+    char* zeros = "";
+
+    if(elapsed_time_ms < 100 && elapsed_time_ms >= 10){
+      zeros = "0";
+    }
+    if(elapsed_time_ms < 10){
+      zeros = "00";
+    }
+
+    uint cpu_time_ms = proc[i].CPU_total_ticks;
+    uint cpu_time_s = cpu_time_ms / 1000;
+    cpu_time_ms = cpu_time_ms % 1000;
+
+    char* cpu_zeros = "";
+
+    if(cpu_time_ms < 100 && cpu_time_ms >= 10){
+      cpu_zeros = "0";
+    }
+    
+    if(cpu_time_ms < 10){
+      cpu_zeros = "00";
+    }
+
+    printf(1,"%d\t%s\t\t%d\t%d\t%d\t%d.%s%d\t%d.%s%d\t%s\t%d\n",
+            proc[i].pid,proc[i].name,proc[i].uid,proc[i].gid,
+            proc[i].ppid,elapsed_time_s,zeros,elapsed_time_ms,
+            cpu_time_s,cpu_zeros,cpu_time_ms,proc[i].state, proc[i].size );
+    
+  }
+
+  free(proc);
+  
+  exit();
+}
\ No newline at end of file
```
```diff
diff --git a/xv6-pdx/runoff.list b/xv6-pdx/runoff.list
index 81930d9..4a3b852 100644
--- a/xv6-pdx/runoff.list
+++ b/xv6-pdx/runoff.list
@@ -87,8 +87,11 @@ date.h
 date.c
 uproc.h
 testuidgid.c
+testsetuid.c
 testuidgid.txt
 p2-test.c
 p3-test.c
 p4-test.c
 testsetprio.c
+time.c
+ps.c
\ No newline at end of file
```
```diff
diff --git a/xv6-pdx/syscall.c b/xv6-pdx/syscall.c
index 94d1df8..091a159 100644
--- a/xv6-pdx/syscall.c
+++ b/xv6-pdx/syscall.c
@@ -112,6 +112,15 @@ extern int sys_halt(void);
 extern int sys_date(void);
 #endif
 
+#ifdef CS333_P2
+extern int sys_getuid(void);
+extern int sys_getgid(void);
+extern int sys_getppid(void);
+extern int sys_setuid(void);
+extern int sys_setgid(void);
+extern int sys_getprocs(void);
+#endif
+
 static int (*syscalls[])(void) = {
 [SYS_fork]    sys_fork,
 [SYS_exit]    sys_exit,
@@ -137,6 +146,16 @@ static int (*syscalls[])(void) = {
 #ifdef CS333_P1
 [SYS_date]    sys_date,
 #endif
+
+#ifdef CS333_P2
+[SYS_getuid]  sys_getuid,
+[SYS_getgid]  sys_getgid,
+[SYS_getppid] sys_getppid,
+[SYS_setuid]  sys_setuid,
+[SYS_setgid]  sys_setgid,
+[SYS_getprocs] sys_getprocs,
+#endif
+
 #ifdef PDX_XV6
 [SYS_halt]    sys_halt,
 #endif // PDX_XV6
 ```
 ```diff
diff --git a/xv6-pdx/syscall.h b/xv6-pdx/syscall.h
index 469f707..86a746d 100644
--- a/xv6-pdx/syscall.h
+++ b/xv6-pdx/syscall.h
@@ -22,4 +22,11 @@
 #define SYS_close   SYS_mkdir+1
 #define SYS_halt    SYS_close+1
 #define SYS_date    SYS_halt+1
+#define SYS_getuid  SYS_date+1
+#define SYS_getgid  SYS_getuid+1
+#define SYS_getppid SYS_getgid+1
+#define SYS_setuid  SYS_getppid+1
+#define SYS_setgid  SYS_setuid+1
+#define SYS_getprocs SYS_setgid+1
+
 // student system calls begin here. Follow the existing pattern.
 ```
 ```diff
diff --git a/xv6-pdx/sysproc.c b/xv6-pdx/sysproc.c
index c7ef177..c3e0d6d 100644
--- a/xv6-pdx/sysproc.c
+++ b/xv6-pdx/sysproc.c
@@ -6,6 +6,11 @@
 #include "memlayout.h"
 #include "mmu.h"
 #include "proc.h"
+
+#ifdef CS333_P2
+#include "uproc.h"
+#endif
+
 #ifdef PDX_XV6
 #include "pdx-kernel.h"
 #endif // PDX_XV6
@@ -113,3 +118,58 @@ int sys_date(void)
   return 0;
 }
 #endif
+
+
+#ifdef CS333_P2
+int 
+sys_getuid (void) {
+  return myproc()->uid;
+}
+
+int 
+sys_getgid (void) {
+  return myproc()->gid;
+}
+
+int 
+sys_getppid(void) {
+  return myproc()->parent != 0 ? myproc()->parent->pid : 0;
+}
+
+int 
+sys_setuid(void) {
+  int uid;
+  argint(0, &uid);
+  if (uid < 0 || uid > 32767) return -1;
+	setuid(&uid);
+
+	return 0;
+}
+
+int 
+sys_setgid(void) {
+  int gid;
+  argint(0, &gid);
+	if (gid < 0 || gid > 32767) return -1;
+		
+		
+	setgid(&gid);
+	return 0;
+}	
+
+int sys_getprocs(void) {
+  uint max;
+  struct uproc* table;
+  if(argint(0,(int *)&max) < 0){
+    return -1;
+  }
+  if(max != 1 && max != 16 && max != 64 && max != 72){
+    return -1;
+  }
+  if(argptr(1, (void*)&table, sizeof(struct uproc)) < 0){
+    return -1;
+  }
+  int num_procs = getprocs(max,table);
+  return num_procs;
+}
+#endif
\ No newline at end of file
```
```diff
diff --git a/xv6-pdx/testsetuid.c b/xv6-pdx/testsetuid.c
new file mode 100644
index 0000000..c43fa2a
--- /dev/null
+++ b/xv6-pdx/testsetuid.c
@@ -0,0 +1,11 @@
+#ifdef CS333_P2
+#include "types.h"
+#include "user.h"
+
+int
+main(int argc, char *argv[])
+{
+  printf(1, "***** In %s: my uid is %d\n\n", argv[0], getuid());
+  exit();
+}
+#endif
```
```diff
diff --git a/xv6-pdx/time.c b/xv6-pdx/time.c
new file mode 100644
index 0000000..e88a0b8
--- /dev/null
+++ b/xv6-pdx/time.c
@@ -0,0 +1,41 @@
+#include "types.h"
+#include "user.h"
+
+int
+main(int argc, char *argv[]) {
+
+  if (argc == 1) {
+    printf(1, "(null) ran in 0.00 seconds\n");
+  } 
+
+  else {
+    int start = uptime();
+    int pid = fork();
+
+    if (pid == 0){
+      if (exec(argv[1], &argv[1]) < 0) {
+        printf(2, "unknown exec command\n");
+        kill(getppid());
+        exit();
+      }
+    }
+
+    else if (pid == -1) printf(1, "error fork");
+
+    else wait();
+
+    int end = uptime();
+    int running_time = end - start;
+    int mod = running_time % 1000;
+
+    char * lead_zeros = "";
+
+    if (mod < 100) lead_zeros = "0";
+    
+    if (mod < 10) lead_zeros = "00";
+
+    printf(1,"%s ran in %d.%s%d\n", argv[1], running_time / 1000 , lead_zeros, mod);
+  }
+
+  exit();
+}
```
```diff
diff --git a/xv6-pdx/uproc.h b/xv6-pdx/uproc.h
index 997ec25..bfb8231 100644
--- a/xv6-pdx/uproc.h
+++ b/xv6-pdx/uproc.h
@@ -1,3 +1,5 @@
+#include "types.h"
+
 #ifndef UPROC_H
 #define UPROC_H
 #define STRMAX 32
@@ -7,9 +9,9 @@ struct uproc {
   uint uid;
   uint gid;
   uint ppid;
-#ifdef CS333_P4
-  uint priority;
-#endif // CS333_P4
+  #ifdef CS333_P4
+    uint priority;
+  #endif // CS333_P4
   uint elapsed_ticks;
   uint CPU_total_ticks;
   char state[STRMAX];
   ```
   ```diff
diff --git a/xv6-pdx/user.h b/xv6-pdx/user.h
index d1d0b6c..1a72a8b 100644
--- a/xv6-pdx/user.h
+++ b/xv6-pdx/user.h
@@ -29,6 +29,16 @@ int halt(void);
 int date(struct rtcdate*);
 #endif
 
+
+#ifdef CS333_P2
+int getprocs(uint max , struct uproc * table);
+int getuid (void);
+int getgid (void);
+int getppid(void);
+int setuid(int);
+int setgid(int);
+#endif
+
 // ulib.c
 int stat(char*, struct stat*);
 char* strcpy(char*, char*);
 ```
 ```diff
diff --git a/xv6-pdx/usys.S b/xv6-pdx/usys.S
index e28e4e5..b164142 100644
--- a/xv6-pdx/usys.S
+++ b/xv6-pdx/usys.S
@@ -30,4 +30,11 @@ SYSCALL(sbrk)
 SYSCALL(sleep)
 SYSCALL(uptime)
 SYSCALL(halt)
-SYSCALL(date)
\ No newline at end of file
+SYSCALL(date)
+
+SYSCALL(getuid)
+SYSCALL(getgid)
+SYSCALL(getppid)
+SYSCALL(setuid)
+SYSCALL(setgid)
+SYSCALL(getprocs)
\ No newline at end of file
```