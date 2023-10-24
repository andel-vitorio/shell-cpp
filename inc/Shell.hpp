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
#include <fstream>

#include "Command.hpp"
#include "Utils.hpp"
#include "IO.hpp"

#define INPUT_REDIRECTION_SYMBOL '<'
#define OUTPUT_REDIRECTION_SYMBOL '>'

enum ShellStatus
{
  SUCCESS,
  FAILURE,
  OPEN_FILE_FAILURE,
  CLOSE_FILE_FAILURE,
  READ_FAILURE,
  FILE_NOT_FOUND,
  SAME_SOURCE_N_TARGET,
  MEMORY_ALLOCATION_FAILURE,
  QUIT_COMMAND
};

class Shell
{

private:
  bool isRunning = false;
  IO io;

  std::vector<pid_t> childProcesses;

  Command<std::string, const std::string &> $echo;
  Command<int> $exit;
  Command<std::string> $pwd;
  Command<std::string> $hostname;
  Command<std::string> $username;
  Command<int, const std::string &> $touch;
  Command<int, const std::string &> $mkdir;
  Command<int, const std::string &> $rmfile;
  Command<int, const std::string &> $rmdir;
  Command<std::vector<dirent *>, const std::string &, const std::string &> $ls;
  Command<int, const std::string &, const std::string &> $mv;
  Command<std::unique_ptr<std::string>, const std::string &, const bool &, int &> $cat;
  Command<int, const std::string &> $cd;
  Command<std::unique_ptr<std::vector<std::string>>, const std::string &, const bool &, const std::string &, int &> $grep;
  Command<int, const pid_t &> $kill;

  inline void printPrompt() { std::cout << this->$hostname.execute() << '@' << this->$username.execute() << ":~$ "; }

  inline void inputRedirection(std::string &args)
  {
    std::string inputStream = STDIN_STREAM;

    std::regex pattern("<\\s*(\"([^\"]*)\"|([^\\s]+))");

    std::smatch match;
    if (std::regex_search(args, match, pattern))
    {
      if (!match[2].str().empty())
        inputStream = match[2].str();
      else
        inputStream = match[3].str();
      args = std::regex_replace(args, pattern, "");
    }
    else
      std::cerr << "Invalid parameter for INPUT_REDIRECTION_SYMBOL." << std::endl;

    if (inputStream != STDIN_STREAM)
    {
      if (io.setInputStream(inputStream) == INPUT_STREAM_FAIL)
      {
        std::cerr << "Input file not found.\n\n";
        return;
      }
    }
    else
      io.setInputStream(STDIN_STREAM);
  }

  inline void outputRedirection(std::string &args)
  {
    std::string outputStream = STDOUT_STREAM;
    std::regex pattern(">\\s*(\"([^\"]*)\"|([^\\s]+))");

    std::smatch match;
    if (std::regex_search(args, match, pattern))
    {
      if (!match[2].str().empty())
        outputStream = match[2].str();
      else
        outputStream = match[3].str();

      args = std::regex_replace(args, pattern, "");
    }
    else
      std::cerr << "Invalid parameter for OUTPUT_REDIRECTION_SYMBOL." << std::endl;

    if (io.setOutputStream(outputStream) == OUTPUT_STREAM_FAIL)
    {
      std::cerr << "Output file not found.\n\n";
      return;
    }
  }

  std::vector<std::string> getItemsName(const std::string &text)
  {
    std::regex argPattern("\"([^\"]+)\"|([^\\s]+)");
    std::vector<std::string> args;

    std::sregex_iterator it(text.begin(), text.end(), argPattern);
    std::sregex_iterator end;

    while (it != end)
    {
      std::smatch match = *it;
      std::string value;

      if (match[1].matched)
        value = match.str(1);
      else
        value = match.str(2);

      args.push_back(value);
      ++it;
    }

    return args;
  }

  void exitSetup()
  {
    $exit.setName("exit")
        .setDescription("Quit shell.")
        .setAction(
            [this]() -> int
            {
              isRunning = false;
              return SUCCESS;
            });
  }

  void echoSetup()
  {
    $echo.setName("echo")
        .setDescription("Prints a message on the screen.")
        .setAction(
            [this](const std::string &message) -> std::string
            {
              this->io.setOutputLine(message);
              return message;
            });
  }

  void pwdSetup()
  {
    $pwd.setName("pwd")
        .setDescription("Gets the current directory.")
        .setAction(
            [this]() -> std::string
            {
              const std::string &currentDir = get_current_dir_name();
              this->io.setOutputLine(currentDir);
              return currentDir;
            });
  }

