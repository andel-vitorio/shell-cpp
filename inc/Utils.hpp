#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <vector>
#include <string>

inline std::vector<std::string> split(const std::string& str, const char& character)
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

inline std::string trim(const std::string & str) {
    const auto begin = str.find_first_not_of(" \t");

    if ( begin == std::string::npos )
        return "";

    const auto end = str.find_last_not_of(" \t");
    const auto len = end - begin + 1;
    
    return str.substr(begin, len);
}

#endif