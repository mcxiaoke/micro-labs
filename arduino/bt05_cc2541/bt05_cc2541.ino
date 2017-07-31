/**
OK
+NAME=BT05
+VERSION=Firmware V3.0.6,Bluetooth V4.0 LE
+LADDR=00:15:83:31:46:CB
+PIN=000000
+TYPE=0
+BAUD=4
+ADVI=0
+NOTI=0
+IMME=0
+UUID=0xFFE0
+IBEA=0
+IBE0=74278BDA
+IBE1=B6444520
+IBE2=8F0C720E
+IBE3=AF059935
ERROR=102
+MINO=0xFFE1
+MEA=0xC5
+PWRM=1
+ROLE=0

iBeacon UUID: 74278BDA-B644-4520-8F0C-720EAF059935

* Command             Description                 *
* ---------------------------------------------------------------- *
* AT                  Check if the command terminal work normally  *
* AT+RESET            Software reboot          *
* AT+VERSION          Get firmware, bluetooth, HCI and LMP version *
* AT+HELP             List all the commands              *
* AT+NAME             Get/Set local device name                    *
* AT+PIN              Get/Set pin code for pairing                 *
* AT+PASS             Get/Set pin code for pairing                 *
* AT+BAUD             Get/Set baud rate                      *
* AT+LADDR            Get local bluetooth address      *
* AT+ADDR             Get local bluetooth address      *
* AT+DEFAULT          Restore factory default        *
* AT+RENEW            Restore factory default        *
* AT+STATE            Get current state          *
* AT+PWRM             Get/Set power on mode(low power)       *
* AT+POWE             Get/Set RF transmit power        *
* AT+SLEEP            Sleep mode                       *
* AT+ROLE             Get/Set current role.                    *
* AT+PARI             Get/Set UART parity bit.                     *
* AT+STOP             Get/Set UART stop bit.                       *
* AT+START            System start working.        *
* AT+IMME             System wait for command when power on.     *
* AT+IBEA             Switch iBeacon mode.                     *
* AT+IBE0             Set iBeacon UUID 0.                        *
* AT+IBE1             Set iBeacon UUID 1.                        *
* AT+IBE2             Set iBeacon UUID 2.                        *
* AT+IBE3             Set iBeacon UUID 3.                        *
* AT+MARJ             Set iBeacon MARJ .                         *
* AT+MINO             Set iBeacon MINO .                         *
* AT+MEA              Set iBeacon MEA .                        *
* AT+NOTI             Notify connection event .                    *
* AT+UUID             Get/Set system SERVER_UUID .                 *
* AT+CHAR             Get/Set system CHAR_UUID .                 *
* -----------------------------------------------------------------*
* Note: (M) = The command support slave mode only.       *
* For more information, please visit http://www.bolutek.com        *
* Copyright@2013 www.bolutek.com. All rights reserved.       *
********************************************************************
 */

#include <SoftwareSerial.h>

#define IO_TX       8
#define IO_RX       9
SoftwareSerial mySerial(IO_RX, IO_TX);   //RX, TX

String _comdata_ = "";             //for incoming serial data

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600); 
  Serial.println("System is Ready!");
  mySerial.println("AT+VERSION");
//  delay(20);
//  mySerial.println("AT+ADDR");
}

void loop() {
  getSerialData();
  if(Serial.available()){
    String order = "";
    while (Serial.available()){
      char cc = (char)Serial.read();
      order += cc;
      delay(2);
    }
    order.trim();
    mySerial.println(order);
  }
  if(_comdata_!=""){
    Serial.println(_comdata_);
    _comdata_ = String("");
  }
}

void getSerialData(){
  while (mySerial.available() > 0){
    _comdata_ += char(mySerial.read());   //get wifi data
    delay(4);
  }
}
