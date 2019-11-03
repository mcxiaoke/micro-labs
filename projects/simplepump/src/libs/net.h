#ifndef __ESP_HTTP_NET_H__
#define __ESP_HTTP_NET_H__

#include "utils.h"

struct HttpResult {
  const int code;
  const String& uri;
  const String& payload;
};

HttpResult wifiHttpPost(const String&, const String&, WiFiClient& client);
HttpResult wifiHttpGet(const String&, WiFiClient& client);
HttpResult httpPost(const String&, const String&);
HttpResult httpGet(const String&);
HttpResult httpsPost(const String&, const String&);
HttpResult httpsGet(const String&);

#endif