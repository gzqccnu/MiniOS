/*
 * Lrix
 * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
 * 
 * This program is released under the terms of the GNU General Public License version 2(GPLv2).
 * See https://opensource.org/licenses/GPL-2.0 for more information.
 * 
 * Project homepage: https://github.com/lrisguan/Lrix
 * Description: A scratch implemention of OS based on RISC-V
 */

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
