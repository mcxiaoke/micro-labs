#include "ntp.h"

namespace {
// const char ntpServer[] = "time.windows.com";
const char ntpServer[] = "ntp.ntsc.ac.cn";
// NTP time is in the first 48 bytes of message
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

WiFiUDP udp;
unsigned int localPort = 54321;
}  // namespace

int getNtpTimeZone() {
  return TIME_ZONE_OFFSET;
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123);  // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

time_t getNtpTime(unsigned int timeOut) {
  udp.begin(localPort);
  IPAddress ntpServerIP;  // NTP server's ip address
  while (udp.parsePacket() > 0)
    ;  // discard any previously received packets
  WiFi.hostByName(ntpServer, ntpServerIP);
  Serial.print(ntpServer);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  Serial.println("[NTP] Transmit NTP Request");
  sendNTPpacket(ntpServerIP);
  auto beginWait = millis();
  while (millis() - beginWait < timeOut) {
    delay(500);
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("[NTP] Receive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      time_t ts = secsSince1900 - SECS_DELTA_1900_1970 +
                  getNtpTimeZone() * SECS_PER_HOUR;
      if (ts > TIME_START_2019) {
        Serial.print("[NTP] Timestamp: ");
        Serial.println(ts);
        return ts;
      }
    }
  }
  Serial.println("[NTP] No Response :-(");
  return 0;  // return 0 if unable to get the time
}