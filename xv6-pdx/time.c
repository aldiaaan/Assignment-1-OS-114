#include "types.h"
#include "user.h"

int
main(int argc, char *argv[]) {

  if (argc == 1) {
    printf(1, "(null) ran in 0.00 seconds\n");
  } 

  else {
    int start = uptime();
    int pid = fork();

    if (pid == 0){
      if (exec(argv[1], &argv[1]) < 0) {
        printf(2, "unknown exec command\n");
        kill(getppid());
        exit();
      }
    }

    else if (pid == -1) printf(1, "error fork");

    else wait();

    int end = uptime();
    int running_time = end - start;
    int mod = running_time % 1000;

    char * lead_zeros = "";

    if (mod < 100) lead_zeros = "0";
    
    if (mod < 10) lead_zeros = "00";

    printf(1,"%s ran in %d.%s%d\n", argv[1], running_time / 1000 , lead_zeros, mod);
  }

  exit();
}
