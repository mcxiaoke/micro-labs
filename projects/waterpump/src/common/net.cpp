#include "net.h"

String wifiHttpPost(String& url, String& body, WiFiClient& client) {
  HTTPClient http;
  if (NET_DEBUG_LOG) {
    LOGF("[HTTP] POST, url: %s\n", url.c_str());
  }
  if (http.begin(client, url)) {  // HTTP
    // Serial.print("[HTTP] POST sending...\n");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    // LOGF("[HTTP] POST, body: %s\n", urldecode(body).c_str());
    int httpCode = http.POST(body);
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      LOGF("[HTTP] POST, code: %d\n", httpCode);
      // file found at server
      String payload = http.getString();
    //   LOGF("[HTTP] POST, content: %s\n", payload.c_str());
      return payload;
    } else {
      LOGF("[HTTP] POST, error: %s\n",
                    http.errorToString(httpCode).c_str());
    }
  } else {
    LOGN("[HTTP] POST failed.");
  }
  http.end();
  yield();
  return "";
}

String wifiHttpGet(String& url, WiFiClient& client) {
  HTTPClient http;
  LOGF("[HTTP] GET, url: %s\n", url.c_str());
  if (http.begin(client, url)) {  // HTTP
    // Serial.print("[HTTP] GET sending...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      LOGF("[HTTP] GET, code: %d\n", httpCode);
      // file found at server
      String payload = http.getString();
    //   LOGF("[HTTP] GET, content: %s\n", payload.c_str());
      return payload;
    } else {
      LOGF("[HTTP] GET, error: %s\n",
                    http.errorToString(httpCode).c_str());
    }
  } else {
    LOGN("[HTTP] GET failed.");
  }
  http.end();
  yield();
  return "";
}

String httpPost(String& url, String& body) {
  WiFiClient client;
  return wifiHttpPost(url, body, client);
}

String httpGet(String& url) {
  WiFiClient client;
  return wifiHttpGet(url, client);
}

String httpsPost(String& url, String& body) {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  return wifiHttpPost(url, body, client);
}

String httpsGet(String& url) {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  return wifiHttpGet(url, client);
}

String getWiFiDisconnectReason(int code) {
  String reason = "(";

  reason += code;
  reason += F(") ");

  switch (code) {
    case WIFI_DISCONNECT_REASON_UNSPECIFIED:
      reason += F("Unspecified");
      break;
    case WIFI_DISCONNECT_REASON_AUTH_EXPIRE:
      reason += F("Auth expire");
      break;
    case WIFI_DISCONNECT_REASON_AUTH_LEAVE:
      reason += F("Auth leave");
      break;
    case WIFI_DISCONNECT_REASON_ASSOC_EXPIRE:
      reason += F("Assoc expire");
      break;
    case WIFI_DISCONNECT_REASON_ASSOC_TOOMANY:
      reason += F("Assoc toomany");
      break;
    case WIFI_DISCONNECT_REASON_NOT_AUTHED:
      reason += F("Not authed");
      break;
    case WIFI_DISCONNECT_REASON_NOT_ASSOCED:
      reason += F("Not assoced");
      break;
    case WIFI_DISCONNECT_REASON_ASSOC_LEAVE:
      reason += F("Assoc leave");
      break;
    case WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED:
      reason += F("Assoc not authed");
      break;
    case WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD:
      reason += F("Disassoc pwrcap bad");
      break;
    case WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD:
      reason += F("Disassoc supchan bad");
      break;
    case WIFI_DISCONNECT_REASON_IE_INVALID:
      reason += F("IE invalid");
      break;
    case WIFI_DISCONNECT_REASON_MIC_FAILURE:
      reason += F("Mic failure");
      break;
    case WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT:
      reason += F("4way handshake timeout");
      break;
    case WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT:
      reason += F("Group key update timeout");
      break;
    case WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS:
      reason += F("IE in 4way differs");
      break;
    case WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID:
      reason += F("Group cipher invalid");
      break;
    case WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID:
      reason += F("Pairwise cipher invalid");
      break;
    case WIFI_DISCONNECT_REASON_AKMP_INVALID:
      reason += F("AKMP invalid");
      break;
    case WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION:
      reason += F("Unsupp RSN IE version");
      break;
    case WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP:
      reason += F("Invalid RSN IE cap");
      break;
    case WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED:
      reason += F("802 1X auth failed");
      break;
    case WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED:
      reason += F("Cipher suite rejected");
      break;
    case WIFI_DISCONNECT_REASON_BEACON_TIMEOUT:
      reason += F("Beacon timeout");
      break;
    case WIFI_DISCONNECT_REASON_NO_AP_FOUND:
      reason += F("No AP found");
      break;
    case WIFI_DISCONNECT_REASON_AUTH_FAIL:
      reason += F("Auth fail");
      break;
    case WIFI_DISCONNECT_REASON_ASSOC_FAIL:
      reason += F("Assoc fail");
      break;
    case WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT:
      reason += F("Handshake timeout");
      break;
    default:
      reason += "Unknown";
      break;
  }
  return reason;
}