#ifndef __ESP_HTTP_NET_H__
#define __ESP_HTTP_NET_H__

#include "utils.h"

typedef struct {
  int code;
  String uri;
  String payload;
} HttpResult;

HttpResult wifiHttpPost(const String&, const String&, WiFiClient& client);
HttpResult wifiHttpGet(const String&, WiFiClient& client);
HttpResult httpPost(const String&, const String&);
HttpResult httpGet(const String&);
HttpResult httpsPost(const String&, const String&);
HttpResult httpsGet(const String&);

#endif