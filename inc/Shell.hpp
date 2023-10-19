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
#include "Utils.hpp"

enum ShellStatus
{
  SUCCESS,
  FAILURE,
  OPEN_FILE_FAILURE,
  CLOSE_FILE_FAILURE
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

public:
  bool setup()
  {
    bool status = false;

    _echo.setName("echo")
        .setDescription("Prints a message on the screen.")
        .setAction(
            [](const std::string &message) -> std::string
            {
              std::cout << message << '\n';
              return message;
            });

    _pwd.setName("pwd")
        .setDescription("Gets the current directory.")
        .setAction(
            []() -> std::string
            {
              const std::string &currentDir = get_current_dir_name();
              std::cout << currentDir << '\n';
              return currentDir;
            });

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


              while ( (d = readdir(dir)) != nullptr )
              {

                if (!all and d->d_name[0] == '.')
                  continue;

                dirs.push_back(d);
              }

              std::sort(dirs.begin(), dirs.end(), [](const dirent *a, const dirent *b)
                        { return (std::string)a->d_name < b->d_name; });


              for (dirent* dir: dirs) {
                if ( dir == nullptr ) continue;
                std::cout << dir->d_name << (list ?'\n' :'\t');
              }

              if ( !list ) std::cout << '\n';

              return dirs;
            });

    // _rmdir.setName("rmdir")
    //     .setDescription("Generate a new directory.")
    //     .setAction(
    //         [](const std::string &_path) -> int
    //         {
    //           std::string path = _path;

    //           if (not(path[0] == '/' or (path[0] == '.' and path[1] == '/')))
    //             path = "./" + path;

    //           auto itens = getItensOfDirectory(path, true);

    //           // O diretório não está vazio
    //           if (itens.size() > 2)
    //           {

    //             struct stat st;

    //             for (auto &item : itens)
    //             {

    //               if (!strcmp(item->d_name, ".") or !strcmp(item->d_name, ".."))
    //                 continue;

    //               std::string p = path + "/" + item->d_name;

    //               if (lstat(p.c_str(), &st) < 0)
    //                 return READ_FAILURE;

    //               if (S_ISDIR(st.st_mode) and removeDirectory(p) != EXIT_SUCCESS)
    //                 return EXIT_FAILURE;

    //               int status = removeFile(p);

    //               if (status == EXIT_SUCCESS)
    //                 continue;
    //               else
    //                 return status;
    //             }
    //           }

    //           if (rmdir(path.c_str()) != 0)
    //             return EXIT_FAILURE;
    //           return EXIT_SUCCESS;
    //         });

    return true;
  }

  int init()
  {
    return 0;
  }
};

#endif