#include "cpptools.h"

bool is_digits(const std::string& str) {
  return std::all_of(str.begin(), str.end(), ::isdigit);  // C++11
}

bool is_digits_old(const std::string& str) {
  return str.find_first_not_of("0123456789") == std::string::npos;
}

std::string& str_replace(std::string& s,
                         const std::string& from,
                         const std::string& to) {
  if (!from.empty())
    for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos;
         pos += to.size())
      s.replace(pos, from.size(), to);
  return s;
}

vector<string> split2(const string phrase,
                      const string delimiter,
                      bool once,
                      bool trim_space) {
  vector<string> list;
  string s = string(std::move(phrase));
  size_t pos = 0;
  string token;
  while ((pos = s.find(delimiter)) != string::npos) {
    token = s.substr(0, pos);
    if (trim_space) {
      trim(token);
    }
    list.push_back(token);
    s.erase(0, pos + delimiter.length());
    if (once) {
      break;
    }
  }
  if (trim_space) {
    trim(s);
  }
  list.push_back(s);
  return list;
}