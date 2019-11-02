#include "ESPCompat.h"

String listFiles() {
  String output = "";
  Serial.println(F("[System] SPIFFS Files:"));
#if defined(ESP8266)

  Dir f = SPIFFS.openDir("/");
  while (f.next()) {
    Serial.println("[File] %s (%d bytes)\n", f.fileName().c_str(),
                   f.fileSize());
    output += f.fileName();
    output += "\n";
  };

#elif defined(ESP32)
  File root = SPIFFS.open("/");
  if (root.isDirectory()) {
    File f = root.openNextFile();
    while (f) {
      Serial.printf("[File] %s (%d bytes)\n", f.name(), f.size());
      output += f.name();
      // output += " (";
      // output += f.fileSize();
      // output += ")";
      output += "\n";
      f = root.openNextFile();
    };
  }

#endif
  return output;
}