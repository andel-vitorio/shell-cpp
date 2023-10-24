#ifndef __IO_HPP__
#define __IO_HPP__

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Utils.hpp"

#define INPUT_STREAM_FAIL 0
#define INPUT_STREAM_SUCCESS 1

#define OUTPUT_STREAM_FAIL 0
#define OUTPUT_STREAM_SUCCESS 1

#define STDIN_STREAM "stdin"
#define STDOUT_STREAM "stdout"

class IO
{

private:
  std::istream *input;
  std::ostream *output;
  std::string lastSource;
  std::string lastDestination;
  bool endOfFile;

public:
  IO() : input(&std::cin), output(&std::cout), lastSource(STDIN_STREAM), lastDestination(STDOUT_STREAM), endOfFile(false) {}

  int setInputStream(const std::string &source)
  {
    if (source == STDIN_STREAM)
    {
      input = &std::cin;
      endOfFile = false;
    }
    else if (trim(source) != trim(lastSource))
    {
      if (input != &std::cin)
      {
        dynamic_cast<std::ifstream *>(input)->close();
        delete input;
      }

      std::ifstream *inputFile = new std::ifstream(source);
      if (inputFile->is_open())
      {
        input = inputFile;
        endOfFile = false;
      }
      else
      {
        std::cerr << "Failed to open input file!" << std::endl;
        delete inputFile;
        return INPUT_STREAM_FAIL;
      }
    }

    lastSource = source;
    return INPUT_STREAM_SUCCESS;
  }

  int setInputStream(std::istream &source)
  {
    input = &source;
    lastSource = "";
    endOfFile = false;
    return INPUT_STREAM_SUCCESS;
  }

  std::string getInputLine()
  {
    std::string line;
    if (!endOfFile)
    {
      if (std::getline(*input, line))
        return line;
      else
        endOfFile = true;
    }

    return "";
  }

  std::string getAllInputLines()
  {
    std::string line, lines = "";
    
    while (true)
    {
      line = this->getInputLine();
      if (this->isEof())
        break;
      lines += line + '\n';
    }

    return lines;
  }

  bool isEof() const
  {
    return endOfFile;
  }

  bool isStdinStream()
  {
    return input == &std::cin;
  }

  int setOutputStream(const std::string &destination)
  {
    if (destination == STDOUT_STREAM)
      output = &std::cout;
    else if (trim(destination) != trim(lastDestination))
    {
      if (output != &std::cout)
      {
        dynamic_cast<std::ofstream *>(output)->close();
        delete output;
      }

      std::ofstream *outputFile = new std::ofstream(destination, std::ofstream::out);
      if (outputFile->is_open())
        output = outputFile;
      else
      {
        std::cerr << "Failed to open output file!" << std::endl;
        delete outputFile;
        return OUTPUT_STREAM_FAIL;
      }
    }

    lastDestination = destination;

    return OUTPUT_STREAM_SUCCESS;
  }

  void setOutputLine(const std::string &line)
  {
    *output << line << std::endl;
  }

  void setOutput(const std::string &str)
  {
    *output << str;
  }

  ~IO()
  {
    if (input != &std::cin)
    {
      dynamic_cast<std::ifstream *>(input)->close();
      delete input;
    }

    if (output != &std::cout)
      delete output;
  }
};

#endif