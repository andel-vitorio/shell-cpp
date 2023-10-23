#include <iostream>
#include "Shell.hpp"

int main () {
  Shell shell;
  shell.setup();
  if ( shell.init() == SUCCESS ) 
    std::cout << "\nShell finished successfully.\n";
	return 0;
}