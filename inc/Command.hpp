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

  Command() {}

  Command(const std::string &n, const std::string &desc, std::function<ResultType(Args...)> act)
      : name(n), description(desc), action(act) {}

  Command& setName(const std::string& name){
    this->name = name;
    return *this;
  }

  Command& setDescription(const std::string& desc) {
    this->description = desc;
    return *this;
  }

  Command& setAction(const std::function<ResultType(Args...)>& act) {
    this->action = act;
    return *this;
  }

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