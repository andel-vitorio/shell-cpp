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

enum ShellStatus {
  SUCCESS, FAILURE, OPEN_FILE_FAILURE, CLOSE_FILE_FAILURE
};

class Shell
{

private:
  Command<std::string, const std::string &> echo;
  Command<std::string> pwd;
  Command<int, const std::string &> touch;

public:
  bool setup()
  {
    bool status = false;

    echo.setName("echo")
        .setDescription("Prints a message on the screen.")
        .setAction(
          [](const std::string &message) -> std::string
          {
            std::cout << message << '\n';
            return message;
          });

    pwd.setName("pwd")
        .setDescription("Gets the current directory.")
        .setAction(
          []() -> std::string
          {
            const std::string &currentDir = get_current_dir_name();
            std::cout << currentDir << '\n';
            return currentDir;
          });

    touch.setName("touch")
        .setDescription("Generates a blank file.")
        .setAction(
          [](const std::string &filename) -> int
          {
            mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            int fd = open(filename.c_str(), O_WRONLY | O_CREAT, mode);

            if (fd < 0)
              return OPEN_FILE_FAILURE;

            if (close(fd) < 0)
              return CLOSE_FILE_FAILURE;

            return SUCCESS;
          });

    return true;
  }

  int init() {
    
    return 0;
  }
};

#endif