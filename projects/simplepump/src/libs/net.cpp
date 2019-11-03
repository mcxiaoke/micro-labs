#include "net.h"

HttpResult wifiHttpPost(const String& url,
                        const String& body,
                        WiFiClient& client) {
  HTTPClient http;
  LOGF("[HTTP] POST, url: %s\n", url.c_str());
  int httpCode = -1;
  String payload = "";
  if (http.begin(client, url)) {  // HTTP
    // LOGN("[HTTP] POST sending...");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    // LOGF("[HTTP] POST, body: %s\n", urldecode(body).c_str());
    httpCode = http.POST(body);
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      LOGF("[HTTP] POST, code: %d\n", httpCode);
      // file found at server
      payload = http.getString();
      LOGF("[HTTP] POST, content: %s\n", payload.c_str());
    } else {
      LOGF("[HTTP] POST, error: %s\n", http.errorToString(httpCode).c_str());
    }
  } else {
    LOGN("[HTTP] POST failed.");
  }
  http.end();
  yield();
  HttpResult ret{httpCode, url, payload};
  return ret;
}

HttpResult wifiHttpGet(const String& url, WiFiClient& client) {
  HTTPClient http;
  LOGF("[HTTP] GET, url: %s\n", url.c_str());
  int httpCode = -1;
  String payload = "";
  if (http.begin(client, url)) {  // HTTP
    // LOGN("[HTTP] GET sending...");
    httpCode = http.GET();
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      LOGF("[HTTP] GET, code: %d\n", httpCode);
      // file found at server
      payload = http.getString();
      LOGF("[HTTP] GET, content: %s\n", payload.c_str());
    } else {
      LOGF("[HTTP] GET, error: %s\n", http.errorToString(httpCode).c_str());
    }
  } else {
    LOGN("[HTTP] GET failed.");
  }
  http.end();
  HttpResult ret{httpCode, url, payload};
  return ret;
}

HttpResult httpPost(const String& url, const String& body) {
  WiFiClient client;
  return wifiHttpPost(url, body, client);
}

HttpResult httpGet(const String& url) {
  WiFiClient client;
  return wifiHttpGet(url, client);
}

HttpResult httpsPost(const String& url, const String& body) {
#if defined(ESP8266)
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
#elif defined(ESP32)
  WiFiClientSecure client;
//   client.setInsecure();
#endif
  return wifiHttpPost(url, body, client);
}

HttpResult httpsGet(const String& url) {
#if defined(ESP8266)
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
#elif defined(ESP32)
  WiFiClientSecure client;
//   client.setInsecure();
#endif
  return wifiHttpGet(url, client);
}