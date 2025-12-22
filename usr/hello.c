// simple demo user program to run "like xv6" via shell+fork

#include "user.h"

// Example user program entry for exec("hello")
void hello_main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  sys_write(1, "Hello from user program hello!\n", 34);
  // Important: programs started via exec must terminate explicitly
  sys_exit(0);
}
