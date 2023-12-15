#include <seng/utils.hpp>

#include <stddef.h>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

vector<char> seng::internal::readFile(const string& filename)
{
  ifstream file(filename, ios::ate | ios::binary);
  if (!file.is_open())
    throw std::runtime_error("failed to open file '" + filename + "'!");

  size_t fileSize = (size_t)file.tellg();
  vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}
