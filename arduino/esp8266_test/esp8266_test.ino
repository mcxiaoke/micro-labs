/**
 * http://www.dfrobot.com.cn/community/forum.php?mod=viewthread&tid=13653
 * 
 * 正面看IO口分布
 * 
 * GND  IO2  IO0  RXD
 * TXD  CHD  IO6  VCC
 * 
 * ESP8266-01模块​​与Arduino搭配
 * ESP8266-01     UNO
 * TX                    8
 * RX                    9
 * 3.3V                 3.3V
 * CH_PD             3.3V
 * GND​​​                GND
 * 
 * 在PC上设置ESP8266-01模块​
 * ESP8266-01     TTL-USB
 * ​UTXD               TXD
 * ​​URXD               RXI
 * ESP8266-01     面包板
 * ​3.3V                 3.3V
 * CH_PD             3.3V
 * GND                GND
 * ​TTL-USB          面包板
 * GND                GND
 * 
 * 初始化指令:
 * AT       返回OK的话证明接线和供电正确, 可以继续执行后面的指令
 * ATE0   关闭回显功能​
 * AT+CWMODE=1                                       设为Station模式​
 * AT+CWJAP="wifi-ssid","wifi-password"   加入你自己的Wifi名称和密码​
 * AT+CWAUTOCONN=1                             设置开机自动连入Wifi​
 * AT+CIPMUX=1                                          设置单连接​
 * AT+RST                                                      重启模块, 如果能获取到IP则证明设置完成​
​ * 重启后, 能看到以下信息证明模块初始化基本完成, 能自动连上你的WIFI:
 * WIFI CONNECTED
 * WIFI GOT IP
 * 如果此处不成功, 请检查你的路由是否限制了新设备接入
 * 最后最重要一步, 修改ESP8266-01的波特率
 * 
 * 指令:AT+CIOBAUD=9600
 * 成功后请用新的波特率连入, 测试一下AT指令
 * 
 * AT+CWSAP?
 * +CWSAP:"esp8266","esp8266",6,0,4,0
 * AT+CIFSR
 * 
 * +CIFSR:APIP,"192.168.4.1"
 * +CIFSR:APMAC,"62:01:94:2a:0b:b7"
 * +CIFSR:STAIP"..4
 * FSA61:07
 * 
 * AT+CIPSTATUS
 * AT+CWLAP
 */
#include <SoftwareSerial.h>

#define WIFI_TX       9
#define WIFI_RX       8
SoftwareSerial wifi(WIFI_RX, WIFI_TX);   //RX, TX

String _comdata_wifi = "";             //for incoming wifi serial data

void setup() {
  Serial.begin(9600);
  wifi.begin(9600); 
  Serial.println("System is Ready!");
  delay(20);
  wifi.println("AT+CIPMUX=1");
  delay(20);
//  wifi.println("AT+CWJAP?");
//  delay(20);
//  wifi.println("AT+CWSAP?");
//  delay(20);
//  wifi.println("AT+CIFSR");
}

void loop() {
  getWifiSerialData();
  if(Serial.available()){
    String order = "";
    while (Serial.available()){
      char cc = (char)Serial.read();
      order += cc;
      delay(2);
    }
    order.trim();
    wifi.println(order);
  }
  if(_comdata_wifi!=""){
    Serial.println(_comdata_wifi);
    _comdata_wifi = String("");
  }
}

void getWifiSerialData(){
  while (wifi.available() > 0){
    _comdata_wifi += char(wifi.read());   //get wifi data
    delay(4);
  }
}
