#ifndef __COMMAND_HPP__
#define __COMMAND_HPP__

#include <string>
#include <functional>

template <typename ResultType, typename... Args>
class Command
{

private:
  std::string name;
  std::string description;
  std::function<ResultType(Args...)> action;

public:

  Command(const std::string &n, const std::string &desc, std::function<ResultType(Args...)> act)
      : name(n), description(desc), action(act) {}

  const std::string &getName() const
  {
    return name;
  }

  const std::string &getDescription() const
  {
    return description;
  }

  ResultType execute(Args... args)
  {
    return action(args...);
  }
};

#endif