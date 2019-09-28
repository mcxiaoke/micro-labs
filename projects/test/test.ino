#include <Blynk.h>
#include <BlynkSimpleEsp8266.h>
#define BLYNK_PRINT Serial
//#include <WiFiClient.h>
//#include <WiFiClientSecureBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
const char* auth = "59CFMgpNHTJTLePxBrVkSSXaoNvONwoz";
const char* url = "https://sc.ftqq.com/SCU11720T078194929646e883a6714faa82db1a1d59b8f38e5a266.send";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "CMCC-9EDc";
char pass[] = "idqf6rnx";

int ledPin = 2;//d4 LED_BUILTIN
int relayPin = 4;//d2
BlynkTimer timer;
// libraries/ESP8266WiFi/src/ESP8266WiFiType.h
WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;
WiFiEventHandler stationGotIPHandler;

ESP8266WebServer server(80);
void handleRoot();
void handleNotFound();
void handleLED();
void handleRelay();

String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

//https://circuits4you.com/2019/03/21/esp8266-url-encode-decode-example/
String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}

void send_report(){
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  Serial.printf("[HTTP] Request, url: %s\n", url);
  if (http.begin(client, url)) {  // HTTP
      Serial.print("[HTTP] Request sending...\n");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String data = "text=";
      data += urlencode("ESP8266-Status-Report");
      data += "&desp=";
      data += urlencode("ESP8266 is Online, ");
      data += urlencode("Wifi IP:");
      data += urlencode(WiFi.localIP().toString());
      Serial.printf("[HTTP] Request, body: %s\n", data.c_str());
      // start connection and send HTTP header
//      int httpCode = http.GET();
      int httpCode = http.POST(data);

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] Response, code: %d\n", httpCode);

        // file found at server
        String payload = http.getString();
        Serial.printf("[HTTP] Response, content: %s\n", payload.c_str());
//        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
//          String payload = http.getString();
//          Serial.println(payload);
//        }
      } else {
        Serial.printf("[HTTP] Response, error: %s\n", http.errorToString(httpCode).c_str());
      }
}
  
 }

 void onStationConnected(const WiFiEventStationModeConnected& evt) {
  Serial.print("[WiFi] connected to ");
  Serial.println(evt.ssid);
}

void onStationDisconnected(const WiFiEventStationModeDisconnected& evt) {
  Serial.print("[WiFi] disconnected from ");
  Serial.println(evt.ssid);
}

void onStationGotIP(const WiFiEventStationModeGotIP& evt) {
  Serial.print("[WiFi] got ip: ");
  Serial.println(evt.ip);
}

void handleRoot() {
  String body = "<!doctype html>\n";
  body += "<html lang=\"en\">\n";
  body += "<head>\n";
  body += "<meta charset=\"utf-8\">\n";
  body += "<meta name=\"viewport\" content=\"width=device-width\">\n";
  body += "<title>ESP8266 Server</title></head>\n<body>\n";
  bool ledOn = digitalRead(ledPin) == 1;
  bool relayOn = digitalRead(relayPin) == 1;
  body += "<h1>ESP8266 Server</h1>\n";
  body += "<p></p>\n";
  body += "<p>LED Status: ";
  body += ledOn? "On": "Off";
  body += "</p>\n<p>";
  body += "Relay Status: ";
  body += relayOn? "On": "Off";
  body += "</p>\n";
  body += "<form action=\"/led\" method=\"POST\">\n";
  body += "<input type=\"submit\" value=\"Toggle LED\"></form>\n";
  body += "<form action=\"/relay\" method=\"POST\">\n";
  body += "<input type=\"submit\" value=\"Toggle Relay\"></form>\n";
  body += "</body>\n</html>\n";
  server.send(200, "text/html", body);
}

void handleNotFound(){
  Serial.println("Server handleNotFound");
  server.send(404, "text/plain", "404: Not found");
}

void handleLED() {  
  Serial.println("Server handleLED");
  // If a POST request is made to URI /LED
  digitalWrite(ledPin,!digitalRead(ledPin));      // Change the state of the LED
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void handleRelay(){
  Serial.println("Server handleRelay");
  digitalWrite(relayPin,!digitalRead(relayPin));      // Change the state of the Relay
  server.sendHeader("Location","/");
  server.send(303);  
}

void server_setup(){
if (MDNS.begin("esp8266")) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }  
  server.on("/", handleRoot); 
  server.on("/led", HTTP_POST, handleLED);
  server.on("/relay", HTTP_POST, handleRelay);
//  server.onNotFound(handleNotFound); 
  server.onNotFound([](){
    server.send(404, "text/plain", "404: Not found");
  });
  server.begin(); 
  Serial.println("HTTP server started");
}

 void wifi_setup(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  WiFi.setAutoReconnect(true);
  stationConnectedHandler = WiFi.onStationModeConnected(&onStationConnected);
  stationDisconnectedHandler = WiFi.onStationModeDisconnected(&onStationDisconnected);
  stationGotIPHandler = WiFi.onStationModeGotIP(&onStationGotIP);
  Serial.print("Wifi Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
   //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Wifi Connected to ");
  Serial.println(ssid);
  Serial.print("Wifi IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
  Serial.print("Wifi hostname: ");
  Serial.println(WiFi.hostname());
  digitalWrite(ledPin, HIGH);
}

BLYNK_WRITE(V0) //Button Widget is writing to pin V0
{
  int v = param.asInt(); 
   Serial.printf("Blynk value %d received at V0\n", v);
  if (v == 0){
    digitalWrite(ledPin, LOW);
    }else{
      digitalWrite(ledPin, HIGH);
   }
}

BLYNK_CONNECTED() {
  Serial.println("Blynk Connected!");
  Blynk.syncAll();
}

BLYNK_APP_CONNECTED() {
  Serial.println("Blynk App Connected!");
}

BLYNK_APP_DISCONNECTED() {
  Serial.println("Blynk App Disconnected!");
}

void checkWiFi(){
  Serial.print("Wifi status: ");
  Serial.print(WiFi.status());
  Serial.print(" IP: ");
  Serial.println(WiFi.localIP());
  if (!WiFi.isConnected()){
    Serial.println("Wifi reconnecting...");
    WiFi.reconnect();
  }
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  delay(100);
//  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("ESP8266 Board is running!");
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  wifi_setup();
  server_setup();
  send_report();
  Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);
  timer.setInterval(30000L, checkWiFi);
}

void loop()
{
  Blynk.run();
  timer.run();
  server.handleClient();
}
