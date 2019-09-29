#ifndef _INTERNAL_H
#define _INTERNAL_H

#include <Arduino.h>
#include <EEPROM.h>
#include <FS.h>
#include <time.h>

#define LEAP_YEAR(Y)                        \
  (((1970 + Y) > 0) && !((1970 + Y) % 4) && \
   (((1970 + Y) % 100) || !((1970 + Y) % 400)))

const unsigned long TIME_START = 1000000000L;  // 20010909

size_t writeLog(const String& path, const String& text);
String readLog(const String& path);
int timedRead(Stream& s);
size_t countLine(const String& path);   // with yield, only in loop
size_t countLine2(const String& path);  // without yield, use in anyhwere
String nowString();
String dateString();
String timeString();
String dateString(unsigned long ts);
void eSetup();
void eWriteInt(int address, int value);
int eReadInt(int address);
void htmlEscape(String& html);
String URLEncode(const char* msg);
String urldecode(String str);
String urlencode(String str);
unsigned char h2int(char c);

size_t writeLog(const String& path, const String& text) {
  File f = SPIFFS.open(path, "a");
  if (!f) {
    return -1;
  }
  size_t c = f.print(text);
  c += f.print('\n');
  return c;
}

String readLog(const String& path) {
  File f = SPIFFS.open(path, "r");
  if (!f) {
    return "";
  }
  return f.readString();
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

String nowString() {
  return dateString(now());
}

String dateString() {
  char buf[32];
  sprintf(buf, "%04d-%02d-%02d", year(), month(), day());
  return String(buf);
}
String timeString() {
  char buf[32];
  sprintf(buf, "%02d:%02d:%02d", hour(), minute(), second());
  return String(buf);
}

String dateString(unsigned long ts) {
  char buf[32];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", year(ts), month(ts), day(ts),
          hour(ts), minute(ts), second(ts));
  return String(buf);
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

#endif