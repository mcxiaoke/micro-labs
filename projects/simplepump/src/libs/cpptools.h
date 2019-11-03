#ifndef __CPP_TOOLS__
#define __CPP_TOOLS__
// #include <iostream>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using std::string;
using std::vector;

#define WS_CHARS " \t\n\r\f\v"

bool is_digits(const std::string& str);

bool is_digits_old(const std::string& str);

std::string& str_replace(std::string& s,
                         const std::string& from,
                         const std::string& to);

// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = WS_CHARS) {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = WS_CHARS) {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = WS_CHARS) {
  return ltrim(rtrim(s, t), t);
}

vector<string> split2(const string phrase,
                      const string delimiter = "\n",
                      bool once = false,
                      bool trim_space = false);

template <template <class, class, class...> class C,
          typename K,
          typename V,
          typename... Args>
V GetWithDef(const C<K, V, Args...>& m, K const& key, const V& defval) {
  typename C<K, V, Args...>::const_iterator it = m.find(key);
  if (it == m.end())
    return defval;
  return it->second;
}

#endif