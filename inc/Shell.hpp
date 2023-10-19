#ifndef __SHELL_HPP__
#define __SHELL_HPP__

#include <iostream>

#include "Command.hpp"

class Shell
{

private:
  Command<std::string, const std::string &> echo;

public:

  bool setup()
  {
    bool status = false;

    echo.setName("echo")
      .setDescription("Prints a message on the screen.")
      .setAction(
        [](const std::string& message) -> std::string
        {
          std::cout << message << '\n';
          return message;
        }
      );

    return true;
  }

  int init()
  {
    this->echo.execute("Andel");

    return 0;
  }
};

#endif