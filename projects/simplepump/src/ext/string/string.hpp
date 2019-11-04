#ifndef __EXT_STRINGS_HPP__
#define __EXT_STRINGS_HPP__

// https://github.com/q191201771/libchef/blob/master/include/chef_base/chef_strings_op.hpp

#include <string>
#include <vector>

namespace extstring {

using std::string;
using std::vector;

static const char SPC = ' ';
static const char TAB = '\t';
static const char CR = '\r';
static const char LF = '\n';
static const string DIGITS = "0123456789";
static const string OCTDIGITS = "01234567";
static const string HEXDIGITS_UPPER = "0123456789ABCDEF";
static const string HEXDIGITS = "0123456789abcdefABCDEF";
static const string LETTERS =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const string LOWERCASE = "abcdefghijklmnopqrstuvwxyz";
static const string UPPERCASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const string PUNCTUATION = "!\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~";
static const string WHITESPACE = " \t\n\r\x0b\x0c";
static const char WHITESPACE_CHARS[] = " \t\n\r\x0b\x0c";
static const string PRINTABLE =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&\'()*"
    "+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c";

inline bool is_digits(const string& str) {
  return all_of(str.begin(), str.end(), ::isdigit);  // C++11
}

inline bool is_digits_old(const string& str) {
  return str.find_first_not_of("0123456789") == string::npos;
}

// trim from end of string (right)
inline string& rtrim(string& s, const char* t = WHITESPACE_CHARS) {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

// trim from beginning of string (left)
inline string& ltrim(string& s, const char* t = WHITESPACE_CHARS) {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

// trim from both ends of string (right then left)
inline string& trim(string& s, const char* t = WHITESPACE_CHARS) {
  return ltrim(rtrim(s, t), t);
}

inline bool contains(const string& s, const string& target) {
  return s.find(target) != string::npos;
}

inline bool contains_any(const string& s, const string& charlist) {
  return s.find_first_of(charlist) != string::npos;
}

inline bool has_prefix(const string& s, const string& prefix) {
  // return s.find(prefix) == 0;

  if (s.empty() || prefix.empty()) {
    return false;
  }
  return s.length() >= prefix.length() &&
         s.substr(0, prefix.length()) == prefix;
}

inline bool has_suffix(const string& s, const string& suffix) {
  // size_t pos = s.find(suffix);
  // if (pos == string::npos) {
  //  return false;
  //}
  // return (suffix.length() + pos) == s.length();

  if (s.empty() || suffix.empty()) {
    return false;
  }
  return s.length() >= suffix.length() &&
         s.substr(s.length() - suffix.length(), suffix.length()) == suffix;
}

inline string to_lower(const string& s) {
  string ret;
  for (string::const_iterator c = s.begin(); c != s.end(); ++c) {
    ret += static_cast<char>(tolower(*c));
  }
  return ret;
}

inline string to_upper(const string& s) {
  string ret;
  for (string::const_iterator c = s.begin(); c != s.end(); ++c) {
    ret += static_cast<char>(toupper(*c));
  }
  return ret;
}

inline string& replace_all2(string& s, const string& from, const string& to) {
  if (!from.empty())
    for (size_t pos = 0; (pos = s.find(from, pos)) != string::npos;
         pos += to.size())
      s.replace(pos, from.size(), to);
  return s;
}

inline string replace_all(const string& s,
                          const string& target,
                          const string& replacement) {
  if (s.empty() || target.empty()) {
    return s;
  }
  string ret;
  const size_t target_len = target.length();
  size_t l = 0;
  size_t r = 0;
  for (;;) {
    r = s.find(target, l);
    if (r == string::npos) {
      break;
    }
    ret.append(s.substr(l, r - l));
    ret.append(replacement);
    l = r + target_len;
  }
  if (l < s.length()) {
    ret.append(s.substr(l));
  }
  return ret;
}

inline string replace_first(const string& s,
                            const string& target,
                            const string& replacement) {
  if (s.empty() || target.empty()) {
    return s;
  }

  string ret = s;
  size_t pos = ret.find(target);
  if (pos == string::npos) {
    return ret;
  }

  return ret.replace(pos, target.length(), replacement.c_str());
}

inline string replace_last(const string& s,
                           const string& target,
                           const string& replacement) {
  if (s.empty() || target.empty()) {
    return s;
  }

  string ret = s;
  size_t pos = ret.rfind(target);
  if (pos == string::npos) {
    return ret;
  }

  return ret.replace(pos, target.length(), replacement.c_str());
}

inline vector<string> split_any(const string& s,
                                const string& charlist = WHITESPACE,
                                bool keep_empty_strings = false) {
  vector<string> ret;
  if (s.empty()) {
    return ret;
  }
  if (charlist.empty()) {
    ret.push_back(s);
    return ret;
  }

  const size_t s_len = s.length();
  size_t l = 0;
  size_t r = 0;
  for (;;) {
    r = s.find_first_of(charlist, l);
    if (r != string::npos) {
      if (l != r || keep_empty_strings) {
        ret.push_back(s.substr(l, r - l));
      }
      l = r + 1;
    } else {
      if (l < s_len || keep_empty_strings) {
        ret.push_back(s.substr(l, r - l));
      }
      break;
    }
  }
  return ret;
}

inline vector<string> split2(const string phrase,
                             const string delimiter,
                             bool once,
                             bool trim_space) {
  vector<string> list;
  string s = string(move(phrase));
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

inline vector<string> split(const string& s,
                            const string& sep,
                            bool keep_empty_strings = false) {
  vector<string> ret;
  if (s.empty()) {
    return ret;
  }
  if (sep.empty()) {
    ret.push_back(s);
    return ret;
  }

  const size_t s_len = s.length();
  const size_t sep_len = sep.length();
  size_t l = 0;
  size_t r = 0;
  for (;;) {
    r = s.find(sep, l);
    if (r != string::npos) {
      if (l != r || keep_empty_strings) {
        ret.push_back(s.substr(l, r - l));
      }
      l = r + sep_len;
    } else {
      if (l < s_len || keep_empty_strings) {
        ret.push_back(s.substr(l, r - l));
      }
      break;
    }
  }
  return ret;
}

inline vector<string> splitlines(const string& s, bool keep_ends = false) {
  vector<string> ret;
  if (s.empty()) {
    return ret;
  }
  size_t len = s.length();
  size_t l = 0;
  size_t r = 0;
  string item;
  for (; r != len; r++) {
    if (s[r] == '\r') {
      item = s.substr(l, r - l);
      if (keep_ends) {
        item += '\r';
      }
      if ((r + 1) != len && s[r + 1] == '\n') {
        r++;
        if (keep_ends) {
          item += '\n';
        }
      }
      l = r + 1;
      ret.push_back(item);
    } else if (s[r] == '\n') {
      item = s.substr(l, r - l);
      if (keep_ends) {
        item += '\n';
      }
      l = r + 1;
      ret.push_back(item);
    }
  }
  if (l < r) {
    ret.push_back(s.substr(l, r - l));
  }
  return ret;
}

inline string join(const vector<string>& ss, const string& sep) {
  if (ss.empty()) {
    return string();
  }
  if (ss.size() == 1) {
    return ss[0];
  }

  string ret;
  size_t i = 0;
  size_t size = ss.size();
  for (; i < size - 1; i++) {
    ret += ss[i] + sep;
  }
  ret += ss[i];
  return ret;
}

inline string trim_left(const string& s, const string& charlist = WHITESPACE) {
  if (s.empty() || charlist.empty()) {
    return s;
  }
  size_t pos = 0;
  size_t sl = s.length();
  for (; pos < sl; pos++) {
    if (charlist.find(s[pos]) == string::npos) {
      break;
    }
  }
  return s.substr(pos);
}

inline string trim_right(const string& s, const string& charlist = WHITESPACE) {
  if (s.empty() || charlist.empty()) {
    return s;
  }
  size_t pos = s.length() - 1;
  for (;;) {
    if (charlist.find(s[pos]) == string::npos) {
      break;
    }
    if (pos == 0) {
      pos = -1;
      break;
    }
    pos--;
  }
  return s.substr(0, pos + 1);
}

inline string trim_prefix(const string& s, const string& prefix) {
  return has_prefix(s, prefix) ? s.substr(prefix.length()) : s;
}

inline string trim_suffix(const string& s, const string& suffix) {
  return has_suffix(s, suffix) ? s.substr(0, s.length() - suffix.length()) : s;
}

inline static char unhex(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'a' && ch <= 'z') {
    return ch - 'a' + 10;
  } else if (ch >= 'A' && ch <= 'Z') {
    return ch - 'A' + 10;
  }
  return 0;
}

inline string url_encode(const string& s) {
  if (s.empty()) {
    return s;
  }

  string result;
  result.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++) {
    const char& ch = s[i];
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
        (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' ||
        ch == '~') {
      result += ch;
    } else if (ch == ' ') {
      result += '+';
    } else {
      result += '%';
      result += HEXDIGITS_UPPER[ch >> 4];
      result += HEXDIGITS_UPPER[ch & 15];
    }
  }
  return result;
}

inline string url_decode(const string& s) {
  if (s.empty()) {
    return s;
  }

  string result;
  result.reserve(s.length());
  for (size_t i = 0; i < s.length();) {
    const char& ch = s[i];
    if (ch == '%') {
      if (i + 2 >= s.length() || HEXDIGITS.find(s[i + 1]) == string::npos ||
          HEXDIGITS.find(s[i + 2]) == string::npos) {
        return string();
      }
      result += ((unhex(s[i + 1]) << 4) | unhex(s[i + 2]));
      i += 3;
    } else if (ch == '+') {
      result += ' ';
      i++;
    } else {
      result += ch;
      i++;
    }
  }
  return result;
}

}  // namespace extstring

#endif