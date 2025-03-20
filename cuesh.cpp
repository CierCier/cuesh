
#include "shell/shell.hpp"
int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  Shell shell = Shell();

  shell.start(); // this loops forever

  // return the error code if we exit the main loop;
  return shell.getError();
}