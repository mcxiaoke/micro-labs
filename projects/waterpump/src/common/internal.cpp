#include "internal.h"

String logFileName() {
  String fileName = "/file/log-";
  fileName += dateString();
  fileName += ".txt";
  return fileName;
}

size_t fileLog(const String& text, bool appendDate) {
  String message = "";
  if (appendDate) {
    message += "[";
    message += timeString();
    message += "] ";
  }
  message += text;
  String fileName = logFileName();
  size_t c = writeLog(fileName, message);
  message = "";
  fileName = "";
  return c;
}

size_t writeLog(const String& path, const String& text) {
  File f = SPIFFS.open(path, "a");
  if (!f) {
    return -1;
  }
  size_t c = f.print(text);
  c += f.print('\n');
  f.close();
  return c;
}

String readLog(const String& path) {
  File f = SPIFFS.open(path, "r");
  if (!f) {
    return "";
  }
  String s = f.readString();
  f.close();
  return s;
}

int timedRead(Stream& s) {
  int c;
  unsigned long _startMillis = millis();
  do {
    c = s.read();
    if (c >= 0)
      return c;
    yield();
  } while (millis() - _startMillis < 500);
  return -1;
}

String readStringUntil(Stream& s, char terminator) {
  String ret;
  int c = s.read();
  while (c >= 0 && c != terminator) {
    ret += (char)c;
    c = s.read();
  }
  return ret;
}

size_t countLine(const String& path) {
  File f = SPIFFS.open(path, "r");
  if (!f) {
    return 0;
  }
  size_t c = 0;
  int i;
  while (true) {
    i = timedRead(f);
    if (i == -1) {
      break;
    }
    if (i == '\n') {
      c++;
    }
  }

  return c;
}

size_t countLine2(const String& path) {
  File f = SPIFFS.open(path, "r");
  if (!f) {
    return 0;
  }
  size_t c = 0;
  int i;
  while (true) {
    i = f.read();
    if (i == -1) {
      break;
    }
    if (i == '\n') {
      c++;
    }
  }

  return c;
}

String nowStringGMT() {
  return gmtString(now());
}

String nowString() {
  if (now() < TIME_START) {
    return "N/A";
  }
  return dateTimeString(now());
}

String dateString() {
  return dateString(now());
}
String timeString() {
  return timeString(now());
}

String dateString(unsigned long ts) {
  ts += TZ_OFFSET;
  if (ts < TIME_START) {
    return "NA";
  }
  char buf[16];
  sprintf(buf, "%04d-%02d-%02d", year(ts), month(ts), day(ts));
  return String(buf);
}
String timeString(unsigned long ts) {
  ts += TZ_OFFSET;
  if (ts < TIME_START) {
    return "N/A";
  }
  char buf[16];
  sprintf(buf, "%02d:%02d:%02d", hour(ts), minute(ts), second(ts));
  return String(buf);
}

String dateTimeString(unsigned long ts) {
  ts += TZ_OFFSET;
  if (ts < TIME_START) {
    return "N/A";
  }
  char buf[32];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", year(ts), month(ts), day(ts),
          hour(ts), minute(ts), second(ts));
  return String(buf);
}

String gmtString(unsigned long ts) {
  ts += TZ_OFFSET;
  if (ts < TIME_START) {
    return "N/A";
  }
  char buf[36];
  sprintf(buf, "%s, %02d %s %04d %02d:%02d:%02d GMT", dayShortStr(ts), day(ts),
          monthShortStr(ts), year(ts), hour(ts), minute(ts), second(ts));
  return String(buf);
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

void eSetup() {
  EEPROM.begin(1024);
}

void eWriteInt(int address, int value) {
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);

  EEPROM.write(address, two);
  EEPROM.write(address + 1, one);
  EEPROM.commit();
}

int eReadInt(int address) {
  long two = EEPROM.read(address);
  long one = EEPROM.read(address + 1);

  return ((two << 0) & 0xFFFFFF) + ((one << 8) & 0xFFFFFFFF);
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
