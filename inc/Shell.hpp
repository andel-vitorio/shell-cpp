#ifndef __SHELL_HPP__
#define __SHELL_HPP__

#include <iostream>

#include "Command.hpp"

// Função de execução para o comando "echo"
std::string echo_func(const std::string &message)
{
  return "echo: " + message;
}

class Shell
{

public:
  int init()
  {
    Command<std::string, const std::string &> echo_command("echo", "Prints a message", echo_func);
    std::string echo_result = echo_command.execute("Hello, World!");
    std::cout << echo_result << std::endl;
    return 0;
  }
};

#endif