#ifndef __SHELL_HPP__
#define __SHELL_HPP__

#include <iostream>
#include <string>
#include <vector>
#include <stack>
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
#include <memory>

#include "Command.hpp"
#include "Utils.hpp"

enum ShellStatus
{
  SUCCESS,
  FAILURE,
  OPEN_FILE_FAILURE,
  CLOSE_FILE_FAILURE,
  READ_FAILURE,
  FILE_NOT_FOUND,
  SAME_SOURCE_N_TARGET,
  MEMORY_ALLOCATION_FAILURE
};

class Shell
{

private:
  Command<std::string, const std::string &> _echo;
  Command<std::string> _pwd;
  Command<int, const std::string &> _touch;
  Command<int, const std::string &> _mkdir;
  Command<int, const std::string &> _rmfile;
  Command<int, const std::string &> _rmdir;
  Command<std::vector<dirent *>, const std::string &, const std::string &> _ls;
  Command<int, const std::string &, const std::string &> _mv;
  Command<std::unique_ptr<std::string>, const std::string &, int &> _cat;

  void echoSetup()
  {
    _echo.setName("echo")
        .setDescription("Prints a message on the screen.")
        .setAction(
            [](const std::string &message) -> std::string
            {
              std::cout << message << '\n';
              return message;
            });
  }

  void pwdSetup()
  {
    _pwd.setName("pwd")
        .setDescription("Gets the current directory.")
        .setAction(
            []() -> std::string
            {
              const std::string &currentDir = get_current_dir_name();
              std::cout << currentDir << '\n';
              return currentDir;
            });
  }

  void touchSetup()
  {
    _touch.setName("touch")
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
  }

