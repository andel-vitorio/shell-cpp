#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <vector>
#include <string>

std::vector<std::string> split(const std::string& str, const char& character)
{
  std::vector<std::string> substrings;
  size_t start = 0;
  size_t end;

  while ((end = str.find(character, start)) != std::string::npos)
  {
    if (end != start)
      substrings.push_back(str.substr(start, end - start));
    start = end + 1;
  }

  substrings.push_back(str.substr(start));

  return substrings;
}

inline bool hasCaracter(std::string str, char character) {
  return str.find(character) != std::string::npos;
}

#endif