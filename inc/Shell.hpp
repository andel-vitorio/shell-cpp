#ifndef __SHELL_HPP__
#define __SHELL_HPP__

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <regex>
#include <iomanip>
#include <map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "Command.hpp"

class Shell
{

private:
  Command<std::string, const std::string &> echo;
  Command<std::string> pwd;

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

    auto pwdAction = [this]() -> std::string
    {
      const std::string& currentDir = get_current_dir_name();
      this->echo.execute(currentDir);
      return currentDir;
    };

    std::function<std::string()> pwdFunction = [pwdAction]() {
        return pwdAction();
    };

    pwd.setName("pwd")
      .setDescription("Gets the current directory.")
      .setAction(pwdFunction);

    return true;
  }

  int init()
  {
    this->echo.execute("Andel");
    this->pwd.execute();

    return 0;
  }
};

#endif