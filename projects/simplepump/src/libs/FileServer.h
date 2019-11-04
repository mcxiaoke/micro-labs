#ifndef SPIFFS_FILE_READ_SERVER_H
#define SPIFFS_FILE_READ_SERVER_H

#if defined(ESP8266)
#include <ESP8266WebServer.h>
#define SERVER_CLASS ESP8266WebServer
#elif defined(ESP32)
#include <SPIFFS.h>
#include <WebServer.h>
#define SERVER_CLASS WebServer
#endif

#include <FS.h>
#include "utils.h"

struct FileServer {
    static bool handle(SERVER_CLASS* server); 
};

#endif