  void hostnameSetup()
  {
    $hostname.setName("hostname")
        .setDescription("Gets the name of the current device.")
        .setAction(
            []() -> std::string
            {
              char hostname[HOST_NAME_MAX + 1];
              gethostname(hostname, HOST_NAME_MAX + 1);
              return hostname;
            });
  }

  void usernameSetup()
  {
    $username.setName("username")
        .setDescription("Gets the name of the current user.")
        .setAction(
            []() -> std::string
            {
              return std::string(getlogin());
            });
  }

  void touchSetup()
  {
    $touch.setName("touch")
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
    $mkdir.setName("mkdir")
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
    $rmfile.setName("rmfile")
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
    $ls.setName("ls")
        .setDescription("Generate a new directory.")
        .setAction(
            [this](const std::string &path, const std::string &mode) -> std::vector<dirent *>
            {
              DIR *dir = opendir(path.c_str());
              dirent *d;

              bool list = contains(mode, 'l');
              bool all = contains(mode, 'a');

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

              if (!contains(mode, 'x'))
              {
                for (dirent *dir : dirs)
                {
                  if (dir == nullptr)
                    continue;

                  if (list)
                    this->io.setOutputLine(std::string(dir->d_name));
                  else
                    this->io.setOutput(std::string(dir->d_name) + "\t");
                }

                if (!list)
                  this->io.setOutputLine("");
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

      if ($ls.execute(path, "ax").size() > 2)
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

        auto items = $ls.execute(current_dir, "ax");

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
            int status = $rmfile.execute(p);

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

    $rmdir.setName("rmdir")
        .setDescription("Remove a directory and its content.")
        .setAction(rmdirAction);
  }

  void mvSetup()
  {
    $mv.setName("mv")
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
    $cat.setName("cat")
        .setDescription("Displays the contents of a file in the shell.")
        .setAction(
            [this](const std::string &filepath, const bool &print = true, int &status) -> std::unique_ptr<std::string>
            {
              int fd = open(filepath.c_str(), O_RDONLY);
              ssize_t nread, total = 0;
              char *buffer;

              if (fd < 0)
              {
                status = OPEN_FILE_FAILURE;
                return nullptr;
              }

              off_t fileSize = lseek(fd, 0, SEEK_END);
              lseek(fd, 0, SEEK_SET);

              buffer = (char *)malloc(fileSize);

              if (buffer == nullptr)
              {
                status = MEMORY_ALLOCATION_FAILURE;
                return nullptr;
              }

              // Lê o arquivo em um loop
              while ((nread = read(fd, buffer + total, fileSize - total)) > 0)
                total += nread;

              if (nread < 0)
              {
                status = READ_FAILURE;
                return nullptr;
              }

              if (buffer[fileSize - 1] == '\n')
                buffer[fileSize - 1] = '\0';

              std::unique_ptr<std::string> content = std::make_unique<std::string>(buffer);

              if (print)
                this->io.setOutputLine(content->c_str());

              free(buffer);
              close(fd);

              status = SUCCESS;

              return content;
            });
  }

  void cdSetup()
  {
    $cd.setName("cd")
        .setDescription("Changes the current directory.")
        .setAction(
            [](const std::string &path) -> int
            {
              return chdir(path.c_str());
            });
  }

  void grepSetup()
  {
    auto grepAction = [this](const std::string &source, const bool &sourceIsFile, const std::string &pattern, int &status) -> std::unique_ptr<std::vector<std::string>>
    {
      std::unique_ptr<std::string> content;

      if (sourceIsFile)
        content = $cat.execute(source, false, status);
      else
        content = std::make_unique<std::string>(source);

      if (content)
      {
        auto lines = split(*content, '\n');

        std::unique_ptr<std::vector<std::string>> ans = std::make_unique<std::vector<std::string>>();

        if (lines.size() == 1)
          lines = split(*content, ' ');

        for (auto &str : lines)
        {
          if (str.find(pattern) != std::string::npos)
          {
            ans->push_back(str);
            this->io.setOutputLine(str);
          }
        }

        status = SUCCESS;
        return ans;
      }

      return nullptr;
    };

    $grep.setName("grep")
        .setDescription("Searches for the location of a word in a file.")
        .setAction(grepAction);
  }

  void killSetup()
  {
    $kill.setName("kill")
        .setDescription("Terminate a process by PID.")
        .setAction(
            [this](const pid_t &pid) -> int
            {
              return kill(pid, SIGTERM);
            });
  }

  void echo(std::string &args)
  {
    std::string msg = "";
    std::istringstream iss;

    if (contains(args, INPUT_REDIRECTION_SYMBOL))
      inputRedirection(args);

    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      outputRedirection(args);

    if (io.isStdinStream())
    {
      iss = std::istringstream(args);
      io.setInputStream(iss);
    }

    msg = this->io.getAllInputLines();

    io.setOutputLine(trim(msg));

    io.setOutputStream(STDOUT_STREAM);
    io.setInputStream(STDIN_STREAM);
  }

  void cat(std::string &args)
  {
    std::vector<std::string> items = this->getItemsName(args);
    std::string msg = "";

    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      outputRedirection(args);

    if (items.size() > 0)
    {
      int status;

      this->$cat.execute(trim(items[0]), true, status);

      switch (status)
      {
      case SUCCESS:
        break;

      case OPEN_FILE_FAILURE:
        std::cout << "Failed to open file.\n";
        break;

      case READ_FAILURE:
        std::cout << "Failed to read file.\n";
        break;

      case MEMORY_ALLOCATION_FAILURE:
        std::cout << "Memory allocation failure.\n";
        break;

      default:
        std::cout << "Failed to execute the command.\n";
        break;
      }
    }

    io.setOutputStream(STDOUT_STREAM);
    puts("");
  }

  void grep(std::string &args, bool fromPipeline = false)
  {
    std::vector<std::string> argsList = this->getItemsName(args);
    int status;

    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      outputRedirection(args);

    if (fromPipeline)
    {
      std::string content;

      while (!this->io.isEof())
        content += this->io.getInputLine() + '\n';

      this->$grep.execute(content, false, trim(argsList[0]), status);
    }
    else
    {
      if (argsList.size() > 1)
        this->$grep.execute(trim(argsList[0]), true, trim(argsList[1]), status);
      else
        std::cerr << "Not enough parameters!\n";
    }

    switch (status)
    {
    case SUCCESS:
      break;

    case OPEN_FILE_FAILURE:
      std::cout << "Failed to open file.\n";
      break;

    case READ_FAILURE:
      std::cout << "Failed to read file.\n";
      break;

    case MEMORY_ALLOCATION_FAILURE:
      std::cout << "Memory allocation failure.\n";
      break;

    default:
      std::cout << "Failed to execute the command.\n";
      break;
    }

    io.setOutputStream(STDOUT_STREAM);
    puts("");
  }

  void pwd(std::string &args)
  {
    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      outputRedirection(args);
    this->$pwd.execute();
    puts("");
  }

  void hostname(std::string &args)
  {
    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      outputRedirection(args);
    this->io.setOutputLine(this->$hostname.execute());
    puts("");
  }

  void username(std::string &args)
  {
    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      outputRedirection(args);
    this->io.setOutputLine(this->$username.execute());
    puts("");
  }

  void touch(std::string &args)
  {
    std::istringstream iss;

    if (contains(args, INPUT_REDIRECTION_SYMBOL))
      this->inputRedirection(args);

    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      this->outputRedirection(args);

    if (io.isStdinStream())
    {
      iss = std::istringstream(args);
      io.setInputStream(iss);
    }

    std::string content = this->io.getAllInputLines();

    for (const std::string &filename : this->getItemsName(content))
    {
      switch (this->$touch.execute(trim(filename)))
      {
      case SUCCESS:
        break;

      case OPEN_FILE_FAILURE:
        std::cout << "Failed to open file.\n";
        break;

      case READ_FAILURE:
        std::cout << "Failed to read file.\n";
        break;

      case MEMORY_ALLOCATION_FAILURE:
        std::cout << "Memory allocation failure.\n";
        break;

      default:
        std::cout << "Failed to execute the command.\n";
        break;
      }
    }

    io.setOutputStream(STDOUT_STREAM);
    puts("");
  }

  void mkDir(std::string &args)
  {
    std::istringstream iss;

    if (contains(args, INPUT_REDIRECTION_SYMBOL))
      this->inputRedirection(args);

    if (io.isStdinStream())
    {
      iss = std::istringstream(args);
      io.setInputStream(iss);
    }

    std::string content = this->io.getAllInputLines();

    for (const std::string &folderName : this->getItemsName(content))
    {
      switch (this->$mkdir.execute(trim(folderName)))
      {
      case SUCCESS:
        std::cout << "Folder created successfully.\n";
        break;

      case FAILURE:
        std::cout << "Failed to create folder.\n";
        break;

      default:
        std::cout << "Failed to execute the command.\n";
        break;
      }
    }

    puts("");
  }

  void rmfile(std::string &args)
  {
    std::istringstream iss;

    if (contains(args, INPUT_REDIRECTION_SYMBOL))
      this->inputRedirection(args);

    if (io.isStdinStream())
    {
      iss = std::istringstream(args);
      io.setInputStream(iss);
    }

    std::string content = this->io.getAllInputLines();

    for (const std::string &filename : this->getItemsName(content))
    {
      switch (this->$rmfile.execute(trim(filename)))
      {
      case SUCCESS:
        std::cout << "File removed successfully.\n";
        break;

      case FAILURE:
        std::cout << "Failed to remove file.\n";
        break;

      default:
        std::cout << "Failed to execute the command.\n";
        break;
      }
    }

    puts("");
  }

  void ls(std::string &args)
  {
    std::string path = "./", mode = "";
    std::istringstream iss;

    if (contains(args, INPUT_REDIRECTION_SYMBOL))
      this->inputRedirection(args);

    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      this->outputRedirection(args);

    if (io.isStdinStream())
    {
      iss = std::istringstream(args);
      io.setInputStream(iss);
    }

    std::string content = this->io.getAllInputLines();

    if (content != "")
    {
      std::vector<std::string> args = this->getItemsName(content);
      if (!args.empty())
      {
        for (const std::string &arg : args)
        {
          if (std::regex_match(arg, std::regex("(\\s*)(-)([^\\s]+)(\\s*)")))
          {
            if (std::regex_match(arg, std::regex("(\\s*)(-a|-l|-la|-al)(\\s*)")))
              mode = arg;
            else
              std::cout << "ls: Argumento inválido: " << arg << "\n";
          }
          else
            path = arg;
        }
      }
    }

    this->$ls.execute(path, mode);
    this->io.setOutputStream(STDOUT_STREAM);

    puts("");
  }

  void rmDir(std::string &args)
  {
    std::istringstream iss;

    if (contains(args, INPUT_REDIRECTION_SYMBOL))
      this->inputRedirection(args);

    if (io.isStdinStream())
    {
      iss = std::istringstream(args);
      io.setInputStream(iss);
    }

    std::string content = this->io.getAllInputLines();

    for (const std::string &filename : this->getItemsName(content))
    {
      switch (this->$rmdir.execute(trim(filename)))
      {
      case SUCCESS:
        std::cout << "Folder removed successfully.\n";
        break;

      case FAILURE:
        std::cout << "Failed to remove folder.\n";
        break;

      default:
        std::cout << "Failed to execute the command.\n";
        break;
      }
    }

    puts("");
  }

  void mv(std::string &args)
  {
    std::istringstream iss;

    if (contains(args, INPUT_REDIRECTION_SYMBOL))
      this->inputRedirection(args);

    if (contains(args, OUTPUT_REDIRECTION_SYMBOL))
      this->outputRedirection(args);

    if (io.isStdinStream())
    {
      iss = std::istringstream(args);
      io.setInputStream(iss);
    }

    std::string content = this->io.getAllInputLines();

    std::vector<std::string> paths = this->getItemsName(content);

    if (paths.size() > 1)
    {
      switch (this->$mv.execute(trim(paths[0]), trim(paths[1])))
      {
      case SUCCESS:
        std::cout << "Moved or renamed successfully.\n";
        break;

      case FILE_NOT_FOUND:
        std::cerr << "File not found.\n";
        break;

      case SAME_SOURCE_N_TARGET:
        std::cerr << "The target and the source are the same.\n";
        break;

      case FAILURE:
        std::cerr << "Failed to move or rename.\n";
        break;

      default:
        std::cerr << "Failed to execute the command.\n";
        break;
      }
    }
    else
      std::cerr << "Invalid arguments!\n";

    puts("");
  }

  void cd(std::string &args) {}

  void execPipeline(const std::string &pipeline)
  {
    std::vector<std::string> commands = split(pipeline, '|');
    int previousPipe[2];
    int currentPipe[2];
    pipe(previousPipe);

    for (size_t i = 0; i < commands.size(); i++)
    {
      std::string command = trim(commands[i]);

      if (i < commands.size() - 1)
      {
        pipe(currentPipe);
      }

      pid_t pid = fork();

      if (pid == 0)
      {
        close(previousPipe[1]);

        if (i < commands.size() - 1)
        {
          dup2(currentPipe[1], STDOUT_FILENO);
          close(currentPipe[0]);
        }

        if (i == 0)
          io.setInputStream(STDIN_STREAM);
        else
          dup2(previousPipe[0], STDIN_FILENO);

        this->execute(command, false, true);

        close(previousPipe[0]);

        if (i < commands.size() - 1)
          close(currentPipe[1]);

        exit(0);
      }
      else if (pid < 0)
      {
        std::cerr << "Failed to fork a child process." << std::endl;
        exit(1);
      }
      else
      {
        close(previousPipe[0]);

        if (i < commands.size() - 1)
        {
          close(currentPipe[1]);
          previousPipe[0] = currentPipe[0];
        }
      }
    }

    for (size_t i = 0; i < commands.size(); i++)
      wait(NULL);
  }

public:
  bool setup()
  {
    int status = SUCCESS;

    this->exitSetup();
    this->echoSetup();
    this->pwdSetup();
    this->hostnameSetup();
    this->usernameSetup();
    this->touchSetup();
    this->mkdirSetup();
    this->rmfileSetup();
    this->lsSetup();
    this->rmdirSetup();
    this->mvSetup();
    this->catSetup();
    this->cdSetup();
    this->grepSetup();
    this->killSetup();

    return true;
  }

  void execute(std::string &script, bool isRunningInBackgroung, bool fromPipeline = false)
  {

    std::istringstream iss(script);
    std::string command, args;

    iss >> command;
    std::getline(iss, args);

    if (command == "exit" or command == "quit")
      isRunning = false;
    else if (command == "echo")
      this->echo(args);
    else if (command == "pwd")
      this->pwd(args);
    else if (command == "hostname")
      this->hostname(args);
    else if (command == "username")
      this->username(args);
    else if (command == "touch")
      this->touch(args);
    else if (command == "mkdir")
      this->mkDir(args);
    else if (command == "rmfile")
      this->rmfile(args);
    else if (command == "ls")
      this->ls(args);
    else if (command == "rmdir")
      this->rmDir(args);
    else if (command == "mv")
      this->mv(args);
    else if (command == "cat")
      cat(args);
    else if (command == "cd")
      this->cd(args);
    else if (command == "grep")
      grep(args, fromPipeline);
    else
      std::cerr << "Command not found: " << command << "\n\n";

    if (std::regex_match(command, std::regex("(\\s*)(cd)(\\s+)(.+)")))
    {
      std::string commandArgumentsString = std::regex_replace(command, std::regex("(\\s*)(cd)(\\s+)"), "");

      std::vector<std::string> args = this->getItemsName(commandArgumentsString);

      if (args.size() == 1)
      {
        if (isRunningInBackgroung)
          std::cout << '\n';

        switch (this->$cd.execute(trim(args[0])))
        {
        case SUCCESS:
          break;

        default:
          std::cout << "Failed to execute the command.\n";
          break;
        }
      }

      puts("");

      if (isRunningInBackgroung)
        this->printPrompt();

      return;
    }
  }

  int init()
  {
    bool runInBackground = false;

    std::cout << "Wellcome to Shell - Command Interpreter!!\n";
    std::cout << "Type 'help' to get a list of available commands.\n\n\n";

    isRunning = true;

    while (isRunning)
    {

      this->io.setInputStream("stdin");

      this->printPrompt();

      std::string textFromPrompt, command;
      textFromPrompt = this->io.getInputLine();

      if (this->io.isEof())
      {
        puts("EOF");
        break;
      }

      if (!io.isStdinStream())
        std::cout << textFromPrompt << '\n';

      runInBackground = std::regex_match(textFromPrompt, std::regex(".*\\s+&\\s*$"));
      command = std::regex_replace(textFromPrompt, std::regex("\\s+&\\s*$"), "");

      if (contains(command, '|'))
      {
        execPipeline(command);
        continue;
      }

      pid_t pid;

      if (runInBackground)
      {
        pid = fork();

        if (pid == 0)
        {
          std::cout << "\nProcess running in background! (PID: " << getpid() << ")\n";

          this->execute(command, runInBackground);

          std::cout << "\nProcess completed! (PID: " << getpid() << ")\n\n";
          this->printPrompt();

          exit(isRunning == false ? QUIT_COMMAND : SUCCESS);
        }
        else if (pid < 0)
          std::cerr << "Erro ao criar o processo filho.\n";
        else
          childProcesses.push_back(pid);
      }
      else
      {
        pid = getpid();
        this->execute(command, runInBackground);

        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != SUCCESS)
          isRunning = false;
      }
    }

    for (pid_t pid : childProcesses)
      this->$kill.execute(pid);

    return 0;
  }
};

#endif