  void mkdirSetup()
  {
    _mkdir.setName("mkdir")
        .setDescription("Generate a new directory.")
        .setAction(
            [](const std::string &path) -> int
            {
              std::string p = "./";
              int status;

              if (path[0] == '/' or (path[0] == '.' and path[1] == '/'))
                p = "";

              for (auto &str : split(path, '/'))
              {
                p += str + "/";
                status = mkdir(p.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
              }

              if (status < 0)
                return FAILURE;
              return SUCCESS;
            });
  }

  void rmfileSetup()
  {
    _rmfile.setName("rmfile")
        .setDescription("Generate a new directory.")
        .setAction(
            [](const std::string &path) -> int
            {
              std::string newPath = path;

              if (not(path[0] == '/' or (path[0] == '.' and path[1] == '/')))
                newPath = "./" + path;

              if (!unlink(newPath.c_str()))
                return SUCCESS;
              else
                return FAILURE;
            });
  }

  void lsSetup()
  {
    _ls.setName("ls")
        .setDescription("Generate a new directory.")
        .setAction(
            [](const std::string &path, const std::string &mode) -> std::vector<dirent *>
            {
              DIR *dir = opendir(path.c_str());
              dirent *d;

              bool list = hasCaracter(mode, 'l');
              bool all = hasCaracter(mode, 'a');

              if (dir == nullptr)
                return {};

              std::vector<dirent *> dirs;

              while ((d = readdir(dir)) != nullptr)
              {

                if (!all and d->d_name[0] == '.')
                  continue;

                dirs.push_back(d);
              }

              std::sort(dirs.begin(), dirs.end(), [](const dirent *a, const dirent *b)
                        { return (std::string)a->d_name < b->d_name; });

              if (!hasCaracter(mode, 'x'))
              {
                for (dirent *dir : dirs)
                {
                  if (dir == nullptr)
                    continue;
                  std::cout << dir->d_name << (list ? '\n' : '\t');
                }

                if (!list)
                  std::cout << '\n';
              }

              return dirs;
            });
  }

  void rmdirSetup()
  {
    auto rmdirAction = [this](const std::string &_path) -> int
    {
      std::string path = _path;

      if (not(path[0] == '/' or (path[0] == '.' and path[1] == '/')))
        path = "./" + path;

      if (_ls.execute(path, "ax").size() > 2)
      {
        std::cout << "This directory contains files and/or directories. When you continue, they will all be removed.\n";
        std::cout << "Do you wish to continue [y/n]?\n";
        std::string res;
        std::cin >> res;

        if (res[0] == 'n')
          return SUCCESS;
      }

      std::vector<std::string> dirs;
      std::stack<std::string> dirsToRemove;
      dirs.push_back(path);

      while (!dirs.empty())
      {
        std::string current_dir = dirs.back();
        dirs.pop_back();

        auto items = _ls.execute(current_dir, "ax");

        dirsToRemove.push(current_dir);

        if (items.size() <= 2)
        {
          continue;
        }

        for (auto &item : items)
        {
          if (!strcmp(item->d_name, ".") or !strcmp(item->d_name, ".."))
            continue;

          std::string p = current_dir + "/" + item->d_name;

          struct stat st;

          if (lstat(p.c_str(), &st) < 0)
            return READ_FAILURE;

          if (S_ISDIR(st.st_mode))
          {
            dirs.push_back(p);
          }
          else
          {
            int status = _rmfile.execute(p);

            if (status != SUCCESS)
              return status;
          }
        }
      }

      while (!dirsToRemove.empty())
      {
        if (rmdir(dirsToRemove.top().c_str()) != 0)
          return FAILURE;
        dirsToRemove.pop();
      }

      return SUCCESS;
    };

    _rmdir.setName("rmdir")
        .setDescription("Remove a directory and its content.")
        .setAction(rmdirAction);
  }

  void mvSetup()
  {
    _mv.setName("mv")
        .setDescription("Move or rename a file or directory.")
        .setAction(
            [](const std::string &source, const std::string &target) -> int
            {
              struct stat source_sb;

              if (stat(source.c_str(), &source_sb) == -1)
                return FILE_NOT_FOUND;

              struct stat target_sb;

              if (stat(target.c_str(), &target_sb) != -1 && target_sb.st_ino == source_sb.st_ino)
                return SAME_SOURCE_N_TARGET;

              if (stat(target.c_str(), &target_sb) != -1)
              {

                if (S_ISDIR(target_sb.st_mode))
                {

                  const char *filename = strrchr(source.c_str(), '/');

                  if (!filename)
                    filename = source.c_str();
                  else
                    filename++;

                  char targetPath[source.size() + strlen(filename) + 2];

                  sprintf(targetPath, "%s/%s", target.c_str(), filename);

                  if (rename(source.c_str(), targetPath) == -1)
                    return FAILURE;
                }
                else if (rename(source.c_str(), target.c_str()) == -1)
                  return FAILURE;
              }
              else
              {
                if (rename(source.c_str(), target.c_str()) == -1)
                  return FAILURE;
              }

              return SUCCESS;
            });
  }

  void catSetup()
  {
    _cat.setName("cat")
        .setDescription("Displays the contents of a file in the shell.")
        .setAction(
            [](const std::string &filepath, int &status) -> std::unique_ptr<std::string>
            {
              int fd = open(filepath.c_str(), O_RDONLY);
              ssize_t nread, total = 0;
              char *buffer;

              if (fd < 0) {
                status = OPEN_FILE_FAILURE;
                return nullptr;
              }

              off_t fileSize = lseek(fd, 0, SEEK_END);
              lseek(fd, 0, SEEK_SET);

              buffer = (char *)malloc(fileSize);

              if (buffer == nullptr) {
                status = MEMORY_ALLOCATION_FAILURE;
                return nullptr;
              }

              // Lê o arquivo em um loop
              while ((nread = read(fd, buffer + total, fileSize - total)) > 0)
                total += nread;

              if (nread < 0) {
                status = READ_FAILURE;
                return nullptr;
              }

              if (buffer[fileSize - 1] == '\n')
                buffer[fileSize - 1] = '\0';

              std::unique_ptr<std::string> content = std::make_unique<std::string>(buffer);

              free(buffer);
              close(fd);

              return content;
            });
  }

public:
  bool setup()
  {
    bool status = false;

    this->echoSetup();
    this->pwdSetup();
    this->touchSetup();
    this->mkdirSetup();
    this->rmfileSetup();
    this->lsSetup();
    this->rmdirSetup();
    this->mvSetup();
    this->catSetup();

    return true;
  }

  int init()
  {
    return 0;
  }
};

#endif