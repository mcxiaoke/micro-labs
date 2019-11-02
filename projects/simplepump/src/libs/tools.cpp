#include "tools.h"

bool strEqual(const char* str1, const char* str2) {
  return strcmp(str1, str2) == 0;
}

String humanTimeMs(unsigned long ms) {
  return humanTime(ms / 1000);
}

// in seconds
String humanTime(unsigned long sec) {
  if (sec < 1) {
    return "0s";
  }
  // 4010
  // 4010/3600 = 1h
  // 4010%3600 = 410
  // 410/60 = 6m
  // 410%60 = 50
  // 50s
  unsigned long h = sec / 3600;
  unsigned long m = (sec % 3600) / 60;
  unsigned long c = sec % 60;
  String s = "";
  if (h > 0) {
    s += h;
    s += "h";
  }
  if (m > 0) {
    if (s.length() > 1) {
      s += " ";
    }
    s += m;
    s += "m";
  }
  if (c > 0) {
    if (s.length() > 1) {
      s += " ";
    }
    s += c;
    s += "s";
  }
  return s;
}

String formatDateTime(time_t ts) {
  char buf[26];
  getDateTimeStr(ts, buf);
  return String(buf);
}

String formatTime(time_t ts) {
  char buf[12];
  getTimeStr(ts, buf);
  return String(buf);
}

void getDateTimeStr(time_t ts, char* buffer) {
  struct tm* tm_info;
  tm_info = localtime(&ts);
  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
}

void getTimeStr(time_t ts, char* buffer) {
  struct tm* tm_info;
  tm_info = localtime(&ts);
  strftime(buffer, 12, "%H:%M:%S", tm_info);
}

void htmlEscape(String& html) {
  html.replace("&", F("&amp;"));
  html.replace("\"", F("&quot;"));
  html.replace("'", F("&#039;"));
  html.replace("<", F("&lt;"));
  html.replace(">", F("&gt;"));
  html.replace("/", F("&#047;"));
}

String URLEncode(const char* msg) {
  const char* hex = "0123456789abcdef";
  String encodedMsg = "";

  while (*msg != '\0') {
    if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') ||
        ('0' <= *msg && *msg <= '9') || ('-' == *msg) || ('_' == *msg) ||
        ('.' == *msg) || ('~' == *msg)) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}

// https://circuits4you.com/2019/03/21/esp8266-url-encode-decode-example/
String urldecode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      encodedString += ' ';
    } else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      encodedString += c;
    } else {
      encodedString += c;
    }

    yield();
  }

  return encodedString;
}

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
}

unsigned char h2int(char c) {
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}