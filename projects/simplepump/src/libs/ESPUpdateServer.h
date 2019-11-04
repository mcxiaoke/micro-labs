#ifndef __ESP_HTTP_UPDATE_SERVER_H__
#define __ESP_HTTP_UPDATE_SERVER_H__

#include "compat.h"

#if defined(ESP8266)
#define ESP_SERVER_CLASS ESP8266WebServer
#elif defined(ESP32)
#include <Update.h>
#define ESP_SERVER_CLASS WebServer
#endif

class ESP_SERVER_CLASS;

class ESPUpdateServer
{
  public:
    ESPUpdateServer(bool serial_debug=false);

    void setup(ESP_SERVER_CLASS *server)
    {
      setup(server, emptyString, emptyString);
    }

    void setup(ESP_SERVER_CLASS *server, const String& path)
    {
      setup(server, path, emptyString, emptyString);
    }

    void setup(ESP_SERVER_CLASS *server, const String& username, const String& password)
    {
      setup(server, "/update", username, password);
    }

    void setup(ESP_SERVER_CLASS *server, const String& path, const String& username, const String& password);

    void updateCredentials(const String& username, const String& password)
    {
      _username = username;
      _password = password;
    }

  protected:
    void _setUpdaterError();

  private:
    bool _serial_output;
    ESP_SERVER_CLASS *_server;
    String _username;
    String _password;
    bool _authenticated;
    String _updaterError;
};


#endif
