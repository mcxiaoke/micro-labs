#include "cpptools.h"

vector<string> split2(const string phrase,
                      const string delimiter,
                      const bool once) {
  vector<string> list;
  string s = string(phrase);
  size_t pos = 0;
  string token;
  while ((pos = s.find(delimiter)) != string::npos) {
    token = s.substr(0, pos);
    list.push_back(token);
    s.erase(0, pos + delimiter.length());
    if (once) {
      break;
    }
  }
  list.push_back(s);
  return list;
}